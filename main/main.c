#include <stdio.h>

#include "ServoMotor.h"
#include "UltrasonicSensor.h"
#include "Potentiometer.h"
#include "Alarm_Buzzer.h"

void allTask(void)
{
    start_servomotor_task();
    start_ultrasonic_task();
    start_potentiometer_task();
    start_alarm_task();
}

void app_main(void)
{
    //allTask();
    start_alarm_task();
}
