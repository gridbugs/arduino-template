#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <avr/io.h>
#include "util.h"
#include "uart_bitbanged.h"
#include "adc.h"
#include "timer.h"

#define COUNT_MASK (BIT(0) | BIT(1) | BIT(2) | BIT(3) | BIT(4))

#define NUM_CHANNELS 4
#define MAX_NUM_STEPS 32

typedef struct {
    bool sequence[MAX_NUM_STEPS];
} channel_t;

void set_count_leds(uint8_t count) {
    uint8_t count_bits =
        ((count & BIT(0)) << 4) |
        ((count & BIT(1)) << 2) |
        (count & BIT(2)) |
        ((count & BIT(3)) >> 2) |
        ((count & BIT(4)) >> 4);
    PORTD &= ~COUNT_MASK;
    PORTD |= count_bits;
}

uint32_t get_delay(void) {
    uint32_t tempo_adc = (uint32_t)ADC_read(0);
    return (4096 + 128) - tempo_adc;
}

uint8_t get_num_steps(void) {
    uint16_t num_steps_adc = ((uint16_t)ADC_read(1));
    return (uint8_t)(num_steps_adc >> 7) + 1;
}

bool get_clear_button(void) {
    return (PINB & BIT(5)) == 0;
}

#define NUM_STEPS_DISPLAY_DELAY 4000

int main(void) {
    ADC_init(0xFF);
    USART0_bitbanged_init();

    channel_t channels[NUM_CHANNELS] = { 0 };

    uint8_t count = 0;

    DDRD |= COUNT_MASK;
    DDRC |= BIT(7);

    DDRD &= ~(BIT(5) | BIT(6) | BIT(7));
    DDRB &= ~(BIT(0) | BIT(5));
    DDRB |= BIT(1) | BIT(2) | BIT(3) | BIT(4);
    PORTD |= BIT(5) | BIT(6) | BIT(7);
    PORTB |= BIT(0) | BIT(5);


    uint32_t cycles_since_last_tick = 0;
    uint8_t prev_num_steps = get_num_steps();
    uint32_t num_steps_display_countdown = 0;
    uint32_t delay = get_delay();
    uint32_t global_count = 0;
    while (1) {
        bool clear = get_clear_button();

        if (global_count % 512 == 0) {
            delay = get_delay();
        }
        global_count++;

        if (!(PIND & BIT(5))) {
            channels[0].sequence[count] = !clear;
        }
        if (!(PIND & BIT(6))) {
            channels[1].sequence[count] = !clear;
        }
        if (!(PIND & BIT(7))) {
            channels[2].sequence[count] = !clear;
        }
        if (!(PINB & BIT(0))) {
            channels[3].sequence[count] = !clear;
        }

        uint32_t channel_0_threshold = ((uint32_t)ADC_read(2) * delay) / 4096;
        uint32_t channel_1_threshold = ((uint32_t)ADC_read(3) * delay) / 4096;
        uint32_t channel_2_threshold = ((uint32_t)ADC_read(4) * delay) / 4096;
        uint32_t channel_3_threshold = ((uint32_t)ADC_read(5) * delay) / 4096;

        if (channels[0].sequence[count] && (cycles_since_last_tick < channel_0_threshold)) {
            PORTB |= BIT(4);
        } else {
            PORTB &= ~BIT(4);
        }

        if (channels[1].sequence[count] && (cycles_since_last_tick < channel_1_threshold)) {
            PORTB |= BIT(3);
        } else {
            PORTB &= ~BIT(3);
        }

        if (channels[2].sequence[count] && (cycles_since_last_tick < channel_2_threshold)) {
            PORTB |= BIT(2);
        } else {
            PORTB &= ~BIT(2);
        }

        if (channels[3].sequence[count] && (cycles_since_last_tick < channel_3_threshold)) {
            PORTB |= BIT(1);
        } else {
            PORTB &= ~BIT(1);
        }

        uint8_t num_steps = get_num_steps();
        if (prev_num_steps != num_steps) {
            prev_num_steps = num_steps;
            num_steps_display_countdown = NUM_STEPS_DISPLAY_DELAY;
        }
        if (cycles_since_last_tick >= delay) {
            count += 1;
            if (count >= num_steps) {
                count = 0;
            }
            cycles_since_last_tick = 0;
        } else {
            cycles_since_last_tick += 1;
        }
        if (num_steps_display_countdown > 0) {
            num_steps_display_countdown--;
            set_count_leds(num_steps);
        } else {
            set_count_leds(count);
        }
    }

    return 0;
}
