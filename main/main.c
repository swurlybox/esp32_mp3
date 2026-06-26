#include <stdio.h>
#include "input.h"
#include "fs.h"

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"

static const gpio_num_t led_pin = GPIO_NUM_32;
static const uint32_t sleep_time_ms = 1000;

int app_main() {
    input_module_init();
    fs_init();
    
    uint32_t led_state = 0;
    gpio_reset_pin(led_pin);
    gpio_set_direction(led_pin, GPIO_MODE_OUTPUT);

    while (1) {
        led_state = !led_state;
        gpio_set_level(led_pin, led_state);
        vTaskDelay(sleep_time_ms / portTICK_PERIOD_MS);
    }

    return 0;
}
