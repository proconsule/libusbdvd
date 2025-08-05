#ifndef USBDVD_SCSI_H
#define USBDVD_SCSI_H


#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <mutex>


#include "switch_usb.h"

    
    
typedef struct {
    struct {
        uint8_t peripheral_device_type : 5;
        uint8_t peripheral_qualifier   : 3;
    };
    struct {
        uint8_t reserved_1 : 7;
        uint8_t rmb        : 1;
    };
    uint8_t version;
    struct {
        uint8_t response_data_format : 4;
        uint8_t hisup                : 1;
        uint8_t naca                 : 1;
        uint8_t reserved_2           : 2;
    };
    uint8_t additional_length;
    struct {
        uint8_t protect    : 1;
        uint8_t reserved_3 : 2;
        uint8_t _3pc       : 1;
        uint8_t tpgs       : 2;
        uint8_t acc        : 1;
        uint8_t sccs       : 1;
    };
    struct {
        uint8_t reserved_4 : 4;
        uint8_t multip     : 1;
        uint8_t vs_1       : 1;
        uint8_t encserv    : 1;
        uint8_t reserved_5 : 1;
    };
    struct {
        uint8_t vs_2       : 1;
        uint8_t cmdque     : 1;
        uint8_t reserved_6 : 6;
    };
    char vendor_id[0x8];
    char product_id[0x10];
    char product_revision[0x4];
    char serial_number[0x8];
} ScsiInquiryStandardData;

typedef struct {
    uint8_t response_code;
    uint8_t segment_number;
    struct {
        uint8_t sense_key  : 4;
        uint8_t reserved_1 : 1;
        uint8_t ili        : 1;
        uint8_t eom        : 1;
        uint8_t file_mark  : 1;
    };
    uint8_t information[0x4];
    uint8_t additional_sense_length;
    uint8_t cmd_specific_info[0x4];
    uint8_t additional_sense_code;
    uint8_t additional_sense_code_qualifier;
    uint8_t field_replaceable_unit_code;
    uint8_t sense_key_specific[0x3];
} ScsiRequestSenseDataFixedFormat;

typedef enum {
    ScsiSenseKey_NoSense        = 0x00,
    ScsiSenseKey_RecoveredError = 0x01,
    ScsiSenseKey_NotReady       = 0x02,
    ScsiSenseKey_MediumError    = 0x03,
    ScsiSenseKey_HardwareError  = 0x04,
    ScsiSenseKey_IllegalRequest = 0x05,
    ScsiSenseKey_UnitAttention  = 0x06,
    ScsiSenseKey_DataProtect    = 0x07,
    ScsiSenseKey_BlankCheck     = 0x08,
    ScsiSenseKey_VendorSpecific = 0x09,
    ScsiSenseKey_CopyAborted    = 0x0A,
    ScsiSenseKey_AbortedCommand = 0x0B,
    ScsiSenseKey_Reserved       = 0x0C,
    ScsiSenseKey_VolumeOverflow = 0x0D,
    ScsiSenseKey_Miscompare     = 0x0E,
    ScsiSenseKey_Completed      = 0x0F
} ScsiSenseKey;


