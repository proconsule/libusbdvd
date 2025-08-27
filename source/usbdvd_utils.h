#ifndef USBDVD_UTILS_H
#define USBDVD_UTILS_H

#include <stdint.h>
#include <usbdvd_common.h>
#include <string>

void usbdvd_log(const char *fmt, ...);

uint32_t byte2u32_le(uint8_t * ptr);
uint32_t byte2u32_be(uint8_t * ptr);
uint16_t byte2u16_le(uint8_t * ptr);
uint16_t byte2u16_be(uint8_t * ptr);

bool cuebin_to_TOC(std::string _cuepath,std::string _binpath,CDDVD_TOC * _toc);
void lbaToMsf(uint32_t lba, uint8_t* minutes, uint8_t* seconds, uint8_t* frames);



std::string trim_left(const std::string& str);

//
//Right trim
//
std::string trim_right(const std::string& str);

//
//Left and Right trim
//
std::string trim(const std::string& str);


#endif