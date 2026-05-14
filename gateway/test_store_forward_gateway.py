import tempfile
import unittest
from pathlib import Path

from gateway.store_forward_gateway import (
    OutboundQueue,
    cloud_retain_for_topic,
    should_replace_pending_state,
    should_store_topic,
)


class OutboundQueueTest(unittest.TestCase):
    def test_queue_persists_across_instances(self):
        with tempfile.TemporaryDirectory() as tmp:
            db_path = Path(tmp) / "gateway_buffer.db"
            queue = OutboundQueue(db_path)
            row_id = queue.enqueue("yolohome/device/yolo_uno_01/telemetry", '{"ts":1}', qos=1)
            queue.close()

            reopened = OutboundQueue(db_path)
            rows = reopened.fetch_batch(10)
            reopened.close()

        self.assertEqual(row_id, 1)
        self.assertEqual(len(rows), 1)
        self.assertEqual(rows[0].payload_json, '{"ts":1}')

    def test_delete_after_successful_publish(self):
        with tempfile.TemporaryDirectory() as tmp:
            queue = OutboundQueue(Path(tmp) / "gateway_buffer.db")
            row_id = queue.enqueue("yolohome/device/yolo_uno_01/event", '{"eventType":"test"}')
            self.assertEqual(queue.count(), 1)

            queue.delete(row_id)
            self.assertEqual(queue.count(), 0)
            queue.close()

    def test_state_latest_policy_replaces_pending_state(self):
        with tempfile.TemporaryDirectory() as tmp:
            queue = OutboundQueue(Path(tmp) / "gateway_buffer.db")
            topic = "yolohome/device/yolo_uno_01/state"
            queue.enqueue(topic, '{"state":1}', replace_pending_for_topic=True)
            queue.enqueue(topic, '{"state":2}', replace_pending_for_topic=True)
            rows = queue.fetch_batch(10)
            queue.close()

        self.assertEqual(len(rows), 1)
        self.assertEqual(rows[0].payload_json, '{"state":2}')

    def test_mark_failed_increments_attempt_count(self):
        with tempfile.TemporaryDirectory() as tmp:
            queue = OutboundQueue(Path(tmp) / "gateway_buffer.db")
            row_id = queue.enqueue("yolohome/device/yolo_uno_01/telemetry", '{"ts":1}')
            queue.mark_failed(row_id, "network_error")
            rows = queue.fetch_batch(10)
            queue.close()

        self.assertEqual(rows[0].attempt_count, 1)


class TopicPolicyTest(unittest.TestCase):
    def test_availability_and_cmd_are_not_stored(self):
        self.assertFalse(should_store_topic("yolohome/device/yolo_uno_01/availability"))
        self.assertFalse(should_store_topic("yolohome/device/yolo_uno_01/cmd"))

    def test_uplink_data_topics_are_stored(self):
        self.assertTrue(should_store_topic("yolohome/device/yolo_uno_01/telemetry"))
        self.assertTrue(should_store_topic("yolohome/device/yolo_uno_01/event"))
        self.assertTrue(should_store_topic("yolohome/device/yolo_uno_01/state"))

    def test_state_latest_policy(self):
        self.assertTrue(should_replace_pending_state("yolohome/device/yolo_uno_01/state", "latest"))
        self.assertFalse(should_replace_pending_state("yolohome/device/yolo_uno_01/state", "replay_all"))

    def test_state_and_availability_are_retained_on_cloud(self):
        self.assertTrue(cloud_retain_for_topic("yolohome/device/yolo_uno_01/state", False))
        self.assertTrue(cloud_retain_for_topic("yolohome/device/yolo_uno_01/availability", False))
        self.assertFalse(cloud_retain_for_topic("yolohome/device/yolo_uno_01/telemetry", False))


if __name__ == "__main__":
    unittest.main()
