/*
 * imu_temp.c
 *
 *  Created on: 3 Jan, 2017
 *      Author: ASUS
 */
#include "ch.h"
#include "hal.h"
#include "mpu6500.h"
#include "imu_temp.h"

extern PWMDriver PWMD12;

#define TEMP_THRESHOLD 40
#define TEMPERATURE_UPDATE_FREQ 200000U
#define TEMPERATURE_UPDATE_PERIOD_US 5U //1000000U/TEMPERATURE_UPDATE_FREQ

//static struct tempPID_Tag{
//  int Kp;
//  int Ki;
//  int Kd;
//  float Error_Integral;
//  float Previous_Error;
//  int PID_Value;
//}*tempStruct, tempPID;

TPIDStruct tempPID1;

static const TPIDConfigStruct tpid1_conf = {
                                            3500,   //Kp
                                            1000,      //Ki
                                            0       //Kd
};

pTPIDStruct TPID_get(void){
  return &tempPID1;
}

void tempPID_Init(pTPIDStruct tempPID, const TPIDConfigStruct* const tpid_conf){
  tempPID->Kp = tpid_conf->Kp;
  tempPID->Ki = tpid_conf->Ki;
  tempPID->Kd = tpid_conf->Kd;
  tempPID->Error_Integral = 0.0f;
  tempPID->Previous_Error = 0.0f;
  tempPID->PID_Value = 0;
}

int tempPID_Update(pTPIDStruct tempPID, PIMUStruct pIMU){
  float Error = TEMP_THRESHOLD - pIMU->temperature;
  tempPID->Error_Integral = tempPID->Error_Integral + Error;
  if(tempPID->Error_Integral > 2500)
    tempPID->Error_Integral = 2500;
  else if(tempPID->Error_Integral < -2500)
    tempPID->Error_Integral = -2500;

  float Error_Derivative = Error - tempPID->Previous_Error;
  tempPID->Previous_Error = Error;

  int PID_Output = (int)((tempPID->Kp * Error) + (tempPID->Ki * tempPID->Error_Integral) + (tempPID->Kd * Error_Derivative));

  if(PID_Output > 10000)
    PID_Output= 10000;
  else if(PID_Output < 0)
    PID_Output = 0;

  tempPID->PID_Value = PID_Output;

  return PID_Output;
}

static THD_WORKING_AREA(Temperature_thread_wa, 4096);
static THD_FUNCTION(Temperature_thread, p)
{
  chRegSetThreadName("IMU Temperature Control");

  (void)p;

  PIMUStruct pIMU = imu_get();
  pTPIDStruct tempPID = TPID_get();
  tempPID_Init(tempPID, &tpid1_conf);
  uint32_t tick = chVTGetSystemTimeX();

  pwmEnableChannel(&PWMD3, 1, PWM_PERCENTAGE_TO_WIDTH(&PWMD3, 0));

//  while(chVTTimeElapsedSinceX(tick) < S2ST(10)){
//    if(pIMU->temperature > 40.0){
//      pwmEnableChannel(&PWMD3, 1, PWM_PERCENTAGE_TO_WIDTH(&PWMD3, 0));
//    }else{
//      pwmEnableChannel(&PWMD3, 1, PWM_PERCENTAGE_TO_WIDTH(&PWMD3, 10000));
//    }
//   chThdSleep(MS2ST(10));
//  }

//  tick = chVTGetSystemTimeX();

  while(true)
  {
    tick += S2ST(TEMPERATURE_UPDATE_PERIOD_US);
    if(chVTGetSystemTimeX() < tick)
      chThdSleepUntil(tick);
    else
    {
      tick = chVTGetSystemTimeX();
      pIMU->errorCode |= IMU_LOSE_FRAME;
    }

    imuGetData(pIMU);
    int PWM_Output = tempPID_Update(tempPID, pIMU);
    // Safety Measure if TEMP > 47 stop the output

//    if(PWMD12.state == PWM_STOP){
////      pwmEnableChannel(&PWMD3, 1, PWM_PERCENTAGE_TO_WIDTH(&PWMD3, 9000));
//    }else{
//      if(pIMU->temperature > 47){
////      pwmEnableChannel(&PWMD3, 1, PWM_PERCENTAGE_TO_WIDTH(&PWMD3, 0));
//        pwmDisableChannel(&PWMD3, 1);
//      }
//      else{
        pwmEnableChannel(&PWMD3, 1, PWM_PERCENTAGE_TO_WIDTH(&PWMD3, PWM_Output));
//      }
//    }
  }
}

void tempControllerInit(void){
  pwm3init();
  chThdCreateStatic(Temperature_thread_wa, sizeof(Temperature_thread_wa),
    NORMALPRIO,
                      Temperature_thread, NULL);
}
