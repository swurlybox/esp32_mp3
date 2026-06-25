#include <stdio.h>

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"

/* Check out WROOM 32 Datasheet and cross-reference with the board's exposed
pins. D25 corresponds to GPIO25. */
static const gpio_num_t led_pin = GPIO_NUM_25;
static const uint32_t sleep_time_ms = 1000;

int app_main() {
    uint8_t led_state = 0;

    // Configure the GPIO
    gpio_reset_pin(led_pin);
    gpio_set_direction(led_pin, GPIO_MODE_OUTPUT);

    while (1) {
        led_state = !led_state;
        gpio_set_level(led_pin, led_state);
        printf("LED state %d\n", led_state);
        vTaskDelay(sleep_time_ms / portTICK_PERIOD_MS);
    }

    return 0;
}
