#ifndef USBDVD_UDF_H
#define USBDVD_UDF_H

#include <string>
#include <cstdint>
#include <vector>
#include <pthread.h>

#include "usbdvd_scsi.h"
#include "usbdvd_common.h"
#include "usbdvd_datadisc.h"

typedef struct {
    uint32_t length;  
    uint32_t location;   
    uint32_t logical_block_size; 
    uint32_t number_of_partition_maps;  
    uint32_t udfver;
    uint32_t metadata_partition_size;
    std::string udf_version_string;
    std::string VolumeIdentifier;
    uint32_t meta_partition_location;
}udf_lvd_struct;


typedef struct{
    uint64_t sector;
    uint64_t len;
    
}sector_range_struct;

class CUSBDVD_UDFFS: public CUSBDVD_DATADISC{
public:
    CUSBDVD_UDFFS(std::string _filename);
    CUSBDVD_UDFFS(CUSBSCSI * _usb_scsi_ctx,uint32_t _startlba,uint32_t _endlba);
    ~CUSBDVD_UDFFS();
    
    void UDFParse();
    
    int udf_filesectorread(uint32_t sector,uint8_t *buffer);
    void list_dir_iso9660(uint32_t sector, const std::string& path);
    void list_dir_joliet(uint32_t sector, const std::string& path);
    uint32_t GetFileSize(std::string _filename);
    
    
    void Parse_FID(uint32_t _partitionlba,uint32_t _fidptr,std::string _path);
    void Parse_FileEntry(uint32_t _partitionlba,uint32_t _fidptr,disc_dirlist_struct * _tmpfile);
    void Parse_FID_Ptr(uint8_t * buffer,std::string _path,uint32_t _fidsize);
    void Parse_FileEntry_Ptr(uint8_t * buffer,disc_dirlist_struct * _tmpfile);
    void Parse_ExtendedFileEntry_Ptr(uint8_t * buffer,disc_dirlist_struct * _tmpfile);
    

    int ReadExtents(std::vector<sector_range_struct> _seclist,uint32_t _len,uint8_t * buffer);
    int UDFReadData(disc_dirlist_struct * _filedesc,uint32_t pos,uint32_t size,uint8_t * buf);

    std::string SystemIdentifier;
    //std::string VolumeIdentifier;
    uint32_t VolumeSectors;
    uint32_t VolumeSpace;
    
    //std::string udf_version_string = "";
    udf_lvd_struct disc_lvd;

private:
    
    uint32_t partitionfsdlba = 0;

    uint8_t * metadata_partition_buffer = NULL;
    
    uint32_t metadata_partition_lba = 0;
    uint32_t metadata_fsd_offset = 0;
    uint32_t partitionlba = 0;
    bool udf15plus = false;
    
    
};

#endif