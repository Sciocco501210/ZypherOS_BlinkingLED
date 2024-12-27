/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>

/* 500 msec delay */
#define SLEEP_TIME_MS 500

/* LED and Button bindings */
#define LED1_NODE DT_ALIAS(led1)
#define LED2_NODE DT_ALIAS(led2)
#define LED3_NODE DT_ALIAS(led3)
#define LED4_NODE DT_ALIAS(led4)
#define LED5_NODE DT_ALIAS(led5)
#define LED6_NODE DT_ALIAS(led6)
#define LED7_NODE DT_ALIAS(led7)
#define LED8_NODE DT_ALIAS(led8)
#define BUTTON_NODE DT_ALIAS(sw0)

/* GPIO devices for LEDs and Button */
static const struct gpio_dt_spec leds[] = {
    GPIO_DT_SPEC_GET_OR(LED1_NODE, gpios, {0}),
    GPIO_DT_SPEC_GET_OR(LED2_NODE, gpios, {0}),
    GPIO_DT_SPEC_GET_OR(LED3_NODE, gpios, {0}),
    GPIO_DT_SPEC_GET_OR(LED4_NODE, gpios, {0}),
    GPIO_DT_SPEC_GET_OR(LED5_NODE, gpios, {0}),
    GPIO_DT_SPEC_GET_OR(LED6_NODE, gpios, {0}),
    GPIO_DT_SPEC_GET_OR(LED7_NODE, gpios, {0}),
    GPIO_DT_SPEC_GET_OR(LED8_NODE, gpios, {0}),
};

static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET_OR(BUTTON_NODE, gpios, {0});
static struct gpio_callback button_cb_data;

static bool is_wave = true;
static uint8_t pattern = 0b1;

/* Reverse bits utility */
static uint8_t reverse_bits(uint8_t bits)
{
    uint8_t reverse = 0;
    for (int i = 0; i < 8; i++) {
        reverse <<= 1;
        reverse |= (bits & 1);
        bits >>= 1;
    }
    return reverse;
}

/* Wave pattern generator */
static void wave_pattern(void)
{
    if (pattern == 0b10000000) {
        pattern = 0b1;
    } else {
        pattern <<= 1;
    }
}

/* Curtain pattern generator */
static void curtain_pattern(void)
{
    uint8_t current = pattern & 0b00001111;
    if (current == 0b1000) {
        current = 0b1;
    } else {
        current <<= 1;
    }
    pattern = reverse_bits(current) | current;
}

/* Button interrupt handler */
void button_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
    is_wave = !is_wave;
    pattern = 0b1;
}

/* Update LEDs based on pattern */
static void update_leds(void)
{
    for (size_t i = 0; i < ARRAY_SIZE(leds); i++) {
        if (!device_is_ready(leds[i].port)) {
            continue;
        }

        if (pattern & (1 << i)) {
            gpio_pin_set(leds[i].port, leds[i].pin, 1);
        } else {
            gpio_pin_set(leds[i].port, leds[i].pin, 0);
        }
    }
}

void main(void)
{
    printk("Starting LED wave/curtain pattern example\n");

    /* Configure LEDs */
    for (size_t i = 0; i < ARRAY_SIZE(leds); i++) {
        if (device_is_ready(leds[i].port)) {
            gpio_pin_configure_dt(&leds[i], GPIO_OUTPUT_INACTIVE);
        }
    }

    /* Configure Button */
    if (device_is_ready(button.port)) {
        gpio_pin_configure_dt(&button, GPIO_INPUT);
        gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_TO_ACTIVE);
        gpio_init_callback(&button_cb_data, button_pressed, BIT(button.pin));
        gpio_add_callback(button.port, &button_cb_data);
    }

    /* Main loop */
    while (1) {
        if (is_wave) {
            wave_pattern();
        } else {
            curtain_pattern();
        }
        update_leds();
        k_msleep(SLEEP_TIME_MS);
    }
}
