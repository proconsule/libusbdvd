#ifndef USBDVD_UDF_H
#define USBDVD_UDF_H

#include <string>
#include <cstdint>
#include <vector>
#include <pthread.h>

#include "usbdvd_scsi.h"
#include "usbdvd_common.h"
#include "usbdvd_datadisc.h"


class CUSBDVD_UDFFS: public CUSBDVD_DATADISC{
public:
    CUSBDVD_UDFFS(std::string _filename);
	CUSBDVD_UDFFS(CUSBSCSI * _usb_scsi_ctx,uint32_t _startlba,uint32_t _endlba);

    
	void UDFParse();
	
    int udf_filesectorread(uint32_t sector,uint8_t *buffer);
    void list_dir_iso9660(uint32_t sector, const std::string& path);
    void list_dir_joliet(uint32_t sector, const std::string& path);
	uint32_t GetFileSize(std::string _filename);
    
    
	void Parse_FID(uint32_t _partitionlba,uint32_t _fidptr,std::string _path);
	void Parse_FileEntry(uint32_t _partitionlba,uint32_t _fidptr,disc_dirlist_struct * _tmpfile);
	void Parse_FID_Ptr(uint8_t * buffer,std::string _path);
	void Parse_FileEntry_Ptr(uint8_t * buffer,disc_dirlist_struct * _tmpfile);


    std::string SystemIdentifier;
    std::string VolumeIdentifier;
    uint32_t VolumeSectors;
    uint32_t VolumeSpace;
	
	std::string udf_version_string = "";

private:
    
	
	uint32_t partitionlba = 0;
    
    
};

#endif