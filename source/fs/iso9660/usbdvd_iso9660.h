#ifndef USBDVD_ISO9660_H
#define USBDVD_ISO9660_H

#include <string>
#include <cstdint>
#include <vector>
#include <pthread.h>

#include "usbdvd_scsi.h"
#include "usbdvd_common.h"


typedef struct{
    std::string name;
    bool isdir;
    uint32_t size;
    uint32_t lba;
	time_t utc_time = 0;
    std::string fullpath;
    
}iso9660_dirlist_struct;

class CUSBDVD_ISO9660FS{
public:
    CUSBDVD_ISO9660FS(std::string _filename);
	CUSBDVD_ISO9660FS(CUSBSCSI * _usb_scsi_ctx,uint32_t _startlba,uint32_t _endlba);
	~CUSBDVD_ISO9660FS();
    
    int iso9660_filesectorread(uint32_t sector,uint8_t *buffer);
    void list_dir_iso9660(uint32_t sector, const std::string& path);
    void list_dir_joliet(uint32_t sector, const std::string& path);
	uint32_t GetFileSize(std::string _filename);
    
    int ReadSector(uint32_t sector,uint8_t * buffer);
	int ReadData(iso9660_dirlist_struct * _filedesc,uint32_t pos,uint32_t size,uint8_t * buf);
    int GetFileDesc(std::string _filename,iso9660_dirlist_struct & _filedesc);
    int FindFile(std::string _filename);
	iso9660_dirlist_struct * GetFileDescFromIDX(int idx);

    std::vector<iso9660_dirlist_struct> iso9660_dirlist;
    std::string SystemIdentifier;
    std::string VolumeIdentifier;
    uint32_t VolumeSectors;
    uint32_t VolumeSpace;
	bool isjoliet = false;
    int jolietver = 0;
    
private:
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