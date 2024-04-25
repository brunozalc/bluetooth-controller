/*
 * LED blink with FreeRTOS
 */
#include "hardware/adc.h"
#include <FreeRTOS.h>
#include <math.h>
#include <queue.h>
#include <semphr.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <task.h>

#include "hc06.h"
#include "pico/stdlib.h"

/* List of IDs
B      - 0
Y      - 1
X      - 2
A      - 3
TR     - 4
TL     - 5
CJS X  - 6
CJS Y  - 7
MJS X  - 8
MJS Y  - 9 */

/* ------------------------------ Constants ------------------------------ */
#define DEBOUNCE_TIME 200

#define GAME_BTN_B_PIN 10
#define GAME_BTN_Y_PIN 11
#define GAME_BTN_X_PIN 12
#define GAME_BTN_A_PIN 13
#define R_TRIGGER_PIN 14 // a mudar
#define L_TRIGGER_PIN 15 // a mudar

#define R_JOYSTICK_SW_PIN 21 // a mudar
#define L_JOYSTICK_SW_PIN 20 // a mudar

#define MUX_A_CONTROL_PIN 16
#define MUX_ADC_PIN 28

#define DEAD_ZONE 180

/* ------------------------------ Data structures ------------------------------ */
typedef struct adc {
    int axis;
    int val;
} adc_t;

/* ------------------------------ Global variables ------------------------------ */
QueueHandle_t xQueueGameButton, xQueueJoyStick, xQueueBluetooth, xQueueJoyStickLeft;

/* ------------------------------ Utilities ------------------------------ */
bool has_debounced(uint32_t current_trigger, uint32_t last_trigger) {
    return current_trigger - last_trigger > DEBOUNCE_TIME;
}

void write_package(adc_t data) {
    int val = data.val;
    int msb = val >> 8;
    int lsb = val & 0xFF;

    uart_putc_raw(HC06_UART_ID, data.axis);
    uart_putc_raw(HC06_UART_ID, msb);
    uart_putc_raw(HC06_UART_ID, lsb);
    uart_putc_raw(HC06_UART_ID, -1);
}

/* ------------------------------ Callbacks ------------------------------ */
void game_btn_callback(uint gpio, uint32_t events) {
    uint pressed = 0;

    if (events == 0x4) { // fall edge
        if (gpio == GAME_BTN_B_PIN)
            pressed = 0;
        else if (gpio == GAME_BTN_Y_PIN)
            pressed = 1;
        else if (gpio == GAME_BTN_X_PIN)
            pressed = 2;
        else if (gpio == GAME_BTN_A_PIN)
            pressed = 3;
        else if (gpio == R_TRIGGER_PIN)
            pressed = 4;
        else if (gpio == L_TRIGGER_PIN)
            pressed = 5;
        else if (gpio == R_JOYSTICK_SW_PIN)
            pressed = 6;
        else if (gpio == L_JOYSTICK_SW_PIN)
            pressed = 7;
    }

    xQueueSendFromISR(xQueueGameButton, &pressed, 0);
}

