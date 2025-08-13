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
    std::string name;
    bool isdir;
    uint32_t size;
    uint32_t lba;
	time_t time = 0;	// ISO9660
	time_t access_time = 0;	// UDF
	time_t modification_time = 0;	// UDF
	time_t attribute_time = 0;	// UDF
	uint32_t partlocation = 0; // UDF 
    std::string fullpath;
	
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