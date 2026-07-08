#include <stdio.h>

#include "ServoMotor.h"
#include "UltrasonicSensor.h"
#include "Potentiometer.h"

void allTask(void)
{
    start_servomotor_task();
    start_ultrasonic_task();
    start_potentiometer_task();
}

void app_main(void)
{
    //allTask();
}
