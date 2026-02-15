#include <stdio.h>
#include <stdint.h>
#include <avr/io.h>
#include "util.h"
#include "uart.h"
#include "adc.h"
#include "timer.h"

int main(void) {
    USART0_init();

    printf("Entering main loop...\n\r");

    while(1) {}

    return 0;
}
