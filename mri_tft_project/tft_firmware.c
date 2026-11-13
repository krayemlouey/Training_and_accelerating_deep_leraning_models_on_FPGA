#include "xio_switch.h"
#include "gpio.h"
#include "timer.h"

#define PIN_RD   0
#define PIN_WR   1
#define PIN_CD   2
#define PIN_CS   3
#define PIN_RST  4

#define CMD_INIT        0x01
#define CMD_CLEAR       0x02
#define CMD_SET_WIN     0x03
#define CMD_WRITE_BURST 0x04

static gpio gpio_ctrl;
static gpio gpio_data;

void write_8bit(uint8_t val) {
    gpio_write(gpio_data, val);
}

void write_cmd(uint8_t cmd) {
    gpio_set(gpio_ctrl, PIN_CS, 0);
    gpio_set(gpio_ctrl, PIN_CD, 0);
    write_8bit(cmd);
    gpio_set(gpio_ctrl, PIN_WR, 0);
    gpio_set(gpio_ctrl, PIN_WR, 1);
    gpio_set(gpio_ctrl, PIN_CS, 1);
}

void write_data(uint8_t data) {
    gpio_set(gpio_ctrl, PIN_CS, 0);
    gpio_set(gpio_ctrl, PIN_CD, 1);
    write_8bit(data);
    gpio_set(gpio_ctrl, PIN_WR, 0);
    gpio_set(gpio_ctrl, PIN_WR, 1);
    gpio_set(gpio_ctrl, PIN_CS, 1);
}

void set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    write_cmd(0x2A);
    write_data(x0 >> 8);
    write_data(x0 & 0xFF);
    write_data(x1 >> 8);
    write_data(x1 & 0xFF);

    write_cmd(0x2B);
    write_data(y0 >> 8);
    write_data(y0 & 0xFF);
    write_data(y1 >> 8);
    write_data(y1 & 0xFF);

    write_cmd(0x2C);
}

void tft_init(void) {
    gpio_set(gpio_ctrl, PIN_RD, 1);
    gpio_set(gpio_ctrl, PIN_WR, 1);
    gpio_set(gpio_ctrl, PIN_CS, 1);

    gpio_set(gpio_ctrl, PIN_RST, 0);
    delay_ms(50);
    gpio_set(gpio_ctrl, PIN_RST, 1);
    delay_ms(150);

    write_cmd(0x01);
    delay_ms(100);
    write_cmd(0x28);
    write_cmd(0x3A);
    write_data(0x55);
    write_cmd(0x36);
    write_data(0x48); // Orientation Paysage
    write_cmd(0x11);
    delay_ms(120);
    write_cmd(0x29);
}

void tft_fill(uint16_t color) {
    set_window(0, 0, 319, 239);
    uint8_t hi = color >> 8;
    uint8_t lo = color & 0xFF;
    
    gpio_set(gpio_ctrl, PIN_CS, 0);
    gpio_set(gpio_ctrl, PIN_CD, 1);
    for (int i = 0; i < 320 * 240; i++) {
        write_8bit(hi);
        gpio_set(gpio_ctrl, PIN_WR, 0);
        gpio_set(gpio_ctrl, PIN_WR, 1);
        write_8bit(lo);
        gpio_set(gpio_ctrl, PIN_WR, 0);
        gpio_set(gpio_ctrl, PIN_WR, 1);
    }
    gpio_set(gpio_ctrl, PIN_CS, 1);
}

int main(void) {
    gpio_ctrl = gpio_open(0);
    gpio_data = gpio_open(1);

    while (1) {
        uint32_t cmd = MAILBOX_CMD_ADDR;
        if (cmd == CMD_INIT) {
            tft_init();
            MAILBOX_CMD_ADDR = 0;
        } else if (cmd == CMD_CLEAR) {
            uint32_t color = MAILBOX_DATA(0);
            tft_fill((uint16_t)color);
            MAILBOX_CMD_ADDR = 0;
        } else if (cmd == CMD_SET_WIN) {
            uint16_t x0 = MAILBOX_DATA(0);
            uint16_t y0 = MAILBOX_DATA(1);
            uint16_t x1 = MAILBOX_DATA(2);
            uint16_t y1 = MAILBOX_DATA(3);
            set_window(x0, y0, x1, y1);
            MAILBOX_CMD_ADDR = 0;
        } else if (cmd == CMD_WRITE_BURST) {
            uint16_t count = MAILBOX_DATA(0);
            uint16_t *pixels = (uint16_t *)&MAILBOX_DATA(1);
            gpio_set(gpio_ctrl, PIN_CS, 0);
            gpio_set(gpio_ctrl, PIN_CD, 1);
            for (int i = 0; i < count; i++) {
                uint16_t px = pixels[i];
                write_8bit(px >> 8);
                gpio_set(gpio_ctrl, PIN_WR, 0);
                gpio_set(gpio_ctrl, PIN_WR, 1);
                write_8bit(px & 0xFF);
                gpio_set(gpio_ctrl, PIN_WR, 0);
                gpio_set(gpio_ctrl, PIN_WR, 1);
            }
            gpio_set(gpio_ctrl, PIN_CS, 1);
            MAILBOX_CMD_ADDR = 0;
        }
    }
    return 0;
}
