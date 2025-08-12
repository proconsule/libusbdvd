#ifndef USBDVD_UDF_H
#define USBDVD_UDF_H

#include <string>
#include <cstdint>
#include <vector>
#include <pthread.h>

#include "usbdvd_scsi.h"
#include "usbdvd_common.h"


typedef struct{
    std::string name;
    bool isdir;
    uint64_t size;
    uint32_t lba;
	uint32_t partlocation;
	time_t utc_time = 0;
    std::string fullpath;
    
}udf_dirlist_struct;

class CUSBDVD_UDFFS{
public:
    CUSBDVD_UDFFS(std::string _filename);
	CUSBDVD_UDFFS(CUSBSCSI * _usb_scsi_ctx,uint32_t _startlba,uint32_t _endlba);
	~CUSBDVD_UDFFS();
    
	void UDFParse();
	
    int udf_filesectorread(uint32_t sector,uint8_t *buffer);
    void list_dir_iso9660(uint32_t sector, const std::string& path);
    void list_dir_joliet(uint32_t sector, const std::string& path);
	uint32_t GetFileSize(std::string _filename);
    
    int ReadSector(uint32_t sector,uint8_t * buffer);
	int ReadData(udf_dirlist_struct * _filedesc,uint32_t pos,uint32_t size,uint8_t * buf);
    int GetFileDesc(std::string _filename,udf_dirlist_struct & _filedesc);
    int FindFile(std::string _filename);
	udf_dirlist_struct * GetFileDescFromIDX(int idx);

	void Parse_FID(uint32_t _partitionlba,uint32_t _fidptr,std::string _path);
	void Parse_FileEntry(uint32_t _partitionlba,uint32_t _fidptr,udf_dirlist_struct * _tmpfile);
	void Parse_FID_Ptr(uint8_t * buffer,std::string _path);
	void Parse_FileEntry_Ptr(uint8_t * buffer,udf_dirlist_struct * _tmpfile);


    std::vector<udf_dirlist_struct> udf_dirlist;
    std::string SystemIdentifier;
    std::string VolumeIdentifier;
    uint32_t VolumeSectors;
    uint32_t VolumeSpace;
	bool isjoliet = false;
	bool isrockridge = false;
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
	
	uint32_t partitionlba = 0;
    
    
};

#endif