/*
 * LED blink with FreeRTOS
 */
#include <FreeRTOS.h>
#include <queue.h>
#include <semphr.h>
#include <stdio.h>
#include <string.h>
#include <task.h>
#include "hardware/adc.h"
#include <math.h>
#include <stdlib.h>

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

#define GAME_BTN_B_PIN 2
#define GAME_BTN_Y_PIN 3
#define GAME_BTN_X_PIN 4
#define GAME_BTN_A_PIN 5

#define DEAD_ZONE 180

/* ------------------------------ Data structures ------------------------------ */
typedef struct adc {
    int axis;
    int val;
} adc_t;

/* ------------------------------ Global variables ------------------------------ */
QueueHandle_t xQueueGameButton, xQueueJoyStick;

/* ------------------------------ Utilities ------------------------------ */
bool has_debounced(uint32_t current_trigger, uint32_t last_trigger) {
    return current_trigger - last_trigger > DEBOUNCE_TIME;
}

int read_and_filter_adc(int axis) {
    adc_select_input(axis);
    int raw = adc_read();
    printf("\nrax axis (%d): %d\n", axis, raw);

    int scaled_val = ((raw - 2047) / 8);

    if (abs(scaled_val) < DEAD_ZONE) {
        scaled_val = 0; // Apply deadzone
    }

    return scaled_val;
}

void write_package(adc_t data) {
    int val = data.val;
    int msb = val >> 8;
    int lsb = val & 0xFF;

    if (data.axis < 4) {
        printf("\nHuman readable: %d %d \n", data.axis, data.val);
    }

    if (data.axis == 6 || data.axis == 7) {
        printf("\nHuman readable: %d %d \n", data.axis, data.val);
    }

    // uart_putc_raw(uart0, data.axis);
    // uart_putc_raw(uart0, msb);
    // uart_putc_raw(uart0, lsb);
    // uart_putc_raw(uart0, -1);
}

/* ------------------------------ Callbacks ------------------------------ */
void game_btn_callback(uint gpio, uint32_t events) {
    uint pressed = 0;

    if (events == 0x4) {  // fall edge
        if (gpio == GAME_BTN_B_PIN)
            pressed = 0;
        else if (gpio == GAME_BTN_Y_PIN)
            pressed = 1;
        else if (gpio == GAME_BTN_X_PIN)
            pressed = 2;
        else if (gpio == GAME_BTN_A_PIN)
            pressed = 3;
    }

    xQueueSendFromISR(xQueueGameButton, &pressed, 0);
}

/* ------------------------------ Tasks ------------------------------ */
void hc06_task(void *p) {
    uart_init(HC06_UART_ID, HC06_BAUD_RATE);
    gpio_set_function(HC06_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(HC06_RX_PIN, GPIO_FUNC_UART);
    hc06_init("bruno-stanz", "1234");

    while (true) {
        uart_puts(HC06_UART_ID, "OLAAA ");
        vTaskDelay(pdMS_TO_TICKS(100));
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

    gpio_set_irq_enabled_with_callback(GAME_BTN_B_PIN,
                                       GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE,
                                       true, &game_btn_callback);
    gpio_set_irq_enabled(GAME_BTN_Y_PIN, GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(GAME_BTN_X_PIN, GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(GAME_BTN_A_PIN, GPIO_IRQ_EDGE_FALL, true);

    uint32_t b_btn_last_trigger = 0;
    uint32_t y_btn_last_trigger = 0;
    uint32_t x_btn_last_trigger = 0;
    uint32_t a_btn_last_trigger = 0;

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
        }
    }
}

void x_task(void *p) {
    adc_gpio_init(26);
    adc_t data;
    data.axis = 6;

    while (1) {
        int val = adc_read();
        int mapped_val   = (data.val - 2047) * 255 / 2047;
        data.val = (int) mapped_val * 0.5;
        xQueueSend(xQueueJoyStick, &data, portMAX_DELAY);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void y_task(void *p) {
    adc_gpio_init(27);
    adc_t data;
    data.axis = 7;

    while (1) {
        int val = adc_read();
        int mapped_val   = (data.val - 2047) * 255 / 2047;
        data.val = (int) mapped_val * 0.5;
        xQueueSend(xQueueJoyStick, &data, portMAX_DELAY);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}


void joystick_task(void *p) {
    adc_t data;

    while (1) {
        if (xQueueReceive(xQueueJoyStick, &data, portMAX_DELAY)) {
            write_package(data);
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

    // Tasks
    xTaskCreate(game_btn_task, "GameButton Task", 4096, NULL, 1, NULL);
    xTaskCreate(joystick_task, "JoyStick Task",   4096, NULL, 1, NULL);
    xTaskCreate(x_task,    "x_task",    4096, NULL, 1, NULL);
    xTaskCreate(y_task,    "y_task",    4096, NULL, 1, NULL);

    vTaskStartScheduler();
    while (true);
}
