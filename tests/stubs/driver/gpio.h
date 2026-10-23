#pragma once
using gpio_num_t = int;
enum { GPIO_MODE_INPUT, GPIO_MODE_OUTPUT_OD };
int gpio_set_level(gpio_num_t pin, unsigned level);
int gpio_get_level(gpio_num_t pin);
int gpio_set_direction(gpio_num_t pin, int mode);
