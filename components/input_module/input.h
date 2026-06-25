#ifndef INPUT_H
#define INPUT_H

#define BUTTON_ARR_SIZE 4
enum button_code {UP_BUTTON, DOWN_BUTTON, SELECT_BUTTON, CANCEL_BUTTON};

typedef struct button {
    enum button_code code;
    void (*button_cb)(void); 
} button_t;

/* TODO: Set up the button array, with a public API to hook callbacks. */
extern button_t button_arr[];

void input_module_init(void);
void button_cb_reset_all(void);
void button_cb_register(enum button_code code, void (*button_cb)(void));

#endif