typedef enum {
    // Profili non conformi
    ATAPI_PROFILE_NON_REMOVABLE_DISK = 0x01,
    ATAPI_PROFILE_REMOVABLE_DISK = 0x02,
    ATAPI_PROFILE_MO_ERASABLE = 0x03,
    ATAPI_PROFILE_MO_WRITE_ONCE = 0x04,
    ATAPI_PROFILE_AS_MO = 0x05,
    
    // Profili CD
    ATAPI_PROFILE_CD_ROM = 0x08,
    ATAPI_PROFILE_CD_R = 0x09,
    ATAPI_PROFILE_CD_RW = 0x0A,
    
    // Profili DVD
    ATAPI_PROFILE_DVD_ROM = 0x10,
    ATAPI_PROFILE_DVD_R_SEQUENTIAL = 0x11,
    ATAPI_PROFILE_DVD_RAM = 0x12,
    ATAPI_PROFILE_DVD_RW_RESTRICTED_OVERWRITE = 0x13,
    ATAPI_PROFILE_DVD_RW_SEQUENTIAL = 0x14,
    ATAPI_PROFILE_DVD_R_DL_SEQUENTIAL = 0x15,
    ATAPI_PROFILE_DVD_R_DL_JUMP_RECORDING = 0x16,
    ATAPI_PROFILE_DVD_RW_DL = 0x17,
    ATAPI_PROFILE_DVD_DOWNLOAD_DISC = 0x18,
    
    // Profili DVD+
    ATAPI_PROFILE_DVD_PLUS_RW = 0x1A,
    ATAPI_PROFILE_DVD_PLUS_R = 0x1B,
    ATAPI_PROFILE_DVD_PLUS_RW_DL = 0x2A,
    ATAPI_PROFILE_DVD_PLUS_R_DL = 0x2B,
    
    // Profili HD DVD
    ATAPI_PROFILE_HD_DVD_ROM = 0x30,
    ATAPI_PROFILE_HD_DVD_R = 0x31,
    ATAPI_PROFILE_HD_DVD_RAM = 0x32,
    ATAPI_PROFILE_HD_DVD_RW = 0x33,
    ATAPI_PROFILE_HD_DVD_R_DL = 0x38,
    ATAPI_PROFILE_HD_DVD_RW_DL = 0x3A,
    
    // Profili Blu-ray
    ATAPI_PROFILE_BD_ROM = 0x40,
    ATAPI_PROFILE_BD_R_SEQUENTIAL = 0x41,
    ATAPI_PROFILE_BD_R_RANDOM = 0x42,
    ATAPI_PROFILE_BD_RE = 0x43,
    ATAPI_PROFILE_BD_R_DL_SEQUENTIAL = 0x50,
    ATAPI_PROFILE_BD_R_DL_JUMP_RECORDING = 0x51,
    ATAPI_PROFILE_BD_RE_DL = 0x52,
    
    // Profilo non standard/non valido
    ATAPI_PROFILE_NONE = 0x00,
    ATAPI_PROFILE_UNKNOWN = 0xFFFF
} atapi_profile_t;

typedef struct {
    uint32_t dCBWSignature;
    uint32_t dCBWTag;
    uint32_t dCBWDataTransferLength;
    uint8_t bmCBWFlags;
    uint8_t bCBWLUN;
    uint8_t bCBWCBLength;
    uint8_t CBWCB[16];
} __attribute__((packed)) CBW;

typedef struct {
    uint32_t dCSWSignature;
    uint32_t dCSWTag;
    uint32_t dCSWDataResidue;
    uint8_t bCSWStatus;
} __attribute__((packed)) CSW;

class CUSBSCSI{
public:
	CUSBSCSI(CSWITCH_USB * _usb_ctx);
	~CUSBSCSI();
	
	int UsbDvdSendInquiry(uint8_t lun,uint16_t allocation_length, void *buf);
	int UsbDvdSendTOC(uint8_t lun,  void **buf, int *bufsize);
	int UsbDvdReadCD_Audio(uint8_t lun,uint32_t read_lba,uint16_t numsec,uint8_t *data);
	int UsbDvdReadCD_Data(uint8_t lun,uint32_t read_lba,uint16_t numsec,uint8_t *data);
	int USBScsiInit();
	void USBScsi_Free();
	int UsbDvdSetSpeed(uint8_t lun);
	int UsbDvdDiscInfo(uint8_t lun,void *buf);
	int UsbDvdUnitReady(uint8_t lun);
	int UsbDvdReadAhead(uint8_t lun,uint32_t read_lba,uint16_t numsec);
	int UsbDvdSense(uint8_t lun,uint16_t allocation_length, ScsiRequestSenseDataFixedFormat *sense_data);
	int UsbDvdPreventMediumRemoval(uint8_t lun,uint32_t prevent);
	int UsbDvdGetConfig(uint8_t lun,uint8_t *buf);
	int send_scsi_command(CBW *cbw,bool receive,void *buf);
	
private:
	CSWITCH_USB *usb_ctx;
	std::mutex usb_mutex;
	
};







#endif /* USBDVD_SCSI_H */

