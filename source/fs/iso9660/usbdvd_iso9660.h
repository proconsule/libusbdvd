#ifndef USBDVD_ISO9660_H
#define USBDVD_ISO9660_H

#include <string>
#include <cstdint>
#include <vector>
#include <pthread.h>

#include "usbdvd_scsi.h"
#include "usbdvd_common.h"
#include "usbdvd_datadisc.h"


class CUSBDVD_ISO9660FS : public CUSBDVD_DATADISC{
public:
    CUSBDVD_ISO9660FS(std::string _filename);
	CUSBDVD_ISO9660FS(CUSBSCSI * _usb_scsi_ctx,uint32_t _startlba,uint32_t _endlba);
	
    void list_dir_iso9660(uint32_t sector, const std::string& path);
    void list_dir_joliet(uint32_t sector, const std::string& path);
	uint32_t GetFileSize(std::string _filename);
  
};

#endif