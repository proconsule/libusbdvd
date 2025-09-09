#include "usbdvd_csscache.h"


CUSBDVD_CSSCache::CUSBDVD_CSSCache(std::string  _cache_path){
    cache_file_path = _cache_path;
    initializeCacheFile();
}

bool CUSBDVD_CSSCache::initializeCacheFile() {
    if (std::filesystem::exists(cache_file_path)) {
        return true;
    }

    std::ofstream cache_file(cache_file_path);
    if (!cache_file.is_open()) {
        return false;
    }

    cache_file << "# DVD Cache - format: hash:css_keys" << std::endl;
    cache_file.close();
    return true;
}

bool CUSBDVD_CSSCache::addKey(const std::string& hash, const std::string& css_key) {
    if (hash.length() != 32) {
        return false;
    }

    for (char c : css_key) {
        if (!std::isxdigit(c)) {
            return false;
        }
    }

    if (keyExists(hash)) {
        return true;
    }

    std::ofstream cache_file(cache_file_path, std::ios::app);
    if (!cache_file.is_open()) {
        return false;
    }

    cache_file << hash << ":" << css_key << std::endl;
    cache_file.close();

    return true;
}

bool CUSBDVD_CSSCache::keyExists(const std::string& hash) {
    std::ifstream cache_file(cache_file_path);
    if (!cache_file.is_open()) {
        return false;
    }

    std::string line;
    while (std::getline(cache_file, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }

        size_t delimiter_pos = line.find(':');
        if (delimiter_pos != std::string::npos) {
            std::string stored_hash = line.substr(0, delimiter_pos);
            if (stored_hash == hash) {
                cache_file.close();
                return true;
            }
        }
    }

    cache_file.close();
    return false;
}

std::string CUSBDVD_CSSCache::getKey(const std::string& hash) {
    std::ifstream cache_file(cache_file_path);
    if (!cache_file.is_open()) {
        return "";
    }

    std::string line;
    while (std::getline(cache_file, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }

        size_t delimiter_pos = line.find(':');
        if (delimiter_pos != std::string::npos) {
            std::string stored_hash = line.substr(0, delimiter_pos);
            if (stored_hash == hash) {
                std::string css_key = line.substr(delimiter_pos + 1);
                cache_file.close();
                return css_key;
            }
        }
    }

    cache_file.close();
    return "";
}

bool CUSBDVD_CSSCache::clearCache() {
    if (std::filesystem::exists(cache_file_path)) {
        std::filesystem::remove(cache_file_path);
    }

    return initializeCacheFile();
}

void CUSBDVD_CSSCache::printCache() {
    std::ifstream cache_file(cache_file_path);
    if (!cache_file.is_open()) {
        std::cout << "File cache non trovato" << std::endl;
        return;
    }

    std::cout << "Contenuto del cache DVD:" << std::endl;
    std::string line;
    int count = 0;
    
    while (std::getline(cache_file, line)) {
        if (!line.empty() && line[0] != '#') {
            std::cout << line << std::endl;
            count++;
        }
    }

    std::cout << "Totale chiavi: " << count << std::endl;
    cache_file.close();
}


int CUSBDVD_CSSCache::countEntrys() {
    std::ifstream cache_file(cache_file_path);
    if (!cache_file.is_open()) {
        return 0;
    }

    std::string line;
    int count = 0;
    
    while (std::getline(cache_file, line)) {
        if (!line.empty() && line[0] != '#') {
            std::cout << line << std::endl;
            count++;
        }
    }
    cache_file.close();
    return count;
}

std::string CUSBDVD_CSSCache::keyToHexString(std::vector<key_storage_struct> keys) {
    std::stringstream ss;
    ss << std::hex << std::uppercase << std::setfill('0');
    
    for (const auto& keyStruct : keys) {
        for (int i = 0; i < 5; ++i) {
            ss << std::setw(2) << static_cast<int>(keyStruct.key[i]);
        }
    }
    
    return ss.str();
}

std::vector<key_storage_struct> CUSBDVD_CSSCache::hexStringToKeys(const std::string& hexString) {
    std::vector<key_storage_struct> keys;
    
    
    if (hexString.length() % 10 != 0) {
        return keys;
    }
    
    size_t numKeys = hexString.length() / 10;
    keys.reserve(numKeys);
    
    for (size_t keyIndex = 0; keyIndex < numKeys; ++keyIndex) {
        key_storage_struct keyStruct;
        
        for (int byteIndex = 0; byteIndex < 5; ++byteIndex) {
            size_t hexPos = keyIndex * 10 + byteIndex * 2;
            std::string byteHex = hexString.substr(hexPos, 2);
            keyStruct.key[byteIndex] = static_cast<uint8_t>(std::stoi(byteHex, nullptr, 16));
        }
        
        keys.push_back(keyStruct);
    }
    
    return keys;
}