/* ------------------------------ Tasks ------------------------------ */
void hc06_task(void *p) {
    printf("HC06 Task\n");
    uart_init(HC06_UART_ID, HC06_BAUD_RATE);
    gpio_set_function(HC06_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(HC06_RX_PIN, GPIO_FUNC_UART);
    hc06_init("bruno-stanz", "1234");

    adc_t data;

    while (1) {
        if (xQueueReceive(xQueueBluetooth, &data, portMAX_DELAY)) {
            write_package(data);
        }
    }
}

void game_btn_task(void *p) {
    gpio_init(GAME_BTN_B_PIN);
    gpio_set_dir(GAME_BTN_B_PIN, GPIO_IN);
    gpio_pull_up(GAME_BTN_B_PIN);

    gpio_init(GAME_BTN_Y_PIN);
    gpio_set_dir(GAME_BTN_Y_PIN, GPIO_IN);
    gpio_pull_up(GAME_BTN_Y_PIN);

    gpio_init(GAME_BTN_X_PIN);
    gpio_set_dir(GAME_BTN_X_PIN, GPIO_IN);
    gpio_pull_up(GAME_BTN_X_PIN);

    gpio_init(GAME_BTN_A_PIN);
    gpio_set_dir(GAME_BTN_A_PIN, GPIO_IN);
    gpio_pull_up(GAME_BTN_A_PIN);

    gpio_init(R_TRIGGER_PIN);
    gpio_set_dir(R_TRIGGER_PIN, GPIO_IN);
    gpio_pull_up(R_TRIGGER_PIN);

    gpio_init(L_TRIGGER_PIN);
    gpio_set_dir(L_TRIGGER_PIN, GPIO_IN);
    gpio_pull_up(L_TRIGGER_PIN);

    gpio_init(L_JOYSTICK_SW_PIN);
    gpio_set_dir(L_JOYSTICK_SW_PIN, GPIO_IN);
    gpio_pull_up(L_JOYSTICK_SW_PIN);

    gpio_init(R_JOYSTICK_SW_PIN);
    gpio_set_dir(R_JOYSTICK_SW_PIN, GPIO_IN);
    gpio_pull_up(R_JOYSTICK_SW_PIN);

    gpio_set_irq_enabled_with_callback(GAME_BTN_B_PIN,
                                       GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE,
                                       true, &game_btn_callback);
    gpio_set_irq_enabled(GAME_BTN_Y_PIN, GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(GAME_BTN_X_PIN, GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(GAME_BTN_A_PIN, GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(R_TRIGGER_PIN, GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(L_TRIGGER_PIN, GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(L_JOYSTICK_SW_PIN, GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(R_JOYSTICK_SW_PIN, GPIO_IRQ_EDGE_FALL, true);

    uint32_t b_btn_last_trigger = 0;
    uint32_t y_btn_last_trigger = 0;
    uint32_t x_btn_last_trigger = 0;
    uint32_t a_btn_last_trigger = 0;
    uint32_t tr_btn_last_trigger = 0;
    uint32_t tl_btn_last_trigger = 0;
    uint32_t r_joysw_last_trigger = 0;
    uint32_t l_joysw_last_trigger = 0;

    uint pressed_button = 0;
    uint32_t trigger_time;

    adc_t data;

    while (true) {
        if (xQueueReceive(xQueueGameButton, &pressed_button, pdMS_TO_TICKS(100))) {
            trigger_time = to_ms_since_boot(get_absolute_time());
            if (pressed_button == 0 && has_debounced(trigger_time, b_btn_last_trigger)) {
                b_btn_last_trigger = trigger_time;
                data.axis = 0;
                data.val = 1;
                write_package(data);
            }

            else if (pressed_button == 1 && has_debounced(trigger_time, y_btn_last_trigger)) {
                y_btn_last_trigger = trigger_time;
                data.axis = 1;
                data.val = 1;
                write_package(data);
            }

            else if (pressed_button == 2 && has_debounced(trigger_time, x_btn_last_trigger)) {
                x_btn_last_trigger = trigger_time;
                data.axis = 2;
                data.val = 1;
                write_package(data);
            }

            else if (pressed_button == 3 && has_debounced(trigger_time, a_btn_last_trigger)) {
                a_btn_last_trigger = trigger_time;
                data.axis = 3;
                data.val = 1;
                write_package(data);
            }

            else if (pressed_button == 4 && has_debounced(trigger_time, tr_btn_last_trigger)) {
                tr_btn_last_trigger = trigger_time;
                data.axis = 4;
                data.val = 1;
                write_package(data);
            }

            else if (pressed_button == 5 && has_debounced(trigger_time, tl_btn_last_trigger)) {
                tl_btn_last_trigger = trigger_time;
                data.axis = 5;
                data.val = 1;
                write_package(data);
            }

            else if (pressed_button == 6 && has_debounced(trigger_time, r_joysw_last_trigger)) {
                r_joysw_last_trigger = trigger_time;
                data.axis = 10;
                data.val = 1;
                write_package(data);
            }

            else if (pressed_button == 7 && has_debounced(trigger_time, l_joysw_last_trigger)) {
                l_joysw_last_trigger = trigger_time;
                data.axis = 11;
                data.val = 1;
                write_package(data);
            }
        }
    }
}

void x_task(void *p) {
    adc_init();
    adc_gpio_init(26);
    adc_set_round_robin(0b00011);

    adc_t data;

    while (1) {
        data.axis = 6;
        adc_select_input(0);
        data.val = adc_read();

        int mapped_val = (data.val - 2047) * 255 / 2047;
        data.val = (int)(-mapped_val);

        xQueueSend(xQueueJoyStick, &data, portMAX_DELAY);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void y_task(void *p) {
    adc_init();
    adc_gpio_init(27);
    adc_set_round_robin(0b00011);

    adc_t data;

    while (1) {
        data.axis = 7;
        adc_select_input(1);
        data.val = adc_read();

        int mapped_val = (data.val - 2047) * 255 / 2047;
        data.val = (int)(-mapped_val);

        xQueueSend(xQueueJoyStick, &data, portMAX_DELAY);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void joystick_task(void *p) {
    adc_t data;

    while (1) {
        if (xQueueReceive(xQueueJoyStick, &data, portMAX_DELAY)) {
            if (data.val > -30 && data.val < 30) {
                data.val = 0;
            }
            xQueueSend(xQueueBluetooth, &data, portMAX_DELAY);
        }
    }
}


void mux_task(void *p) {
    adc_init();
    adc_gpio_init(MUX_ADC_PIN);
    adc_set_round_robin(0b00011);

    gpio_init(MUX_A_CONTROL_PIN);
    gpio_set_dir(MUX_A_CONTROL_PIN, GPIO_OUT);

    adc_t data;
    bool get_x = true;
    while (1) {
        if (get_x) {
            data.axis = 8;
            gpio_put(MUX_A_CONTROL_PIN, 0);
        } else {
            data.axis = 9;
            gpio_put(MUX_A_CONTROL_PIN, 1);
        }

        adc_select_input(2);
        data.val = adc_read();
        int mapped_val = (data.val - 2047) * 255 / 2047;
        data.val = (int)(-mapped_val);

        xQueueSend(xQueueJoyStickLeft, &data, portMAX_DELAY);
        vTaskDelay(pdMS_TO_TICKS(50));

        get_x = get_x ? false : true;
    }
}


void left_joystick_task(void *p) {
    adc_t data;

    while (1) {
        if (xQueueReceive(xQueueJoyStickLeft, &data, portMAX_DELAY)) {
            if (data.val > -42 && data.val < 42) {
                data.val = 0;
            }
            xQueueSend(xQueueBluetooth, &data, portMAX_DELAY);
        }
    }
}


/* ------------------------------ Main ------------------------------ */
int main() {
    stdio_init_all();
    adc_init();

    // Semaphores

    // Queues
    xQueueGameButton = xQueueCreate(32, sizeof(uint));
    if (xQueueGameButton == NULL) {
        printf("Falha em criar a fila xQueueGameButton... \n");
    }

    xQueueJoyStick = xQueueCreate(32, sizeof(adc_t));
    if (xQueueJoyStick == NULL) {
        printf("Falha em criar a fila xQueueJoyStick... \n");
    }

    xQueueBluetooth = xQueueCreate(32, sizeof(adc_t));
    if (xQueueBluetooth == NULL) {
        printf("Falha em criar a fila xQueueBluetooth... \n");
    }

    xQueueJoyStickLeft = xQueueCreate(32, sizeof(adc_t));
    if (xQueueJoyStickLeft == NULL) {
        printf("Falha em criar a fila xQueueJoyStickLeft... \n");
    }

    // Tasks
    xTaskCreate(hc06_task, "HC06 Task", 4096, NULL, 1, NULL);
    xTaskCreate(game_btn_task, "GameButton Task", 4096, NULL, 1, NULL);
    xTaskCreate(joystick_task, "JoyStick Task", 4096, NULL, 1, NULL);
    xTaskCreate(x_task, "x_task", 4096, NULL, 1, NULL);
    xTaskCreate(y_task, "y_task", 4096, NULL, 1, NULL);
    xTaskCreate(left_joystick_task, "left_joystick_task", 4096, NULL, 1, NULL);
    xTaskCreate(mux_task, "mux_task", 4096, NULL, 1, NULL);


    vTaskStartScheduler();
    while (true)
        ;
}
