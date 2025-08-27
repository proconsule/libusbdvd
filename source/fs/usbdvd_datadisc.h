#ifndef USBDVD_DATADISC_H
#define USBDVD_DATADISC_H

#include <string>
#include <cstdint>
#include <vector>
#include <pthread.h>

#include "usbdvd_scsi.h"
#include "usbdvd_common.h"

#define DATA_SECOTR_SIZE 2048

typedef struct{
	uint32_t length;  
	uint32_t location;
	uint16_t partition_reference;  
}udf_extent_struct;

typedef struct{
    std::string name;
    bool isdir;
    uint64_t size;
    uint64_t lba;
	uint32_t meta_len; // UDF >= 2.50
	uint32_t part_lba; // Location ob Meatadata Partition UDF >= 2.50
	time_t time = 0;	// ISO9660
	time_t access_time = 0;	// UDF or RockRidge
	time_t modification_time = 0;	// UDF or RockRidge
	time_t attribute_time = 0;	// UDF or RockRidge
	uint32_t st_mode = 0;	// RockRidge
	uint32_t st_nlink = 0;	// RockRidge
	uint32_t st_uid = 0;	// RockRidge
	uint32_t st_gid = 0;	// RockRidge
	uint32_t st_ino = 0;	// RockRidge
	
    std::string fullpath;
	std::vector<udf_extent_struct> extents; //UDF Extents
	
}disc_dirlist_struct;

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
	disc_dirlist_struct * GetFileDescFromIDX(int idx);

    std::vector<disc_dirlist_struct> disc_dirlist;
    std::string SystemIdentifier;
    std::string VolumeIdentifier;
    uint32_t VolumeSectors;
    uint32_t VolumeSpace;
	bool isjoliet = false;
	bool isrockridge = false;
	int jolietver = 0;
    
protected:
    std::string filename;
    FILE *isofp;
    bool isofile = false;
    pthread_mutex_t read_lock;		
	CUSBSCSI * usb_scsi_ctx;
	uint32_t absstartlba;
	uint32_t absendlba;
	
	uint8_t read_buffer[2048];
    uint8_t read_sector = 0;
    
    
};

#endif