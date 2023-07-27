#include <driver/gpio.h>
#include <esp_wifi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
#include <driver/dedic_gpio.h>

#include <math.h>

#include "soc/gpio_reg.h"
#include "soc/gpio_struct.h"

#define RST 15
#define CS 2
#define RS 0
#define RD 4

const int pins[8] = { 33,32,13,21,14,27,26,25 };
const int function_pins[4] = { RST,CS,RS,RD };

void setGPIOHigh(int pin) {
	GPIO.out_w1ts = 1 << pin;
}

void setGPIOLow(int pin) {
	GPIO.out_w1tc = 1 << pin;
}

void sendByte(bool isCmd, uint8_t data) {
	if (isCmd) {
		setGPIOLow(RS);
	}
	else {
		setGPIOHigh(RS);
	}
	setGPIOLow(CS);
	for (int i = 0; i < 8; i++) {
		if ((data >> i) & 0x1) {
			if (pins[i] < 32) {
				setGPIOHigh(pins[i]);
			}
			else {
				GPIO.out1_w1ts.val = (1 << (pins[i] - 32));
			}
		}
		else {
			if (pins[i] < 32) {
				setGPIOLow(pins[i]);
			}
			else {
				GPIO.out1_w1tc.val = (1 << (pins[i] - 32));
			}
		}
	}
	setGPIOLow(RD);
	setGPIOHigh(RD);
	setGPIOHigh(CS);
}

void send2Byte(bool isCmd, uint16_t data) {
	sendByte(isCmd, data >> 8);
	sendByte(isCmd, data & 0xFF);
}

void initPins() {
	for (int i = 0; i < 8; i++) {
		esp_rom_gpio_pad_select_gpio(pins[i]);
		gpio_set_direction(pins[i], GPIO_MODE_OUTPUT);
	}

	for (int i = 0; i < 4; i++) {
		esp_rom_gpio_pad_select_gpio(function_pins[i]);
		gpio_set_direction(function_pins[i], GPIO_MODE_OUTPUT);
		gpio_set_level(function_pins[i], 1);
	}
}

void initLCD() {
	sendByte(1, 0x01);
	vTaskDelay(12);

	sendByte(1, 0x11);
	vTaskDelay(1);

	sendByte(1, 0x36);
	sendByte(0, 0x00);

	sendByte(1, 0x3A);
	sendByte(0, 0x55);

	sendByte(1, 0x29);
}

void setRegion(int xStart, int yStart, int xEnd, int yEnd) {
	sendByte(1, 0x2A);
	send2Byte(0, xStart);
	send2Byte(0, xEnd - 1);

	sendByte(1, 0x2B);
	send2Byte(0, yStart);
	send2Byte(0, yEnd - 1);

	sendByte(1, 0x2C);
}

void fillLCD(uint16_t color) {
	setRegion(0, 0, 240, 320);
	for (int i = 0; i < 240; i++) {
		for (int j = 0; j < 320; j++) {
			send2Byte(0, color);
		}
	}
}

void fillRect(uint16_t color, int xStart, int yStart, int xEnd, int yEnd) {
	setRegion(xStart, yStart, xEnd, yEnd);
	for (int i = 0; i < xEnd - xStart; i++) {
		for (int j = 0; j < yEnd - yStart; j++) {
			send2Byte(0, color);
		}
	}
}

void app_main() {
	initPins();
	initLCD();

	fillLCD(0xFFFF);
	int i = 0;
	while (1) {
		fillRect(0xFFFF, i, i, 25 + i, 25 + i);
		i++;
		if (i > 240 - 25) {
			i = 0;
		}
		fillRect(0x0000, i, i, 25 + i, 25 + i);

		vTaskDelay(1);
	}
}
