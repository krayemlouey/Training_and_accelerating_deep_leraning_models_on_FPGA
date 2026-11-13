#include "xio_switch.h"
#include "gpio.h"
#include "timer.h"

// Définition des broches de contrôle (A0..A3 -> GPIO 14..17)
#define PIN_RD  14
#define PIN_WR  15
#define PIN_CD  16
#define PIN_CS  17

// Commandes Mailbox
#define CMD_PING    0x01
#define CMD_INIT    0x02
#define CMD_CLEAR   0x03
#define CMD_PIXELS  0x04

static gpio gpio_port;

// Mappage physique standard Arduino Shield :
// D0->D8, D1->D9, D2->D2, D3->D3, D4->D4, D5->D5, D6->D6, D7->D7
static const int pin_map[8] = {8, 9, 2, 3, 4, 5, 6, 7};

// Envoi d'un octet sur le bus de données
void write_8bit(uint8_t val) {
    for (int i = 0; i < 8; i++) {
        gpio_set(gpio_port, pin_map[i], (val >> i) & 1);
    }
}

void write_cmd(uint8_t cmd) {
    gpio_set(gpio_port, PIN_CS, 0);
    gpio_set(gpio_port, PIN_CD, 0); // 0 = Commande
    write_8bit(cmd);
    gpio_set(gpio_port, PIN_WR, 0);
    gpio_set(gpio_port, PIN_WR, 1);
    gpio_set(gpio_port, PIN_CS, 1);
}

void write_data(uint8_t data) {
    gpio_set(gpio_port, PIN_CS, 0);
    gpio_set(gpio_port, PIN_CD, 1); // 1 = Donnée
    write_8bit(data);
    gpio_set(gpio_port, PIN_WR, 0);
    gpio_set(gpio_port, PIN_WR, 1);
    gpio_set(gpio_port, PIN_CS, 1);
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

void lcd_init(void) {
    gpio_set(gpio_port, PIN_RD, 1);
    gpio_set(gpio_port, PIN_WR, 1);
    gpio_set(gpio_port, PIN_CS, 1);
    delay_ms(50);

    // Sequence standard de réveil
    write_cmd(0x01); // Soft Reset
    delay_ms(120);
    write_cmd(0x11); // Sleep Out
    delay_ms(120);
    write_cmd(0x3A); // Format Pixel
    write_data(0x55); // 16 bits (RGB565)
    write_cmd(0x36); // Orientation
    write_data(0x48); // Mode Paysage
    write_cmd(0x29); // Display ON
    delay_ms(50);
}

void lcd_fill(uint16_t color) {
    set_window(0, 0, 319, 239);
    uint8_t hi = color >> 8;
    uint8_t lo = color & 0xFF;

    gpio_set(gpio_port, PIN_CS, 0);
    gpio_set(gpio_port, PIN_CD, 1);
    for (int i = 0; i < 320 * 240; i++) {
        write_8bit(hi);
        gpio_set(gpio_port, PIN_WR, 0);
        gpio_set(gpio_port, PIN_WR, 1);
        write_8bit(lo);
        gpio_set(gpio_port, PIN_WR, 0);
        gpio_set(gpio_port, PIN_WR, 1);
    }
    gpio_set(gpio_port, PIN_CS, 1);
}

int main() {
    init_io_switch();
    gpio_port = gpio_open_device(0);
    gpio_set_direction(gpio_port, 0x00000000);

    MAILBOX_CMD_ADDR = 0;

    while (1) {
        while (MAILBOX_CMD_ADDR == 0);
        uint32_t cmd = MAILBOX_CMD_ADDR;

        if (cmd == CMD_PING) {
            MAILBOX_DATA(0) = 0x12345678;
            MAILBOX_CMD_ADDR = 0;
        }
        else if (cmd == CMD_INIT) {
            lcd_init();
            MAILBOX_CMD_ADDR = 0;
        }
        else if (cmd == CMD_CLEAR) {
            uint32_t color = MAILBOX_DATA(0);
            lcd_fill((uint16_t)color);
            MAILBOX_CMD_ADDR = 0;
        }
        else {
            MAILBOX_CMD_ADDR = 0;
        }
    }
    return 0;
}
