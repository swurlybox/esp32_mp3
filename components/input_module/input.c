#include "input.h"

#include "driver/gpio.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#define DEBOUNCE_DELAY_US 200000ULL

static const gpio_num_t button_pin = GPIO_NUM_12;
static volatile uint64_t last_isr_time = 0;

/* FreeRTOS structure for message passing? */
static QueueHandle_t gpio_evt_queue = NULL;

/* void(*gpio_isr_t)(void *) */
static void gpio_isr_handler(void *arg) {
    uint64_t now = esp_timer_get_time();
    if (now - last_isr_time > DEBOUNCE_DELAY_US) {
        uint32_t gpio_num = (uint32_t) arg;
        xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
        last_isr_time = now;
    }
}

/* Thread that handles button inputs. Blocks until some data is received
    from the queue. This prevents this thread from hogging CPU w/ a spin
    lock. */
static void button_handler_task(void *arg) {
    uint32_t io_num;
    while (1) {
        /* Blocks until a message is received. */
        if (xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
            printf("GPIO[%"PRIu32"] intr, val: %d\n", io_num, 
                gpio_get_level(io_num));
        
            /* TODO: Eventually want to call the 
                callbacks attached to each button. */


        }
    }
};

void input_module_init(void) {

    // Configure the BUTTON
    gpio_reset_pin(button_pin);
    gpio_set_direction(button_pin, GPIO_MODE_INPUT);
    gpio_pullup_en(button_pin);
    gpio_set_intr_type(button_pin, GPIO_INTR_POSEDGE);

    // TODO: Look these up
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    xTaskCreate(button_handler_task, "button_handler_task", 2048, NULL, 10,
        NULL);

    /* Install the driver's GPIO ISR handler service, which allows per-pin
        GPIO interrupt handlers. This function provides a global GPIO ISR and
        individual pin handlers are registered via the gpio_isr_handler_add()
        function. */
    gpio_install_isr_service(0);
    gpio_isr_handler_add(button_pin, gpio_isr_handler, (void*) button_pin);

}
