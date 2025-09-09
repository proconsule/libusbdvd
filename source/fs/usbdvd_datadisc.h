#ifndef USBDVD_DATADISC_H
#define USBDVD_DATADISC_H

#include <string>
#include <cstdint>
#include <vector>
#include <pthread.h>

#include "usbdvd_scsi.h"
#include "usbdvd_common.h"

#define DATA_SECOTR_SIZE 2048

typedef struct {
    int file_idx;            // Indice nel vector dei file
    uint8_t *data;           // Dati del file in memoria
    size_t size;             // Dimensione del file
    bool is_loaded;          // Flag per sapere se è caricato
    time_t load_time;        // Opzionale: quando è stato caricato
} ifo_cache_entry_t;

typedef struct {
    ifo_cache_entry_t *entries;  // Array di entry IFO
    int num_entries;             // Numero di entry correnti
    int capacity;                // Capacità allocata
} ifo_cache_t;

typedef struct{
    uint32_t length;  
    uint32_t location;
    uint16_t partition_reference;  
}udf_extent_struct;

typedef struct{
    std::string name;
    bool isdir;
    bool cached = false;
    int idx;
    uint64_t size;
    uint64_t lba;
    uint32_t meta_len; // UDF >= 2.50
    uint32_t part_lba; // Location ob Meatadata Partition UDF >= 2.50
    time_t time = 0;    // ISO9660
    time_t access_time = 0; // UDF or RockRidge
    time_t modification_time = 0;   // UDF or RockRidge
    time_t attribute_time = 0;  // UDF or RockRidge
    uint32_t st_mode = 0;   // RockRidge
    uint32_t st_nlink = 0;  // RockRidge
    uint32_t st_uid = 0;    // RockRidge
    uint32_t st_gid = 0;    // RockRidge
    uint32_t st_ino = 0;    // RockRidge
    
    std::string fullpath;
    std::vector<udf_extent_struct> extents; //UDF Extents
    
}disc_dirlist_struct;

typedef struct{
    uint8_t read_buffer[2048];
    uint8_t read_sector = 0;
}drive_readbuf_struct;

typedef struct{
    uint8_t key[5];
    uint8_t titlenum;
}css_titlekey_struct;

class CUSBDVD_DATADISC{
public:
    CUSBDVD_DATADISC(std::string _filename);
    CUSBDVD_DATADISC(CUSBSCSI * _usb_scsi_ctx,uint32_t _startlba,uint32_t _endlba);
    ~CUSBDVD_DATADISC();
    
    void UDFParse();
    
    int isofile_filesectorread(uint32_t sector,uint8_t *buffer);
    uint32_t GetFileSize(std::string _filename);
    
    int ReadSector(uint32_t sector,uint8_t * buffer);
    int ReadNumSectors(uint32_t startsector,uint16_t numblocks,uint8_t * buffer);
    int ReadSectorsLen(uint32_t startsector,uint32_t _len,uint8_t * buffer);
    int ReadData(disc_dirlist_struct * _filedesc,uint32_t pos,uint32_t size,uint8_t * buf);
    int GetFileDesc(std::string _filename,disc_dirlist_struct & _filedesc);
    int FindFile(std::string _filename);
    void DVDGetAllCSSKeys();
    void Cache_IFO_Files();
    disc_dirlist_struct * GetFileDescFromIDX(int idx);
    bool isAACSProtected(const std::string& directory_path);

    std::vector<disc_dirlist_struct> disc_dirlist;
    std::string SystemIdentifier;
    std::string VolumeIdentifier;
    uint32_t VolumeSectors;
    uint32_t VolumeSpace;
    bool isjoliet = false;
    bool isrockridge = false;
    int jolietver = 0;
    
    int FindTitleKey_IDX(std::string _fullpath);
    
    
    std::vector<css_titlekey_struct> titlekeys;
    int currenttitlekey_idx = -1;
    
    bool DVD_CSS = false;
    bool ACSS = false;
    CUSBSCSI * usb_scsi_ctx;
    
    std::string disc_hash;
    
protected:
    std::string filename;
    FILE *isofp;
    bool isofile = false;
    std::mutex read_mutex;
    uint32_t absstartlba;
    uint32_t absendlba;
    
    drive_readbuf_struct drive_readbuffer;
    //uint8_t read_buffer[2048];
    //uint8_t read_sector = 0;
    
    ifo_cache_t g_ifo_cache = {NULL, 0, 0};
    void cleanup_ifo_cache();
    int load_ifo_by_index(int file_idx);
    uint8_t* get_ifo_data(int file_idx, size_t *size_out);
    
    
    
};

#endif