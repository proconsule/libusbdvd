#ifndef USBDVD_H
#define USBDVD_H

#include <string>
#include <switch.h>

class SWITCH_AUDIOCDFS;
class CSWITCH_USB; 
class CAUDIOCD_PSEUDOFS;
class CUSBSCSI;
class CUSBDVD_ISO9660FS;
class SWITCH_ISO9660FS;

class CUSBDVD{
public:
	CUSBDVD();
	CUSBDVD(std::string _cuepath,std::string _binpath);
	CUSBDVD(std::string _isofilepath);
	~CUSBDVD();
	
	SWITCH_AUDIOCDFS *SWITCH_CDAUDIODEVOPTAB = nullptr;
	
	CSWITCH_USB *SWITCH_USB = nullptr;
	CAUDIOCD_PSEUDOFS *CDAUDIOFS = nullptr;
	CUSBDVD_ISO9660FS *ISO9660FS = nullptr;
	CUSBSCSI *USB_SCSI = nullptr;
	SWITCH_ISO9660FS * SWITCH_ISO9660DEVOPTAB = nullptr;
	
	std::string vendor_id;
	std::string product_id;
	std::string product_revision;
	std::string serial_number;
	
	std::string disctype;
	
	bool acd_init_ok = false;
	bool datacd_init_ok = false;
	bool fileimage = false;
	std::string mountpoint = "";
	int drive_status = 0;
	
private:

	Thread UsbDVDThread = {0};
	
	bool usb_init = false;
	bool cdfs_init = false;
	bool pseudofs_init = false;
	
	
	std::string isofilepath;
	std::string cuepath;
	std::string binpath;
	
};

#endif /* USBDVD_H */

