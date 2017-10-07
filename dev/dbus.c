#include "ch.h"
#include "hal.h"

#include "dbus.h"

static uint8_t rxbuf[DBUS_BUFFER_SIZE];
static RC_Ctl_t RC_Ctl;
static thread_reference_t uart_host_thread_handler = NULL;
static uint8_t rx_start_flag = 1;

//Interrupt function
static void decryptDBUS(void)
{
  RC_Ctl.rc.channel0 = ((rxbuf[0]) | (rxbuf[1]<<8)) & 0x07FF;
	RC_Ctl.rc.channel1 = ((rxbuf[1]>>3) | (rxbuf[2]<<5)) & 0x07FF;
	RC_Ctl.rc.channel2 = ((rxbuf[2]>>6) | (rxbuf[3]<<2) | ((uint32_t)rxbuf[4]<<10)) & 0x07FF;
	RC_Ctl.rc.channel3 = ((rxbuf[4]>>1) | (rxbuf[5]<<7)) & 0x07FF;
  RC_Ctl.rc.s1  = ((rxbuf[5] >> 4)& 0x000C) >> 2;                         //!< Switch left
  RC_Ctl.rc.s2  = ((rxbuf[5] >> 4)& 0x0003);
}


/*
 * This callback is invoked when a receive buffer has been completely written.
 */
static void rxend(UARTDriver *uartp) {

  if(rx_start_flag)
  {
    chSysLockFromISR();
    palTogglePad(GPIOF,GPIOF_LED_G);
    decryptDBUS();
    chThdResumeI(&uart_host_thread_handler, MSG_OK);
    chSysUnlockFromISR();
  }
  else
    rx_start_flag = 1;
}

/*
 * UART driver configuration structure.
 */
static UARTConfig uart_cfg = {
  NULL,NULL,rxend,NULL,NULL,
  100000,
  USART_CR1_PCE,
  0,
  0
};

#define  DBUS_INIT_WAIT_TIME_MS      4U
#define  DBUS_WAIT_TIME_MS          10U
static THD_WORKING_AREA(uart_dbus_thread_wa, 512);
static THD_FUNCTION(uart_dbus_thread, p)
{
  (void)p;
  chRegSetThreadName("uart dbus receiver");

  uartStart(UART_DBUS, &uart_cfg);
  dmaStreamRelease(*UART_DBUS.dmatx);

  size_t rx_size;

  while(!chThdShouldTerminateX() && !rx_start_flag)
  {
    uartStartReceive(UART_DBUS, DBUS_BUFFER_SIZE, rxbuf);
    chThdSleepMilliseconds(DBUS_INIT_WAIT_TIME_MS);
    uartStopReceive(UART_DBUS);
  }

  while(!chThdShouldTerminateX())
  {
    uartStartReceive(UART_DBUS, DBUS_BUFFER_SIZE, rxbuf);

    chSysLock();
    chThdSuspendS(&uart_host_thread_handler);
    chSysUnlock();
  }
}

RC_Ctl_t* RC_get(void)
{
  return &RC_Ctl;
}

void RC_Init(void)
{
	RC_Ctl.rc.channel0 = 5;
	RC_Ctl.rc.channel1 = 5;
	RC_Ctl.rc.channel2 = 5;
	RC_Ctl.rc.channel3 = 5;

  chThdCreateStatic(uart_dbus_thread_wa, sizeof(uart_dbus_thread_wa),
                    NORMALPRIO + 7,
                    uart_dbus_thread, NULL);
}
