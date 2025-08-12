#ifndef USBDVD_UTILS_H
#define USBDVD_UTILS_H

#include <stdint.h>

void usbdvd_log(const char *fmt, ...);

uint32_t byte2u32_le(uint8_t * ptr);
uint32_t byte2u32_be(uint8_t * ptr);
uint16_t byte2u16_le(uint8_t * ptr);
uint16_t byte2u16_be(uint8_t * ptr);


#endif