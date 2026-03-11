#include <FSM_SETTING.hpp>

void FSM_Main()
{
    if (State_FSM)
    {
        // FSM_Manual();
        FSM_Auto();
    }
    else
    {
        // FSM_Auto();
        FSM_Manual();
    }
    //State_FSM ? FSM_Auto() : FSM_Manual();
}

void FSM_Auto()
{
    // Serial.print(Value_SoilMoisture);
    if (Value_SoilMoisture <= 30)
    {
        pump_on();
        ledred_on();
    }
    if (Value_SoilMoisture > 30)
    {
        pump_off();
        ledgreen_on();
    }
    if (Value_Temperature >= 25)
    {
        controlFan();
    }
    if (Value_Light <= 20 && Value_Light >= 0)
    {
        ledwhite_on();
    }
    if (Value_Light > 20)
    {
        ledwhite_off();
    }
}

void FSM_Manual()
{   
    led_off();
    pump_control_manual(Pump1.value);
    fan_control_manual(Fan1.active);
    led_control_manual(Led1.active);
}