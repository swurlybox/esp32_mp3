#ifndef INPUT_H
#define INPUT_H

enum button_code {UP_BUTTON, DOWN_BUTTON, SELECT_BUTTON, CANCEL_BUTTON};

int input_module_init(void);
void button_cb_reset_all(void);
void button_cb_register(enum button_code code, void (*button_cb)(void));

#endif
