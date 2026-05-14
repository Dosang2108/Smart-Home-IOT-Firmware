#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import logging
import os
import signal
import sqlite3
import ssl
import threading
import time
from dataclasses import dataclass
from datetime import datetime, timezone
from pathlib import Path
from typing import Any, Iterable

try:
    import paho.mqtt.client as mqtt
except ModuleNotFoundError:  # pragma: no cover - handled at runtime.
    mqtt = None


LOG = logging.getLogger("yolohome.gateway")


DEFAULT_UPLINK_TOPICS = (
    "yolohome/device/yolo_uno_01/ack",
    "yolohome/device/yolo_uno_01/state",
    "yolohome/device/yolo_uno_01/telemetry",
    "yolohome/device/yolo_uno_01/event",
    "yolohome/device/yolo_uno_01/availability",
)

DEFAULT_DOWNLINK_TOPICS = ("yolohome/device/yolo_uno_01/cmd",)


@dataclass(frozen=True)
class BrokerConfig:
    host: str
    port: int
    client_id: str
    username: str = ""
    password: str = ""
    tls: bool = False
    insecure_tls: bool = False
    ca_certs: str = ""

    @classmethod
    def from_dict(cls, data: dict[str, Any], prefix: str) -> "BrokerConfig":
        env_prefix = prefix.upper()
        return cls(
            host=os.getenv(f"{env_prefix}_MQTT_HOST", data.get("host", "127.0.0.1")),
            port=int(os.getenv(f"{env_prefix}_MQTT_PORT", data.get("port", 1883))),
            client_id=data.get("client_id", f"yolohome-{prefix}"),
            username=os.getenv(f"{env_prefix}_MQTT_USERNAME", data.get("username", "")),
            password=os.getenv(f"{env_prefix}_MQTT_PASSWORD", data.get("password", "")),
            tls=bool(data.get("tls", False)),
            insecure_tls=bool(data.get("insecure_tls", False)),
            ca_certs=data.get("ca_certs", ""),
        )


@dataclass(frozen=True)
class GatewayConfig:
    database: Path
    local: BrokerConfig
    cloud: BrokerConfig
    uplink_topics: tuple[str, ...]
    downlink_topics: tuple[str, ...]
    flush_interval_seconds: float = 2.0
    publish_timeout_seconds: float = 5.0
    batch_size: int = 100
    state_policy: str = "latest"

    @classmethod
    def from_file(cls, path: Path) -> "GatewayConfig":
        with path.open("r", encoding="utf-8") as fh:
            data = json.load(fh)

        base_dir = path.parent
        database = Path(data.get("database", "gateway_buffer.db"))
        if not database.is_absolute():
            database = base_dir / database

        return cls(
            database=database,
            local=BrokerConfig.from_dict(data.get("local", {}), "local"),
            cloud=BrokerConfig.from_dict(data.get("cloud", {}), "cloud"),
            uplink_topics=tuple(data.get("uplink_topics", DEFAULT_UPLINK_TOPICS)),
            downlink_topics=tuple(data.get("downlink_topics", DEFAULT_DOWNLINK_TOPICS)),
            flush_interval_seconds=float(data.get("flush_interval_seconds", 2.0)),
            publish_timeout_seconds=float(data.get("publish_timeout_seconds", 5.0)),
            batch_size=int(data.get("batch_size", 100)),
            state_policy=data.get("state_policy", "latest"),
        )


@dataclass(frozen=True)
class QueuedMessage:
    id: int
    topic: str
    payload_json: str
    qos: int
    retain: bool
    created_at: str
    attempt_count: int


