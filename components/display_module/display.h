#ifndef DISPLAY_H
#define DISPLAY_H

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include <stdint.h>

/* Any request to update the display must call
    xSemaphoreGive(display_semaphore) */
extern SemaphoreHandle_t display_semaphore;
void display_init();

/* We want to present a graphics API to draw to this display. 

    We could probably just port over the graphics API we wrote for bare-metal

    Requirements:
        - want to be able to render text to the screen at specific locations
        - draw some elements for the filesystem navigation state.
*/

/* Draw a string of characters to the frame-buffer; specifying its location
    via row {0-7}, start {0-127}; and the length of the string via len. */
void graphics_draw_line_chars(char *str, uint8_t row, uint8_t start,
     uint8_t len);

void graphics_clear();

#endif
