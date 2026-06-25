#include "input.h"

#include "driver/gpio.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#define DEBOUNCE_DELAY_US 200000ULL

static void df_hdlr(void) {
    (void) 0;
    printf("default handler :D\n");
}

/* Define button pins, and hook them up to an array. */
button_t button_arr[BUTTON_ARR_SIZE] = {
    {UP_BUTTON, df_hdlr},
    {DOWN_BUTTON, df_hdlr},
    {SELECT_BUTTON, df_hdlr},
    {CANCEL_BUTTON, df_hdlr}
};

static const gpio_num_t up_pin = GPIO_NUM_27;
static const gpio_num_t down_pin = GPIO_NUM_26;
static const gpio_num_t select_pin = GPIO_NUM_25;
static const gpio_num_t cancel_pin = GPIO_NUM_33;

/* FreeRTOS message queue between GPIO ISR and button task handler. */
static QueueHandle_t gpio_evt_queue = NULL;

static volatile uint64_t last_isr_time = 0;
static void gpio_isr_handler(void *arg) {
    uint64_t now = esp_timer_get_time();    // Button debouncing
    if (now - last_isr_time > DEBOUNCE_DELAY_US) {
        uint32_t input_code = (uint32_t) arg;
        xQueueSendFromISR(gpio_evt_queue, &input_code, NULL);
        last_isr_time = now;
    }
}

static void button_handler_task(void *arg) {
    uint32_t input_code;
    while (1) {
        /* Blocks until a message is received. */
        if (xQueueReceive(gpio_evt_queue, &input_code, portMAX_DELAY)) {
            /* Loop instead of raw index to not assume fixed ordering. */ 
            for (int i = 0; i < BUTTON_ARR_SIZE; i++) {
                if (button_arr[i].code == input_code) {
                    button_arr[i].button_cb();
                }
            }
        }
    }
};

void input_module_init(void) {
    // Configure the buttons
    gpio_reset_pin(up_pin);
    gpio_set_direction(up_pin, GPIO_MODE_INPUT);
    gpio_pullup_en(up_pin);
    gpio_set_intr_type(up_pin, GPIO_INTR_POSEDGE);
    
    gpio_reset_pin(down_pin);
    gpio_set_direction(down_pin, GPIO_MODE_INPUT);
    gpio_pullup_en(down_pin);
    gpio_set_intr_type(down_pin, GPIO_INTR_POSEDGE);
    
    gpio_reset_pin(select_pin);
    gpio_set_direction(select_pin, GPIO_MODE_INPUT);
    gpio_pullup_en(select_pin);
    gpio_set_intr_type(select_pin, GPIO_INTR_POSEDGE);

    gpio_reset_pin(cancel_pin);
    gpio_set_direction(cancel_pin, GPIO_MODE_INPUT);
    gpio_pullup_en(cancel_pin);
    gpio_set_intr_type(cancel_pin, GPIO_INTR_POSEDGE);

    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    xTaskCreate(button_handler_task, "button_handler_task", 2048, NULL, 10,
        NULL);

    /* Install the driver's GPIO ISR handler service, which allows per-pin
        GPIO interrupt handlers. This function provides a global GPIO ISR and
        individual pin handlers are registered via the gpio_isr_handler_add()
        function. */
    gpio_install_isr_service(0);
    gpio_isr_handler_add(up_pin, gpio_isr_handler, (void*) UP_BUTTON); 
    gpio_isr_handler_add(down_pin, gpio_isr_handler, (void*) DOWN_BUTTON);
    gpio_isr_handler_add(select_pin, gpio_isr_handler, (void*) SELECT_BUTTON);
    gpio_isr_handler_add(cancel_pin, gpio_isr_handler, (void*) CANCEL_BUTTON);
}

void button_cb_reset_all(void) {
    for (int i = 0; i < BUTTON_ARR_SIZE; i++) {
        button_arr[i].button_cb = df_hdlr;
    }
}

void button_cb_register(enum button_code input_code, void(*fptr)(void)) {
    for (int i = 0; i < BUTTON_ARR_SIZE; i++) {
        if (button_arr[i].code == input_code) {
            button_arr[i].button_cb = fptr;
            break;
        }
    }
}