class OutboundQueue:
    def __init__(self, database: Path | str):
        self.database = Path(database)
        self.database.parent.mkdir(parents=True, exist_ok=True)
        self._lock = threading.RLock()
        self._conn = sqlite3.connect(
            str(self.database),
            timeout=30,
            isolation_level=None,
            check_same_thread=False,
        )
        self._init_schema()

    def close(self) -> None:
        with self._lock:
            self._conn.close()

    def _init_schema(self) -> None:
        with self._lock:
            self._conn.execute("PRAGMA journal_mode=WAL;")
            self._conn.execute("PRAGMA synchronous=NORMAL;")
            self._conn.execute(
                """
                CREATE TABLE IF NOT EXISTS outbound_queue (
                  id INTEGER PRIMARY KEY AUTOINCREMENT,
                  topic TEXT NOT NULL,
                  payload_json TEXT NOT NULL,
                  qos INTEGER NOT NULL DEFAULT 1,
                  retain INTEGER NOT NULL DEFAULT 0,
                  created_at TEXT NOT NULL,
                  attempt_count INTEGER NOT NULL DEFAULT 0,
                  last_attempt_at TEXT,
                  last_error TEXT
                );
                """
            )
            self._conn.execute(
                "CREATE INDEX IF NOT EXISTS idx_outbound_queue_id ON outbound_queue(id);"
            )
            self._conn.execute(
                "CREATE INDEX IF NOT EXISTS idx_outbound_queue_topic ON outbound_queue(topic);"
            )

    def enqueue(
        self,
        topic: str,
        payload_json: str,
        qos: int = 1,
        retain: bool = False,
        replace_pending_for_topic: bool = False,
    ) -> int:
        created_at = datetime.now(timezone.utc).isoformat()
        with self._lock, self._conn:
            if replace_pending_for_topic:
                self._conn.execute("DELETE FROM outbound_queue WHERE topic = ?;", (topic,))
            cur = self._conn.execute(
                """
                INSERT INTO outbound_queue
                  (topic, payload_json, qos, retain, created_at)
                VALUES (?, ?, ?, ?, ?);
                """,
                (topic, payload_json, int(qos), 1 if retain else 0, created_at),
            )
            return int(cur.lastrowid)

    def fetch_batch(self, limit: int) -> list[QueuedMessage]:
        with self._lock:
            rows = self._conn.execute(
                """
                SELECT id, topic, payload_json, qos, retain, created_at, attempt_count
                FROM outbound_queue
                ORDER BY id ASC
                LIMIT ?;
                """,
                (int(limit),),
            ).fetchall()

        return [
            QueuedMessage(
                id=int(row[0]),
                topic=str(row[1]),
                payload_json=str(row[2]),
                qos=int(row[3]),
                retain=bool(row[4]),
                created_at=str(row[5]),
                attempt_count=int(row[6]),
            )
            for row in rows
        ]

    def delete(self, message_id: int) -> None:
        with self._lock, self._conn:
            self._conn.execute("DELETE FROM outbound_queue WHERE id = ?;", (int(message_id),))

    def mark_failed(self, message_id: int, error: str) -> None:
        now = datetime.now(timezone.utc).isoformat()
        with self._lock, self._conn:
            self._conn.execute(
                """
                UPDATE outbound_queue
                SET attempt_count = attempt_count + 1,
                    last_attempt_at = ?,
                    last_error = ?
                WHERE id = ?;
                """,
                (now, error[:500], int(message_id)),
            )

    def count(self) -> int:
        with self._lock:
            row = self._conn.execute("SELECT count(*) FROM outbound_queue;").fetchone()
            return int(row[0])


def should_store_topic(topic: str) -> bool:
    if topic.endswith("/availability"):
        return False
    if topic.endswith("/cmd"):
        return False
    return True


def should_replace_pending_state(topic: str, state_policy: str) -> bool:
    return state_policy == "latest" and topic.endswith("/state")


def cloud_retain_for_topic(topic: str, inbound_retain: bool) -> bool:
    if topic.endswith("/state") or topic.endswith("/availability"):
        return True
    return inbound_retain


def reason_code_value(reason_code: Any) -> int:
    return int(getattr(reason_code, "value", reason_code))


