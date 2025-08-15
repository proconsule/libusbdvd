#ifndef USBDVD_H
#define USBDVD_H


#ifdef __cplusplus
extern "C" {
#endif

#define LIBUSBDVD_VERSION_MAJOR    0
#define LIBUSBDVD_VERSION_MINOR    0
#define LIBUSBDVD_VERSION_MICRO    6

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

#define LIBUSBDVD_VERSION_STRING \
    TOSTRING(LIBUSBDVD_VERSION_MAJOR) "." \
    TOSTRING(LIBUSBDVD_VERSION_MINOR) "." \
    TOSTRING(LIBUSBDVD_VERSION_MICRO)

#include <stdbool.h>
#include <stdint.h>

typedef struct{
	bool mounted;
	char disc_fstype[0x80];
	char mountpoint[0x10];
	char volid[0xff];
	/* FS Specific Data*/
	int jolietver; // Joliet Version
}usbdvd_fs_struct;

typedef struct{
	bool drive_found;
	bool fileimage;
	char vendor_id[0x8+1];
    char product_id[0x10+1];
    char product_revision[0x4+1];
    char serial_number[0x8+1];
	char disc_type[0x20];
	uint64_t disc_size;
	usbdvd_fs_struct fs;
}usbdvd_drive_struct;


typedef struct usbdvd_obj usbdvd_obj;

usbdvd_obj* usbdvd_init();
usbdvd_obj* usbdvd_initimage(const char * _path);
void usbdvd_destroy(usbdvd_obj* obj);
void usbdvd_eject(usbdvd_obj* obj);
int usbdvd_mountdisc(usbdvd_obj* obj);
usbdvd_drive_struct * usbdvd_get_drivectx(usbdvd_obj* obj);
const char* usbdvd_version(void);

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
class CUSBDVD_UDFFS;
class SWITCH_UDFFS;


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
	CUSBDVD_UDFFS * USBDVD_UDFFS = nullptr;
	SWITCH_UDFFS *SWITCH_UDFDEVOPTAB = nullptr;
	std::string get_version();
	
	int drive_status = 0;
	
	usbdvd_drive_struct usbdvd_drive_ctx;
	
	void Eject();
	int MountDisc();
	
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

