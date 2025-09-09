#ifndef USBDVD_H
#define USBDVD_H


#ifdef __cplusplus
extern "C" {
#endif

#define LIBUSBDVD_VERSION_MAJOR    0
#define LIBUSBDVD_VERSION_MINOR    1
#define LIBUSBDVD_VERSION_MICRO    0

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

#define LIBUSBDVD_VERSION_STRING \
    TOSTRING(LIBUSBDVD_VERSION_MAJOR) "." \
    TOSTRING(LIBUSBDVD_VERSION_MINOR) "." \
    TOSTRING(LIBUSBDVD_VERSION_MICRO)

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    USBDVD_FS_Invalid = 0,
    USBDVD_FS_CDAUDIOFS   = 1,
    USBDVD_FS_ISO9660   = 2,
    USBDVD_FS_ISO9660_JOLIET   = 3,
    USBDVD_FS_ISO9660_RR = 4,
    USBDVD_FS_UTF = 5   
} USBDVD_FS_TYPES;


typedef struct{
    uint32_t udfver;
    uint32_t number_of_partition_maps;
    uint32_t metadata_partition_size;
    uint32_t metadata_extents;
}udf_extended_struct;

typedef struct{
    uint32_t version;
}joliet_extended_struct;

typedef struct{
    bool mounted;
    char disc_fstype[0x80];
    char mountpoint[0x10];
    char volid[0xff];
    uint32_t block_size;
    /* FS Specific Data*/
    USBDVD_FS_TYPES disc_fsidx;
    udf_extended_struct udf_extended;
    joliet_extended_struct joliet_extended;
    
}usbdvd_fs_struct;

typedef struct{
    bool fileimage;
    bool drive_found;
    char vendor_id[0x8+1];
    char product_id[0x10+1];
    char product_revision[0x4+1];
    char serial_number[0x8+1];
    char disc_type[0x20];
    uint64_t disc_size;
    
}usbdvd_drive_struct;


typedef struct{
    bool CSS;
    uint8_t regions;  //Inverted Bit Mask ex: Region 2 DVD: RMI = 0xFD (11111101 - only bit 1 is 0)
    uint32_t decss_msecs;
    uint32_t cache_keys;
    
}dvdprotection_struct;

typedef struct{
    bool AACS;
}blurayprotection_struct;

typedef struct{
    
    usbdvd_drive_struct drive;
    usbdvd_fs_struct fs;
    dvdprotection_struct dvd_protection;
    blurayprotection_struct bluray_protection;
    
    
}usbdvd_struct;


typedef struct usbdvd_obj usbdvd_obj;

usbdvd_obj* usbdvd_init();
usbdvd_obj* usbdvd_initimage(const char * _path);
usbdvd_obj* usbdvd_initcuebin(const char * _cuepath,const char * _binpath);
void usbdvd_destroy(usbdvd_obj* obj);
void usbdvd_eject(usbdvd_obj* obj);
int usbdvd_mountdisc(usbdvd_obj* obj);
usbdvd_struct * usbdvd_get_ctx(usbdvd_obj* obj);
const char* usbdvd_version(void);
void usbdvd_cache_ifo_files(usbdvd_obj* obj);

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
    
    usbdvd_struct usbdvd_ctx;
    
    void Cache_IFO_Files();
    
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

