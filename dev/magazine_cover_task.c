#include "magazine_cover_task.h"
#include "hal.h"
#include "canBusProcess.h"
#include "dbus.h"
#include "keyboard.h"
static PWMConfig pwm8cfg = {
        1000000,   /* 1MHz PWM clock frequency.   */
        20000,      /* Initial PWM period 20ms.       */
        NULL,
        {
                {PWM_OUTPUT_DISABLED, NULL},
                {PWM_OUTPUT_ACTIVE_HIGH, NULL},
                {PWM_OUTPUT_ACTIVE_HIGH, NULL},
                {PWM_OUTPUT_DISABLED, NULL}
        },
        0,
        0
};
Gimbal_Send_Dbus_canStruct* PRC;
// const int LEFTCOVER = 0; // D
// const int RIGHTCOVER = 1; // C

static bool internalState = false; // True = open; False = close;
static int* bitmap_for_magCover;
static bool Q_press = false;
const int LEFTCOVER = 1; // D
const int RIGHTCOVER = 2; // C



// void magCoverClose(void){
//     pwmStop(&PWMD5);
//     pwmStart(&PWMD5,&pwm5cfg);
//     pwmEnableChannel(&PWMD5, LEFTCOVER, PWM_PERCENTAGE_TO_WIDTH(&PWMD5, 5000));
//     pwmEnableChannel(&PWMD5, RIGHTCOVER, PWM_PERCENTAGE_TO_WIDTH(&PWMD5, 5));
// }

// void magCoverOpen(void){
//     pwmStop(&PWMD5);
//     pwmStart(&PWMD5,&pwm5cfg);
//     pwmEnableChannel(&PWMD5, LEFTCOVER, PWM_PERCENTAGE_TO_WIDTH(&PWMD5, 500));
//     pwmEnableChannel(&PWMD5, RIGHTCOVER, PWM_PERCENTAGE_TO_WIDTH(&PWMD5, 1000));
// }

void magCoverToggle(void){
  if(!internalState){
    magCoverOpen();
    internalState = true;
  }else{
    magCoverClose();
    internalState = false;

  }
}

void magCoverClose(void){

//    pwmStop(&PWMD8);
//    pwmStart(&PWMD8,&pwm8cfg);
    // if(!internalState){
      pwmEnableChannel(&PWMD8, LEFTCOVER, PWM_PERCENTAGE_TO_WIDTH(&PWMD8, 5000));
      pwmEnableChannel(&PWMD8, RIGHTCOVER, PWM_PERCENTAGE_TO_WIDTH(&PWMD8, 5));
    // }
}

void magCoverOpen(void){
//    pwmStop(&PWMD8);
//    pwmStart(&PWMD8,&pwm8cfg);
  // if(internalState){
    pwmEnableChannel(&PWMD8, LEFTCOVER, PWM_PERCENTAGE_TO_WIDTH(&PWMD8, 500)); //800
    pwmEnableChannel(&PWMD8, RIGHTCOVER, PWM_PERCENTAGE_TO_WIDTH(&PWMD8, 1000)); //750
  // }


}


static THD_WORKING_AREA(magazine_cover_wa, 2048);
static THD_FUNCTION(magazine_cover, p)
{

  (void*)p;
  chRegSetThreadName("Magazine_cover");
  PRC = can_get_sent_dbus();
  uint32_t tick = chVTGetSystemTimeX();
  while(!chThdShouldTerminateX())
  {
    tick += US2ST(Maga_UPDATE_PERIOD_US);
    if(tick > chVTGetSystemTimeX())
      chThdSleepUntil(tick);
    else
    {
      tick = chVTGetSystemTimeX();
    }
    if(bitmap_for_magCover[KEY_Q]){
      if(!Q_press){
        magCoverToggle();
      }
      Q_press = true;

    }
    else{
      Q_press = false;
    }

    // if(PRC->s1 == MI){
    //   magCoverOpen();
    // }
    // else{
    //   magCoverClose();
    // }


  }
}


void pwm_magazine_cover_init(void)
{
    void pwm_config(PWMDriver *pwmp, const PWMConfig *config,int p){
        pwmStop(pwmp);
        pwmStart(pwmp,config);
        
        pwmEnableChannel(pwmp, 1, PWM_PERCENTAGE_TO_WIDTH(pwmp, p));
        pwmEnableChannel(pwmp, 2, PWM_PERCENTAGE_TO_WIDTH(pwmp, p));
        pwmEnableChannel(pwmp, 3, PWM_PERCENTAGE_TO_WIDTH(pwmp, p));
    }
    pwmStart(&PWMD8,&pwm8cfg);

    bitmap_for_magCover = Bitmap_get();

    magCoverClose();
    chThdCreateStatic(magazine_cover_wa, sizeof(magazine_cover_wa),
                               NORMALPRIO, magazine_cover, NULL);
    // chThdSleepSeconds(1);
}