class StoreForwardGateway:
    def __init__(self, config: GatewayConfig):
        if mqtt is None:
            raise RuntimeError("Missing dependency: install paho-mqtt from gateway/requirements.txt")

        self.config = config
        self.queue = OutboundQueue(config.database)
        self.cloud_connected = threading.Event()
        self.local_connected = threading.Event()
        self.stop_event = threading.Event()
        self.stopped = threading.Event()
        self.local_client = self._make_client(config.local)
        self.cloud_client = self._make_client(config.cloud)
        self.flush_thread = threading.Thread(target=self._flush_loop, name="queue-flush", daemon=True)

        self.local_client.on_connect = self._on_local_connect
        self.local_client.on_disconnect = self._on_local_disconnect
        self.local_client.on_message = self._on_local_message
        self.cloud_client.on_connect = self._on_cloud_connect
        self.cloud_client.on_disconnect = self._on_cloud_disconnect
        self.cloud_client.on_message = self._on_cloud_message

    def _make_client(self, broker: BrokerConfig):
        client = mqtt.Client(client_id=broker.client_id, clean_session=True)
        if broker.username:
            client.username_pw_set(broker.username, broker.password or None)
        if broker.tls:
            ca_certs = broker.ca_certs or None
            client.tls_set(ca_certs=ca_certs, tls_version=ssl.PROTOCOL_TLS_CLIENT)
            client.tls_insecure_set(broker.insecure_tls)
        return client

    def run(self) -> None:
        LOG.info("opening sqlite queue at %s", self.config.database)
        self.flush_thread.start()
        self._connect(self.cloud_client, self.config.cloud, "cloud")
        self._connect(self.local_client, self.config.local, "local")

        while not self.stop_event.is_set():
            time.sleep(0.2)

    def stop(self) -> None:
        if self.stopped.is_set():
            return
        self.stopped.set()
        self.stop_event.set()
        for client in (self.local_client, self.cloud_client):
            try:
                client.disconnect()
                client.loop_stop()
            except Exception:
                LOG.exception("failed to stop mqtt client")
        self.flush_thread.join(timeout=3)
        self.queue.close()

    def _connect(self, client, broker: BrokerConfig, name: str) -> None:
        LOG.info("connecting %s mqtt to %s:%s tls=%s", name, broker.host, broker.port, broker.tls)
        client.connect_async(broker.host, broker.port, keepalive=60)
        client.reconnect_delay_set(min_delay=1, max_delay=30)
        client.loop_start()

    def _on_local_connect(self, client, _userdata, _flags, rc, *_extra) -> None:
        if reason_code_value(rc) != 0:
            LOG.warning("local mqtt connect failed rc=%s", rc)
            return
        self.local_connected.set()
        for topic in self.config.uplink_topics:
            client.subscribe(topic, qos=1)
            LOG.info("subscribed local uplink %s", topic)

    def _on_local_disconnect(self, _client, _userdata, *_args) -> None:
        self.local_connected.clear()
        LOG.warning("local mqtt disconnected")

    def _on_cloud_connect(self, client, _userdata, _flags, rc, *_extra) -> None:
        if reason_code_value(rc) != 0:
            self.cloud_connected.clear()
            LOG.warning("cloud mqtt connect failed rc=%s", rc)
            return
        self.cloud_connected.set()
        LOG.info("cloud mqtt connected; pending=%s", self.queue.count())
        for topic in self.config.downlink_topics:
            client.subscribe(topic, qos=1)
            LOG.info("subscribed cloud downlink %s", topic)

    def _on_cloud_disconnect(self, _client, _userdata, *_args) -> None:
        self.cloud_connected.clear()
        LOG.warning("cloud mqtt disconnected")

    def _on_local_message(self, _client, _userdata, message) -> None:
        payload_text = message.payload.decode("utf-8", errors="replace")
        topic = str(message.topic)
        qos = 1
        retain = cloud_retain_for_topic(topic, bool(message.retain))

        if self.cloud_connected.is_set() and self._publish_cloud_sync(topic, payload_text, qos, retain):
            LOG.info("forwarded live %s", topic)
            return

        if not should_store_topic(topic):
            LOG.warning("dropped non-replayable topic while offline: %s", topic)
            return

        replace_state = should_replace_pending_state(topic, self.config.state_policy)
        row_id = self.queue.enqueue(topic, payload_text, qos, retain, replace_state)
        LOG.info("stored offline row=%s topic=%s pending=%s", row_id, topic, self.queue.count())

    def _on_cloud_message(self, _client, _userdata, message) -> None:
        topic = str(message.topic)
        if topic not in self.config.downlink_topics:
            return
        if not self.local_connected.is_set():
            LOG.warning("dropped cloud command because local broker is offline: %s", topic)
            return
        info = self.local_client.publish(topic, message.payload, qos=message.qos, retain=False)
        if info.rc == mqtt.MQTT_ERR_SUCCESS:
            LOG.info("forwarded command to local broker: %s", topic)
        else:
            LOG.warning("failed to forward command to local broker: %s rc=%s", topic, info.rc)

    def _publish_cloud_sync(self, topic: str, payload_text: str, qos: int, retain: bool) -> bool:
        if not self.cloud_connected.is_set():
            return False
        info = self.cloud_client.publish(topic, payload_text.encode("utf-8"), qos=qos, retain=retain)
        if info.rc != mqtt.MQTT_ERR_SUCCESS:
            LOG.warning("cloud publish rejected topic=%s rc=%s", topic, info.rc)
            return False
        try:
            info.wait_for_publish(timeout=self.config.publish_timeout_seconds)
        except RuntimeError as exc:
            LOG.warning("cloud publish wait failed topic=%s error=%s", topic, exc)
            return False
        return bool(info.is_published())

    def _flush_loop(self) -> None:
        while not self.stop_event.is_set():
            if not self.cloud_connected.is_set():
                self.stop_event.wait(self.config.flush_interval_seconds)
                continue

            batch = self.queue.fetch_batch(self.config.batch_size)
            if not batch:
                self.stop_event.wait(self.config.flush_interval_seconds)
                continue

            for item in batch:
                if self.stop_event.is_set() or not self.cloud_connected.is_set():
                    break
                ok = self._publish_cloud_sync(item.topic, item.payload_json, item.qos, item.retain)
                if ok:
                    self.queue.delete(item.id)
                    LOG.info("flushed row=%s topic=%s pending=%s", item.id, item.topic, self.queue.count())
                else:
                    self.queue.mark_failed(item.id, "publish_failed")
                    LOG.warning("flush stopped at row=%s; will retry later", item.id)
                    break


def parse_args(argv: Iterable[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="YoloHome MQTT Store-and-Forward Gateway")
    parser.add_argument("--config", type=Path, default=Path("config.json"), help="Path to JSON config")
    parser.add_argument("--log-level", default="INFO", help="DEBUG, INFO, WARNING, ERROR")
    return parser.parse_args(argv)


def main(argv: Iterable[str] | None = None) -> int:
    args = parse_args(argv)
    logging.basicConfig(
        level=getattr(logging, args.log_level.upper(), logging.INFO),
        format="%(asctime)s %(levelname)s %(name)s: %(message)s",
    )

    config = GatewayConfig.from_file(args.config)
    gateway = StoreForwardGateway(config)

    def handle_stop(_signum, _frame) -> None:
        LOG.info("stopping gateway")
        gateway.stop()

    signal.signal(signal.SIGINT, handle_stop)
    signal.signal(signal.SIGTERM, handle_stop)

    try:
        gateway.run()
    finally:
        gateway.stop()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
