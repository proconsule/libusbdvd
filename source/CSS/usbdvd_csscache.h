#ifndef USBDVD_CSS_H
#define USBDVD_CSS_H

#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>
#include <vector>

typedef struct{
    uint8_t key[5];
}key_storage_struct;

class CUSBDVD_CSSCache {
public:
    CUSBDVD_CSSCache(std::string  _cache_path = "css_cache.db");
    bool initializeCacheFile();
    bool addKey(const std::string& hash, const std::string& css_key);
    std::string getKey(const std::string& hash);
    bool keyExists(const std::string& hash);
    bool clearCache();
    void printCache();
    std::string keyToHexString(std::vector<key_storage_struct> keys);
    std::vector<key_storage_struct> hexStringToKeys(const std::string& hexString);
    int countEntrys();


    std::string cache_file_path;
};

#endif