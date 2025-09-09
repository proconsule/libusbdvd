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

inline bool endsWithIfo(const std::string& filename) noexcept {
    return filename.size() >= 4 &&
           (filename[filename.size()-4] | 32) == '.' &&  // No conversion needed for '.'
           (filename[filename.size()-3] | 32) == 'i' &&
           (filename[filename.size()-2] | 32) == 'f' &&
           (filename[filename.size()-1] | 32) == 'o';
}

bool usbdvdutils_pathExists(const std::string& path);
bool usbdvdutils_isDirectory(const std::string& path);
std::string usbdvdutils_joinPath(const std::string& base, const std::string& sub);
std::string create_dvd_hash_id(uint8_t vol_id[32], uint8_t creation_date[17]);

#endif