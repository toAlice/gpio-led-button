#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <gpiod.h>
#include <stdbool.h>
#include <stdint.h>
#include <pthread.h>

struct gpiod_line *led;
struct gpiod_line *button;

bool flag;
bool led_on;

void *led_listener(void *_) {
    bool last = led_on;
    while (flag) {
        if (led_on != last) {
            last = led_on;
            gpiod_line_set_value(led, last);
        } 
        usleep(50000);
    }
    return NULL;
}

void *button_listener(void *_) {
    while (flag) {
        led_on = gpiod_line_get_value(button);
        usleep(50000);
    }
    return NULL;
}

int main(void) {
    // the names are chosen from the list printed by code from line 44 to line 67
    const char *LED_GPIO_NAME = "GPIO24";
    const char *BUTTON_GPIO_NAME = "GPIO22";

    flag = true;
    led_on = false;

// print all the lines
#if defined(PRINT_LINES) 
    int err = 0;

    // for raspberry pi 4B there're 2 gpiochips. 
    // change this number if the number on your device is not 2.
    for (int i = 0; i < 2; ++i) {
        char name[16];
        sprintf(name, "gpiochip%d", i);
        struct gpiod_chip *chip0 = gpiod_chip_open_by_name(name);
        printf("%s: %d\n", name, gpiod_chip_num_lines(chip0));
        struct gpiod_line_bulk bulk0 = GPIOD_LINE_BULK_INITIALIZER;
        err = gpiod_chip_get_all_lines(chip0, &bulk0);
        if (err) {
            char errmsg[64];
            sprintf(errmsg, "Error getting all %s lines\n", name);
            fprintf(stderr, errmsg);
            return EXIT_FAILURE;
        }
        for (int i = 0; i < bulk0.num_lines; ++i) {
            const char *name = gpiod_line_name(bulk0.lines[i]);
            printf("    %d. %s\n", i, name);
        }
    }
#endif

    // see https://pinout.xyz for more details
    // LED+ at GPIO24 (pin 18 on rpi4) 
    // LED- at Ground (pin 20 on rpi4)
    led = gpiod_line_find(LED_GPIO_NAME);
    if (!led) {
        fprintf(stderr, "Error finding led line\n");
        return EXIT_FAILURE;
    }
    if (gpiod_line_request_output(led, "led-gpio", 0) < 0) {
        fprintf(stderr, "Error requesting led line");
        return EXIT_FAILURE;
    }
    
    // GPIO22 (pin 15 on rpi4) and 3v3 Power (pin 17 on rpi4)
    button = gpiod_line_find(BUTTON_GPIO_NAME);
    if (!button) {
        fprintf(stderr, "Error finding button line\n");
        return EXIT_FAILURE;
    }
    if (gpiod_line_request_input(button, "switch-gpio") < 0) {
        fprintf(stderr, "Error requesting button line");
        return EXIT_FAILURE;
    }
    
    pthread_t led_thread;
    pthread_t button_thread;
    if (pthread_create(&led_thread, NULL, led_listener, NULL)
        || pthread_create(&button_thread, NULL, button_listener, NULL)) {
        fprintf(stderr, "Error creating thread\n");
        return EXIT_FAILURE;
    }
     
    puts("Press Enter to Exit");
    char enter;
    do {
        enter = getchar();
    } while (enter != '\n');

    flag = false;

    // wait for the threads to finish
    if (pthread_join(led_thread, NULL)
        || pthread_join(button_thread, NULL)) {
        fprintf(stderr, "Error joining thread\n");
        return EXIT_FAILURE;
    }

    // free the lines
    gpiod_line_release(led);
    gpiod_line_release(button);

    return 0;
}

