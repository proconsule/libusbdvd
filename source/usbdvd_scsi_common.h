#ifndef USBDVD_SCSI_COMMON_H
#define USBDVD_SCSI_COMMON_H

#include <stdint.h>


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

#define CBW_SIGNATURE 0x43425355
#define CSW_SIGNATURE 0x53425355

void CreateCommandBlockWrapper(CBW *cbw, uint32_t data_size, bool data_in, uint8_t lun, uint8_t cb_size);


#endif