#include "display.h"

#include <stdio.h>
#include "driver/i2c_master.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#include "freertos/task.h"

#define I2C_BUS_PORT 0

/* Configure according to LCD specs */
#define LCD_PIXEL_CLOCK_HZ      (400 * 1000)
#define PIN_NUM_SDA             4
#define PIN_NUM_SCL             2
#define PIN_NUM_RST             -1
#define I2C_HW_ADDR             0x3C

/* Pixel number of horizontal and vertical */
#define LCD_H_RES               128
#define LCD_V_RES               64
#define MAX_ROWS                (LCD_V_RES / 8)
#define DISPLAY_BYTE_SIZE       (LCD_H_RES * LCD_V_RES / 8)

#define LCD_CMD_BITS            8
#define LCD_PARAM_BITS          8

#define DISPLAY_TASK_PRIORITY   9 /* 1 step lower from the input task */

/* Character Bitmap Data */
#define CHAR_BYTE_LEN   (5)
#define SUPPORTED_CHARS (48)

#define PERIOD_LOC      (26)
#define SPACE_LOC       (27)
#define QUESTION_LOC    (28)
#define EXCLAM_LOC      (29)
#define COMMA_LOC       (30)
#define APOST_LOC       (31)
#define DQUOT_LOC       (32)
#define FSLASH_LOC      (33)
#define COLON_LOC       (34)
#define ZERO_LOC        (35)
#define UNDERSCORE_LOC  (45)
#define RIGHT_ARR_LOC   (46)

#define UNSUPPORTED     (47)


typedef struct {
    uint8_t byte[CHAR_BYTE_LEN];
} display_char;

static const display_char arr[SUPPORTED_CHARS] = {
    #include "char_byte_map.txt"
};

static uint32_t dbi = 0;    /* index into a byte in the oled_buffer. */
static uint8_t oled_buffer[DISPLAY_BYTE_SIZE];  /* 128 * 64 bits */
static esp_lcd_panel_handle_t panel_handle = NULL;

static int find_index(char c);
static void draw_char(int bitmap_index);

/* I think a semaphore here is fine to synchronize the display updates
    with the display task. It's essentially an indicator that a display
    update needs to happen. TODO: make this extern available */
SemaphoreHandle_t display_semaphore = NULL;

static void display_task(void *arg) {
    printf("Starting display task\n");
    while(1) {
        /* Thread should yield here. */
        xSemaphoreTake(display_semaphore, portMAX_DELAY);
        printf("display_update triggered\n");
        /* TODO: If I2C transaction error, do try again */
        esp_lcd_panel_draw_bitmap(panel_handle, 0, 0, 128, 64, oled_buffer);
        graphics_clear();
    }
}

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
    panel_handle = NULL;    /* global */
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

    display_semaphore = xSemaphoreCreateBinary();
    xTaskCreate(display_task, "display_task", 4098, NULL,
        DISPLAY_TASK_PRIORITY, NULL);
};


void graphics_draw_line_chars(char *str, uint8_t row, uint8_t start, 
    uint8_t buflen) {
    char c;
    int char_index;

    if (row >= MAX_ROWS || start >= LCD_H_RES) {
        return;
    }

    /* So the first byte of oled_buffer in my original implementation
        was reserved, probably for sending an i2c command. But I think thats
        been abstracted away by esp-idf's LCD panel driver, so I think I can
        just use the first byte... */
    dbi = (row * LCD_H_RES) + start; 
    while (((c = *str) != '\0') && buflen-- > 0) {
        /* find_index and draw_char are used to get the bitmap data for
            each character. */
        char_index = find_index(c);

        /* don't want to wrap to next line, so just get out. */
        if ((LCD_H_RES - (dbi % LCD_H_RES)) < CHAR_BYTE_LEN) {
            break;
        }

        draw_char(char_index);
       
        /* db++ gives us a 1 bit margin between subsequent chars. */ 
        dbi++;
        str++;
    }
    dbi = 0;
}

static int find_index(char c) {
    int char_index = UNSUPPORTED;
    if ((c >= 'A' && c <= 'Z')) {
        char_index = c - 'A';
    }
    else if ((c >= 'a' && c <= 'z')) {
        char_index = c - 'a';
    }
    else if ((c >= '0' && c <= '9')) {
        char_index = ZERO_LOC + (c - '0');
    }
    else if ((c == '.')) {
        char_index = PERIOD_LOC;
    }
    else if ((c == ' ')) {
        char_index = SPACE_LOC;
    }
    else if ((c == '?')) {
        char_index = QUESTION_LOC;
    }
    else if ((c == '!')) {
        char_index = EXCLAM_LOC;
    }
    else if ((c == ',')) {
        char_index = COMMA_LOC;
    }
    else if ((c == '\'')) {
        char_index = APOST_LOC;
    }
    else if ((c == '"')) {
        char_index = DQUOT_LOC;
    }
    else if ((c == '/')) {
        char_index = FSLASH_LOC;
    }
    else if ((c == ':')) {
        char_index = COLON_LOC;
    }
    else if ((c == '_')) {
        char_index = UNDERSCORE_LOC;
    }
    else if ((c == '>')) {
        char_index = RIGHT_ARR_LOC;
    }
    return char_index;
}

static void draw_char(int index) {
    for (int i = 0; i < CHAR_BYTE_LEN && dbi < DISPLAY_BYTE_SIZE; i++) {    
        oled_buffer[dbi++] |= arr[index].byte[i];
    }   
}

void graphics_clear() {
    for (int i = 0; i < DISPLAY_BYTE_SIZE; i++) {
        oled_buffer[i] = 0;
    }
}
