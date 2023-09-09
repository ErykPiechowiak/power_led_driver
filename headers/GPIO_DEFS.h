#ifndef GPIO_DEFS_H_
#define GPIO_DEFS_H_

#include "pico/stdlib.h"
#include "hardware/uart.h"

//UART
#define UART_ID uart0
#define UART_TX_PIN 0
#define UART_RX_PIN 1
#define BAUD_RATE 9600
#define DATA_BITS 8
#define STOP_BITS 1
#define PARITY    UART_PARITY_NONE
#define UART0_IRQ 20
#define LED_PIN 22

//PWM
#define PWM_PIN 16

//Flash
#define FLASH_TARGET_OFFSET (PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE)
#define BRIGHTNESS_LVL_FLASH_OFFSET (PICO_FLASH_SIZE_BYTES - 2*FLASH_SECTOR_SIZE)

#endif /*GPIO_DEFS_H*/