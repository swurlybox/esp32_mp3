#include "display.h"

#include <stdio.h>
#include "driver/i2c_master.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"

#define I2C_BUS_PORT 0

/* Configure according to LCD specs */
#define LCD_PIXEL_CLOCK_HZ      (400 * 1000)
#define PIN_NUM_SDA             4
#define PIN_NUM_SCL             2
#define PIN_NUM_RST             -1
#define I2C_HW_ADDR             0x3C

/* Pixel numbeer of horizontal and vertical */
#define LCD_H_RES               128
#define LCD_V_RES               64

#define LCD_CMD_BITS            8
#define LCD_PARAM_BITS          8

/* LVGL related configurations */
#define LVGL_TICK_PERIOD_MS     5
#define LVGL_TASK_STACK_SIZE    (4 * 1024)
#define LVGL_TASK_PRIORITY      2
#define LVGL_PALETTE_SIZE       8
#define LVGL_TASK_MAX_DELAY_MS  500
#define LVGL_TASK_MIN_DELAY_MS  1000 / CONFIG_FREERTOS_HZ

static uint8_t oled_buffer[LCD_H_RES * LCD_V_RES / 8];  /* 128 * 64 bits */

void display_init() {
    /* Initialize the SSD1306 driver. */
    printf("Initialize I2C bus\n");
    i2c_master_bus_handle_t i2c_bus = NULL;
    i2c_master_bus_config_t bus_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .i2c_port = I2C_BUS_PORT,
        .sda_io_num = PIN_NUM_SDA,
        .scl_io_num = PIN_NUM_SCL,
        .flags.enable_internal_pullup = true
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &i2c_bus));

    printf("Install panel IO\n");
    esp_lcd_panel_io_handle_t io_handle = NULL; 
    esp_lcd_panel_io_i2c_config_t io_config = {
        .dev_addr = I2C_HW_ADDR,
        .scl_speed_hz = LCD_PIXEL_CLOCK_HZ,
        .control_phase_bytes = 1,
        .lcd_cmd_bits = LCD_CMD_BITS,
        .lcd_param_bits = LCD_PARAM_BITS,
        .dc_bit_offset = 6
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c(i2c_bus, &io_config, &io_handle));

    printf("Install SSD1306 panel driver\n");
    esp_lcd_panel_handle_t panel_handle = NULL;
    esp_lcd_panel_dev_config_t panel_config = {
        .bits_per_pixel = 1,
        .reset_gpio_num = PIN_NUM_RST
    };
    esp_lcd_panel_ssd1306_config_t ssd1306_config = {
        .height = LCD_V_RES
    };
    panel_config.vendor_config = &ssd1306_config;
    ESP_ERROR_CHECK(esp_lcd_new_panel_ssd1306(io_handle, &panel_config, 
        &panel_handle));

    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

    /* Initialize LVGL (optional if you choose to use LVGL) */

    /* Create a task that periodically checks for display updates, (can 
        use a message queue blocking scheme similar to the button inputs. )
        and sends the frame buffer to the driver when needed. */

    /* TEST: Draw a dummy bitmap to the panel
        TODO: Save reference to panel_handle somewhere, such that API calls
            to update the display can reference the panel_handle.

     */

    for (int i = 0; i < 1024; i++) {
        oled_buffer[i] = (uint8_t) ((i + 1) % 256);
    }
    esp_lcd_panel_draw_bitmap(panel_handle, 0, 0, 128, 64, oled_buffer);

};
