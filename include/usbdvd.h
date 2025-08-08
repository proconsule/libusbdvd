#ifndef USBDVD_H
#define USBDVD_H


#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

typedef struct{
	bool mounted;
	char disc_fstype[0x80];
	char mountpoint[0x05];
}usbdvd_fs_struct;

typedef struct{
	bool drive_found;
	char vendor_id[0x8+1];
    char product_id[0x10+1];
    char product_revision[0x4+1];
    char serial_number[0x8+1];
	char disc_type[0x20];
	usbdvd_fs_struct fs;
}usbdvd_drive_struct;


typedef struct usbdvd_obj usbdvd_obj;

usbdvd_obj* usbdvd_create();
void usbdvd_destroy(usbdvd_obj* obj);
usbdvd_drive_struct * usbdvd_get_drivectx(usbdvd_obj* obj);

#ifdef __cplusplus
}
#endif



#ifdef __cplusplus

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
	
	//bool drive_found = false;
	//bool acd_init_ok = false;
	//bool datacd_init_ok = false;
	bool fileimage = false;
	//std::string mountpoint = "";
	//std::string disc_fstype = "";
	//std::string disc_type;
	int drive_status = 0;
	
	usbdvd_drive_struct usbdvd_drive_ctx;
	
private:

	Thread UsbDVDThread = {0};
	
	bool usb_init = false;
	bool cdfs_init = false;
	bool pseudofs_init = false;
	
	
	std::string isofilepath;
	std::string cuepath;
	std::string binpath;
	
};

#endif










#endif

