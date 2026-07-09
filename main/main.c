#include <stdio.h>

#include "ServoMotor.h"
#include "UltrasonicSensor.h"
#include "Potentiometer.h"
#include "Alarm_Buzzer.h"
#include "Communication.h"
#include "st7735_lcd.h"

void allTask(void)
{
    start_servomotor_task();
    start_ultrasonic_task();
    start_potentiometer_task();
    start_alarm_task();
    start_st7735_task();
}

void app_main(void)
{
    init_communication();
    allTask();
}
