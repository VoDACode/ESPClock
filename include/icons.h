#ifndef ICONS_DEF_H
#define ICONS_DEF_H

#include <Arduino.h>

// Іконки у форматі XBM для OLED дисплея 128x64

#define ICON_WIFI_CONNECTED_width 9
#define ICON_WIFI_CONNECTED_height 9
const unsigned char ICON_WIFI_CONNECTED_bits [] PROGMEM = {
	// 'wifi, 9x9px
	0x00, 0x00, 0x00, 0x00, 0x3e, 0x00, 0x41, 0x00, 0x1c, 0x00, 0x22, 0x00, 0x08, 0x00, 0x00, 0x00, 
	0x00, 0x00
};

#endif // ICONS_DEF_H