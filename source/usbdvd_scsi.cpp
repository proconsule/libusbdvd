#include "usbdvd_scsi.h"
#include "usbdvd_utils.h"
#include "usbdvd_css.h"


typedef struct{
    uint8_t operation;
    uint8_t flags;
    uint32_t lba;
    uint8_t group;
    char length;
    uint8_t control;
} __attribute__((packed)) SCSI_CDB_10 ;

#define CBW_SIGNATURE 0x43425355
#define CSW_SIGNATURE 0x53425355

#define ScsiCommandOperationCode_Inquiry 0x12
#define SCSI_READ_TOC 0x43
#define SCSI_READ_CD 0xbe
#define SCSI_ASC_MEDIUM_NOT_PRESENT 0x3A
#define AUDIOCD_SECOTR_SIZE 2352
#define DATA_SECTOR_SIZE 2048

typedef enum {
    ScsiCommandStatus_Passed     = 0x00,
    ScsiCommandStatus_Failed     = 0x01,
    ScsiCommandStatus_PhaseError = 0x02
} ScsiCommandStatus;

static void CreateCommandBlockWrapper(CBW *cbw, uint32_t data_size, bool data_in, uint8_t lun, uint8_t cb_size)
{
    
    if (!cbw) return;
    cbw->dCBWSignature = CBW_SIGNATURE;
    randomGet(&(cbw->dCBWTag), sizeof(cbw->dCBWTag));
    cbw->dCBWDataTransferLength = data_size;
    cbw->bmCBWFlags = (data_in ? USB_ENDPOINT_IN : USB_ENDPOINT_OUT);
    cbw->bCBWLUN = lun;
    cbw->bCBWCBLength = cb_size;
    
}

int CUSBSCSI::UsbDvdSendInquiry(uint8_t lun,uint16_t allocation_length, void *buf){
    
    CBW cbw = {0};
    memset(&cbw,0,sizeof(CBW));
    CreateCommandBlockWrapper(&cbw,allocation_length,true,0,6);
    
    uint16_t allocation_length_bigendian = __builtin_bswap16(allocation_length);
    cbw.CBWCB[0] = 0x12;            
    cbw.CBWCB[1] = 0;                 
    cbw.CBWCB[2] = 0;
    memcpy(&(cbw.CBWCB[3]), &allocation_length_bigendian, sizeof(uint16_t));
    
    
    return send_scsi_command(&cbw,true,buf);
    
    
}

int CUSBSCSI::UsbDvdGetConfig(uint8_t lun,uint8_t *buf)
{
    
    CBW cbw = {0};
    memset(&cbw,0,sizeof(CBW));
    CreateCommandBlockWrapper(&cbw,0x08,true,0,12);
    
    cbw.CBWCB[0] = 0x46;            
    cbw.CBWCB[1] = 0x01;                 
    cbw.CBWCB[8] = 0x08;                 
    
    
    
    return send_scsi_command(&cbw,true,buf);
    
    
}


int CUSBSCSI::UsbDvdGetCapacity(uint8_t lun,uint8_t *buf)
{
    
    CBW cbw = {0};
    memset(&cbw,0,sizeof(CBW));
    CreateCommandBlockWrapper(&cbw,0x08,true,0,12);
    
    cbw.CBWCB[0] = 0x25;            
    
    
    return send_scsi_command(&cbw,true,buf);
    
    
}

int CUSBSCSI::UsbDvdGetAGID(uint8_t lun,uint8_t *buf)
{
    
    CBW cbw = {0};
    memset(&cbw,0,sizeof(CBW));
    CreateCommandBlockWrapper(&cbw,0x08,true,0,12);
    
    cbw.CBWCB[0] = 0xA4;      
    cbw.CBWCB[5] = 0x00;
    cbw.CBWCB[9] = 0x08;  
    
    
    
    return send_scsi_command(&cbw,true,buf);
    
    
}


int CUSBSCSI::UsbDvdSendChallenge(uint8_t lun,uint8_t* challenge_seed,uint8_t _agid)
{
    
    CBW cbw = {0};
    memset(&cbw,0,sizeof(CBW));
    CreateCommandBlockWrapper(&cbw,0x10,false,0,12);
    
    cbw.CBWCB[0] = 0xA3;      
    cbw.CBWCB[9] = 0x10;  
    cbw.CBWCB[10] =  0x01 | (_agid << 6);
    
    uint8_t challenge_data[16];

    // Header dei dati
    challenge_data[0] = 0x00;           // Data Length MSB
    challenge_data[1] = 0x0e;           // Data Length LSB (10 bytes di challenge key)
    challenge_data[2] = 0x00;          
    challenge_data[3] = 0x00;           


    challenge_data[14] = 0x00;          // Challenge byte 9
    challenge_data[15] = 0x00;          // Challenge byte 9

    memcpy(challenge_data+4,challenge_seed,10);
    
    //int ret = send_scsi_command(&cbw,false,NULL);
    unsigned int transferred;
    int ret = usb_ctx->usb_bulk_transfer(false,&cbw,sizeof(CBW),&transferred,5000);
    

    ret = usb_ctx->usb_bulk_transfer(false,challenge_data,0x10,&transferred,5000);
    
    
    uint32_t cswtrans = 0;
    CSW csw = {0};    
    int r = usb_ctx->usb_bulk_transfer(true,(uint8_t*)&csw,sizeof(CSW),&cswtrans,5000);
    if(cswtrans==sizeof(CSW)){
            
            
        if (r < 0) {
            return r;
        }

                
        if (csw.dCSWSignature != CSW_SIGNATURE) {
            printf("SIG ERROR\r\n");
            return -1;
        }
        if (csw.dCSWTag != cbw.dCBWTag) {
            printf("TAG ERROR\r\n");
            return -1;
        }
        if (csw.bCSWStatus != 0) {
            printf("STATUS ERROR\r\n");
            return -1;
        }
    }
    
    
    return ret;
}

int CUSBSCSI::SendKey2(uint8_t lun,uint8_t _agid,uint8_t *_key2){
    
    CBW cbw = {0};
    memset(&cbw,0,sizeof(CBW));
    CreateCommandBlockWrapper(&cbw,12,false,0,12);
    
    cbw.CBWCB[0] = 0xA3;      
    cbw.CBWCB[9] = 12;  
    cbw.CBWCB[10] =  0x03 | (_agid << 6);
    
    uint8_t challenge_data[12];
    memset(challenge_data,0,12);
    
    // Header dei dati
    challenge_data[0] = 0x00;           // Data Length MSB
    challenge_data[1] = 0x0a;           // Data Length LSB (10 bytes di challenge key)
    challenge_data[2] = 0x00;          
    challenge_data[3] = 0x00;           


    

    memcpy(challenge_data+4,_key2,5);
    
    //int ret = send_scsi_command(&cbw,false,NULL);
    unsigned int transferred;
    int ret = usb_ctx->usb_bulk_transfer(false,&cbw,sizeof(CBW),&transferred,5000);
    

    ret = usb_ctx->usb_bulk_transfer(false,challenge_data,0x0c,&transferred,5000);
    
    
    uint32_t cswtrans = 0;
    CSW csw = {0};    
    int r = usb_ctx->usb_bulk_transfer(true,(uint8_t*)&csw,sizeof(CSW),&cswtrans,5000);
    if(cswtrans==sizeof(CSW)){
            
            
        if (r < 0) {
            return r;
        }

                
        if (csw.dCSWSignature != CSW_SIGNATURE) {
            printf("SIG ERROR\r\n");
            return -1;
        }
        if (csw.dCSWTag != cbw.dCBWTag) {
            printf("TAG ERROR\r\n");
            return -1;
        }
        if (csw.bCSWStatus != 0) {
            printf("STATUS ERROR\r\n");
            return -1;
        }
    }
    
    
    return ret;
}

int CUSBSCSI::ReportKey1(uint8_t lun,uint8_t _agid,uint8_t *buf){
    
    CBW cbw = {0};
    memset(&cbw,0,sizeof(CBW));
    CreateCommandBlockWrapper(&cbw,0x0c,true,0,12);
    
    cbw.CBWCB[0] = 0xA4;           
    cbw.CBWCB[8] = 0x00;  
    cbw.CBWCB[9] = 0x0c;  
    cbw.CBWCB[10] = (_agid << 6) | 0x02;
    
    
    return send_scsi_command(&cbw,true,buf);
}

int CUSBSCSI::UsbDvdReportChallenge(uint8_t lun,uint8_t _agid,uint8_t *buf)
{
    
    CBW cbw = {0};
    memset(&cbw,0,sizeof(CBW));
    CreateCommandBlockWrapper(&cbw,0x10,true,0,12);
    
    cbw.CBWCB[0] = 0xA4;           
    cbw.CBWCB[8] = 0x00;  
    cbw.CBWCB[9] = 0x10;  
    cbw.CBWCB[10] = (_agid << 6) | 0x01;
    
    
    return send_scsi_command(&cbw,true,buf);
    
    
}

int CUSBSCSI::UsbDvdInvalidateAGID(uint8_t lun,uint8_t _agid){
    CBW cbw = {0};
    memset(&cbw,0,sizeof(CBW));
    CreateCommandBlockWrapper(&cbw,0x08,false,0,12);
    
    cbw.CBWCB[0] = 0xA3; 
    cbw.CBWCB[10] = 0x3F | (_agid << 6) ;
    
    return send_scsi_command(&cbw,false,NULL);
    
}

int CUSBSCSI::GetASF(uint8_t lun,uint8_t *buf){
    
    CBW cbw = {0};
    memset(&cbw,0,sizeof(CBW));
    CreateCommandBlockWrapper(&cbw,0x8,true,0,12);
    
    cbw.CBWCB[0] = 0xA4;           
    cbw.CBWCB[8] = 0x00; 
    cbw.CBWCB[9] = 0x08;    
    cbw.CBWCB[10] = 0x05;
    
    
    return send_scsi_command(&cbw,true,buf);
    
    
}


int CUSBSCSI::UsbDvdGetDiscKey(uint8_t lun,uint8_t _agid,uint8_t *buf)
{
    
    CBW cbw = {0};
    memset(&cbw,0,sizeof(CBW));
    CreateCommandBlockWrapper(&cbw,0x0804,true,0,12);
    
    cbw.CBWCB[0] = 0xAD;      
    cbw.CBWCB[7] = 0x02;
    cbw.CBWCB[8] = 0x08;
    cbw.CBWCB[9] = 0x04;
    cbw.CBWCB[10] = _agid << 6;
    
    
    return send_scsi_command(&cbw,true,buf);
    
    
}

int CUSBSCSI::UsbDvdSetSpeed(uint8_t lun)
{
    
    CBW cbw = {0};
    memset(&cbw,0,sizeof(CBW));
    CreateCommandBlockWrapper(&cbw,0,false,0,12);
    
    cbw.CBWCB[0] = 0xBB;            
    cbw.CBWCB[1] = 0;                 
    cbw.CBWCB[2] = 0x0E;
    cbw.CBWCB[3] = 0x10;
    cbw.CBWCB[4] = 0xFF;
    cbw.CBWCB[5] = 0xFF;
    
    
    return send_scsi_command(&cbw,false,NULL);
    
    
}

int CUSBSCSI::UsbDvdDiscInfo(uint8_t lun,void *buf){
    CBW cbw = {0};
    memset(&cbw,0,sizeof(CBW));
    CreateCommandBlockWrapper(&cbw,0x22,true,0,12);
    
    cbw.CBWCB[0] = 0x2b;            
    cbw.CBWCB[11] = 0x22;
    
    return send_scsi_command(&cbw,true,buf);
    
}

int CUSBSCSI::UsbDvdReadDVDStructure(uint8_t lun,uint8_t _format,uint16_t allocation_length, void *buf){
    CBW cbw = {0};
    memset(&cbw,0,sizeof(CBW));
    CreateCommandBlockWrapper(&cbw,allocation_length,true,0,12);
    cbw.CBWCB[0] = 0xAD;            
    cbw.CBWCB[7] = _format;
    cbw.CBWCB[8] = (allocation_length >> 8) & 0xFF;
    cbw.CBWCB[9] = allocation_length & 0xFF;
    
    
    return send_scsi_command(&cbw,true,buf);
}

int CUSBSCSI::UsbDvdSendPlayerKey(uint8_t lun,uint8_t _agid,uint8_t * _playerkey){
    
    uint32_t transferred = 0;
    CBW cbw = {0};
    memset(&cbw,0,sizeof(CBW));
    CreateCommandBlockWrapper(&cbw,0,false,0,12);
    cbw.CBWCB[0] = 0xA3;
    cbw.CBWCB[8] = 0x00;
    cbw.CBWCB[9] = 0x0c;
    cbw.CBWCB[10] = 0x01 | (_agid << 6) ;
    int r = usb_ctx->usb_bulk_transfer(false,&cbw,sizeof(CBW),&transferred,5000);
    
    uint8_t pk_payload[12];
    memset(pk_payload,0,sizeof(pk_payload));
    pk_payload[0] = 0x00;
    pk_payload[1] = 0x0a;
    
    pk_payload[4] = _playerkey[0];
    pk_payload[5] = _playerkey[1];
    pk_payload[6] = _playerkey[2];
    pk_payload[7] = _playerkey[3];
    pk_payload[8] = _playerkey[5];
    r = usb_ctx->usb_bulk_transfer(false,pk_payload,sizeof(pk_payload),&transferred,5000);
    CSW csw = {0};
    if(1){
  
        uint32_t cswtrans = 0;
        
        r = usb_ctx->usb_bulk_transfer(true,(uint8_t*)&csw,sizeof(CSW),&cswtrans,5000);
        if(cswtrans==sizeof(CSW)){
            //uint8_t *testcsw = (uint8_t *)&csw;
            /*
            usbdvd_log("CSW: ");
            for(int i=0;i<(int)sizeof(CSW);i++){
                usbdvd_log("%02hhx ",testcsw[i]);
            }
            usbdvd_log("\r\n");
            */
            if (r < 0) {
                return r;
            }

            
            if (csw.dCSWSignature != CSW_SIGNATURE) {
                //usbdvd_log("SIG ERROR\r\n");
                return -1;
            }
            if (csw.dCSWTag != cbw.dCBWTag) {
                //usbdvd_log("TAG ERROR\r\n");
                return -1;
            }
            if (csw.bCSWStatus != 0) {
                
                ScsiRequestSenseDataFixedFormat testsense = {0};
                
                UsbDvdSense(0,sizeof(ScsiRequestSenseDataFixedFormat),&testsense);
                //usbdvd_log("STATUS: %02hhx\r\n",testsense.sense_key);
                switch(testsense.sense_key)
                {
                    case ScsiSenseKey_NoSense:
                    case ScsiSenseKey_RecoveredError:
                    case ScsiSenseKey_UnitAttention:
                    case ScsiSenseKey_Completed:
                        return 0;
                        break;
                    case ScsiSenseKey_NotReady:
                        if (testsense.additional_sense_code == SCSI_ASC_MEDIUM_NOT_PRESENT){
                            //usbdvd_log("No Medium on drive\r\n");
                            return -2;
                            break;
                        }
                    default:
                        //usbdvd_log("Unrecoverable error");
                        break;
                }
                //usbdvd_log("STATUS ERROR\r\n");
                return -1;
            }
        }
    }
    return 0;

}



int CUSBSCSI::UsbDvdUnitReady(uint8_t lun){
    CBW cbw = {0};
    memset(&cbw,0,sizeof(CBW));
    CreateCommandBlockWrapper(&cbw,0,false,0,12);
    CSW csw = {0};
    CBW * mycbw = &cbw;
    
    uint32_t transferred = 0;
    
    int r = usb_ctx->usb_bulk_transfer(false,mycbw,sizeof(CBW),&transferred,5000);
    /*
    uint8_t *testcbw = (uint8_t*)mycbw;
    
    usbdvd_log("CBW : ");
    for(int i=0;i<(int)sizeof(CBW);i++){
        usbdvd_log("%02hhx ",testcbw[i]);
    }
    usbdvd_log("\r\n");
    */
    if (r < 0) {
        return r;
    }
    
    if(1){
  
        uint32_t cswtrans = 0;
        
        r = usb_ctx->usb_bulk_transfer(true,(uint8_t*)&csw,sizeof(CSW),&cswtrans,5000);
        if(cswtrans==sizeof(CSW)){
            //uint8_t *testcsw = (uint8_t *)&csw;
            /*
            usbdvd_log("CSW: ");
            for(int i=0;i<(int)sizeof(CSW);i++){
                usbdvd_log("%02hhx ",testcsw[i]);
            }
            usbdvd_log("\r\n");
            */
            if (r < 0) {
                return r;
            }

            
            if (csw.dCSWSignature != CSW_SIGNATURE) {
                //usbdvd_log("SIG ERROR\r\n");
                return -1;
            }
            if (csw.dCSWTag != mycbw->dCBWTag) {
                //usbdvd_log("TAG ERROR\r\n");
                return -1;
            }
            if (csw.bCSWStatus != 0) {
                
                ScsiRequestSenseDataFixedFormat testsense = {0};
                
                UsbDvdSense(0,sizeof(ScsiRequestSenseDataFixedFormat),&testsense);
                //usbdvd_log("STATUS: %02hhx\r\n",testsense.sense_key);
                switch(testsense.sense_key)
                {
                    case ScsiSenseKey_NoSense:
                    case ScsiSenseKey_RecoveredError:
                    case ScsiSenseKey_UnitAttention:
                    case ScsiSenseKey_Completed:
                        return 0;
                        break;
                    case ScsiSenseKey_NotReady:
                        if (testsense.additional_sense_code == SCSI_ASC_MEDIUM_NOT_PRESENT){
                            //usbdvd_log("No Medium on drive\r\n");
                            return -2;
                            break;
                        }
                    default:
                        //usbdvd_log("Unrecoverable error");
                        break;
                }
                //usbdvd_log("STATUS ERROR\r\n");
                return -1;
            }
        }
    }
    
    return 0;
}

int CUSBSCSI::UsbDvdPreventMediumRemoval(uint8_t lun,uint32_t prevent){
    CBW cbw = {0};
    memset(&cbw,0,sizeof(CBW));
    CreateCommandBlockWrapper(&cbw,0,false,0,12);
    
    cbw.CBWCB[0] = 0x1e;            
    cbw.CBWCB[4] = prevent; 
    
    return send_scsi_command(&cbw,false,NULL);
    
}

int CUSBSCSI::UsbDvdSense(uint8_t lun,uint16_t allocation_length, ScsiRequestSenseDataFixedFormat *sense_data){
    CBW cbw = {0};
    memset(&cbw,0,sizeof(CBW));
    CreateCommandBlockWrapper(&cbw,allocation_length,true,0,12);
    
    cbw.CBWCB[0] = 0x03;            
    cbw.CBWCB[4] = (uint8_t)cbw.dCBWDataTransferLength;
    
    
    return send_scsi_command(&cbw,true,sense_data);
    
}


int CUSBSCSI::UsbDvdSendTOC(uint8_t lun,  void **buf, int *bufsize){
    
    
    CBW cbw = {0};
    memset(&cbw,0,sizeof(CBW));
    CreateCommandBlockWrapper(&cbw,4,true,0,12);
    
    cbw.CBWCB[0] = SCSI_READ_TOC;
    cbw.CBWCB[1] = 0x02;
    cbw.CBWCB[2] = 0x00;
    cbw.CBWCB[3] = 0x00;
    cbw.CBWCB[4] = 0x00;
    cbw.CBWCB[5] = 0x00;
    cbw.CBWCB[6] = 0x00;
    cbw.CBWCB[7] = 0x00;
    cbw.CBWCB[8] = 0x04;
    cbw.CBWCB[9] = 0x00;
    
    uint8_t mytocdata[4];
    
    int r1 = send_scsi_command(&cbw,true,mytocdata);
    
    
    if(r1!=0)return -1;
    
    int tocsize = (mytocdata[0] << 8 & 0xFFFF) | mytocdata[1];
    tocsize = tocsize;
    
    
    CBW cbw2 = {0};
    memset(&cbw2,0,sizeof(CBW));
    CreateCommandBlockWrapper(&cbw2,tocsize,true,0,12);
    
    cbw2.CBWCB[0] = SCSI_READ_TOC;
    cbw2.CBWCB[1] = 0x02;
    cbw2.CBWCB[2] = 0x00;
    cbw2.CBWCB[3] = 0x00;
    cbw2.CBWCB[4] = 0x00;
    cbw2.CBWCB[5] = 0x00;
    cbw2.CBWCB[6] = 0x00;
    cbw2.CBWCB[7] = (tocsize >> 8) & 0xFF;
    cbw2.CBWCB[8] = tocsize  & 0xFF;
    
    uint8_t fulltocdata[tocsize];
    
    r1 = send_scsi_command(&cbw2,true,fulltocdata);
   
    if(r1!=0)return -1;
    *buf = malloc(tocsize*sizeof(uint8_t));
    *bufsize = tocsize;
    memcpy(*buf,fulltocdata,tocsize);
    return 0;
}


int CUSBSCSI::UsbDvdReadCD_Audio(uint8_t lun,uint32_t read_lba,uint16_t numsec,uint8_t *data){
    
    CBW cbw = {0};
    CreateCommandBlockWrapper(&cbw,numsec*AUDIOCD_SECOTR_SIZE,true,0,12);
    
    
    
    cbw.CBWCB[0] = SCSI_READ_CD;
    cbw.CBWCB[1] = 0x04; 
    cbw.CBWCB[2] = (read_lba >> 24) & 0xFF;
    cbw.CBWCB[3] = (read_lba >> 16) & 0xFF;
    cbw.CBWCB[4] = (read_lba >> 8) & 0xFF;
    cbw.CBWCB[5] = read_lba & 0xFF;
    cbw.CBWCB[7] = (numsec >> 8) & 0xFF;
    cbw.CBWCB[8] = numsec & 0xFF;
    cbw.CBWCB[9] = 0x10;      
    cbw.CBWCB[10] = 0x00;  
    cbw.CBWCB[11] = 0x00;  

    return send_scsi_command(&cbw,true,data);
}

int CUSBSCSI::UsbDvd_Eject(uint8_t lun){
    
    CBW cbw = {0};
    CreateCommandBlockWrapper(&cbw,0,false,0,12);

    
    cbw.CBWCB[0] = 0x1b;            
    cbw.CBWCB[1] = 0x00;   
    cbw.CBWCB[2] = 0x00;
    cbw.CBWCB[3] = 0x00;
    cbw.CBWCB[4] = 0x02;
    cbw.CBWCB[5] = 0x00;
    

    return send_scsi_command(&cbw,false,NULL);
}

int CUSBSCSI::UsbDvdReadCD_Data(uint8_t lun,uint32_t read_lba,uint16_t numsec,uint8_t *data){
    
    CBW cbw = {0};
    CreateCommandBlockWrapper(&cbw,numsec*DATA_SECTOR_SIZE,true,0,12);

    
    cbw.CBWCB[0] = 0xa8;            
    cbw.CBWCB[2] = (read_lba >> 24) & 0xFF; 
    cbw.CBWCB[3] = (read_lba >> 16) & 0xFF;
    cbw.CBWCB[4] = (read_lba >> 8) & 0xFF;
    cbw.CBWCB[5] = read_lba & 0xFF;
    cbw.CBWCB[6] = (numsec >> 24) & 0xFF; 
    cbw.CBWCB[7] = (numsec >> 16) & 0xFF;
    cbw.CBWCB[8] = (numsec >> 8) & 0xFF;
    cbw.CBWCB[9] = numsec & 0xFF;
    cbw.CBWCB[10] = 0x00;  
    cbw.CBWCB[11] = 0x00;  

    return send_scsi_command(&cbw,true,data);
}

int CUSBSCSI::UsbDvdReadAhead(uint8_t lun,uint32_t read_lba,uint16_t numsec){
    CBW cbw = {0};
    memset(&cbw,0,sizeof(CBW));
    CreateCommandBlockWrapper(&cbw,0,false,0,12);
    
    cbw.CBWCB[0] = 0xa7;            
    cbw.CBWCB[2] = (read_lba >> 24) & 0xFF; 
    cbw.CBWCB[3] = (read_lba >> 16) & 0xFF;
    cbw.CBWCB[4] = (read_lba >> 8) & 0xFF;
    cbw.CBWCB[5] = read_lba & 0xFF;
    cbw.CBWCB[6] = (numsec >> 24) & 0xFF; 
    cbw.CBWCB[7] = (numsec >> 16) & 0xFF;
    cbw.CBWCB[8] = (numsec >> 8) & 0xFF;
    cbw.CBWCB[9] = numsec & 0xFF;
    cbw.CBWCB[10] = 0x00;  
    cbw.CBWCB[11] = 0x00;  
    
    return send_scsi_command(&cbw,false,NULL);
    
}

int CUSBSCSI::send_scsi_command(CBW *cbw,bool receive,void *buf){
    int r = 0;
    uint32_t transferred = 0;
    
    CSW csw = {0};
    
    r = usb_ctx->usb_bulk_transfer(false,cbw,sizeof(CBW),&transferred,5000);
            if (r < 0) {
                return r;
            }
    if (cbw->dCBWDataTransferLength > 0 && receive) {
        
        r = usb_ctx->usb_bulk_transfer(true,buf,cbw->dCBWDataTransferLength,&transferred,10000);
        
        
            if (r < 0) {
            }
    }
    
    if(1){
  
        uint32_t cswtrans = 0;
        
        r = usb_ctx->usb_bulk_transfer(true,(uint8_t*)&csw,sizeof(CSW),&cswtrans,5000);
        if(cswtrans==sizeof(CSW)){
            
            
            if (r < 0) {
                return r;
            }

            
            if (csw.dCSWSignature != CSW_SIGNATURE) {
                usbdvd_log("SIG ERROR\r\n");
                return -1;
            }
            if (csw.dCSWTag != cbw->dCBWTag) {
                usbdvd_log("TAG ERROR\r\n");
                return -1;
            }
            if (csw.bCSWStatus != 0) {
                usbdvd_log("STATUS ERROR\r\n");
                return -1;
            }
        }
    }


    
    return 0;
}


CUSBSCSI::CUSBSCSI(CSWITCH_USB *_usb_ctx){
    this->usb_ctx = _usb_ctx;
    
}

CUSBSCSI::~CUSBSCSI(){
    
}

/* "BruteForce" Title Key extraction https://www.videolan.org/developers/libdvdcss.html */

int CUSBSCSI::CrackTitleKey( int i_pos, int i_len,uint8_t * p_titlekey )
{
    uint8_t       p_buf[ 2048 ];
    const uint8_t p_packstart[4] = { 0x00, 0x00, 0x01, 0xba };
    int i_reads = 0;
    int i_encrypted = 0;
    int b_stop_scanning = 0;
    //int b_read_error = 0;
    //int i_ret;

    usbdvd_log("cracking title key at block %i\r\n", i_pos );

    uint32_t i_tries = 0;
    uint32_t i_success = 0;

    do
    {
        //i_ret = dvdcss->pf_seek( dvdcss, i_pos );

        //if( i_ret != i_pos )
        //{
        //    printf("seek failed\r\n" );
        //}
        
        GetBusKey();
        
        int ret = UsbDvdReadCD_Data(0,i_pos,1,p_buf);
        //printf("RET: %d\r\n",ret);
        
        if(ret != 0) return -1;
        
        /* Stop when we find a non-MPEG stream block.
         * (We must have reached the end of the stream).
         * For now, allow all blocks that begin with a start code. */
        if( memcmp( p_buf, p_packstart, 3 ) )
        {
            usbdvd_log( "block %i is a non-MPEG block "
                                 "(end of title)\r\n", i_pos );
            break;
        }
        if( p_buf[0x0d] & 0x07 )
            usbdvd_log( "stuffing in pack header\r\n" );
        
        /* PES_scrambling_control does not exist in a system_header,
         * a padding_stream or a private_stream2 (and others?). */
        if( p_buf[0x14] & 0x30  && ! ( p_buf[0x11] == 0xbb
                                       || p_buf[0x11] == 0xbe
                                       || p_buf[0x11] == 0xbf ) )
        {
            
            i_encrypted++;

            if( AttackPattern( p_buf, p_titlekey,&i_success,&i_tries ) > 0 )
            {
                b_stop_scanning = 1;
            }
#if 0
            if( AttackPadding( p_buf ) > 0 )
            {
                b_stop_scanning = 1;
            }
#endif /* 0 */
        }
        i_pos++;
        i_len--;
        i_reads++;
        
        /* Emit a progress indication now and then. */
        if( !( i_reads & 0xfff ) )
        {
            usbdvd_log( "at block %i, still cracking...\r\n", i_pos );
        }

        /* Stop after 2000 blocks if we haven't seen any encrypted blocks. */
        if( i_reads >= 2000 && i_encrypted == 0 ) break;

    } while( !b_stop_scanning && i_len > 0);

    if( !b_stop_scanning )
    {
        usbdvd_log( "end of title reached\r\n" );
    }

    /* Print some statistics. */
    usbdvd_log( "successful attempts %d/%d, scrambled blocks %d/%d\r\n",
                         i_success, i_tries, i_encrypted, i_reads );

    if( i_success > 0 /* b_stop_scanning */ )
    {
        usbdvd_log( "Video Title Set (VTS) key initialized\r\n" );
        return 1;
    }

    if( i_encrypted == 0 && i_reads > 0 )
    {
        memset( p_titlekey, 0, 5 );
        usbdvd_log( "no scrambled sectors found\r\n" );
        return 0;
    }

    memset( p_titlekey, 0, 5 );
    return -1;
}


int CUSBSCSI::GetBusKey(){
    
    
    int ret = 0;
    uint8_t myagid[0x08];
    agid_struct agid = {0};
    ret = UsbDvdGetAGID(0,(uint8_t *)&myagid);
    if(ret != 0){
        return -1;
    }
    memcpy(&agid,myagid,sizeof(agid_struct));
                    
                    
    int resagid = agid.agid >> 6;
                    
                    
    uint8_t challenge_seed[10];
    uint8_t challenge_seed_host[10];
    for(int i=0;i<10;i++){
        challenge_seed[i]=i;
    }
    for(int i=0;i<10;i++){
        challenge_seed_host[9-i]=challenge_seed[i];
    }
    ret = UsbDvdSendChallenge(0,challenge_seed_host,resagid);
    if(ret != 0){
        UsbDvdInvalidateAGID(0,resagid);
        return -1;
    }
    css_chalkey_struct css_key1;
    ret = ReportKey1(0,resagid,(uint8_t *)&css_key1);
    if(ret != 0){
        UsbDvdInvalidateAGID(0,resagid);
        return -1;
    }
    uint8_t css_key1_host[5];
    for(int i=0;i<5;i++){
        css_key1_host[i] = css_key1.key[4-i];
    }
                    
    uint8_t p_key_check[5];
    uint8_t i_variant = 0;
    int i = 0;
    for(i=0;i<32;i++){
        CryptKey( 0, i, challenge_seed, p_key_check );
                        
        if( memcmp( p_key_check,css_key1_host, 5 ) == 0 ){
            i_variant = i;
            break;
        }
                        
    }
    if(i==32){
        usbdvd_log("CANNOT AUTH DRIVE\r\n");
        return -1;
    }
    css_chal2_struct css_chal2;
    UsbDvdReportChallenge(0,resagid,(uint8_t *)&css_chal2);
    uint8_t css_chal2_host[10];
    for(int i=0;i<10;i++){
        css_chal2_host[i] = css_chal2.key[9-i];
    }
                    
    uint8_t p_key2[5];
    uint8_t p_key2_host[5];
    CryptKey( 1, i_variant, css_chal2_host, p_key2 );
    for(int i=0;i<5;i++){
        p_key2_host[i] = p_key2[4-i];
    }
                    
    ret = SendKey2(0,resagid,p_key2_host);
    //printf("RET: %d\r\n",ret);
                    
                    
    uint8_t chal3[10];
    memcpy(chal3,css_key1.key,5);
    memcpy(chal3+5,p_key2,5);
                    
    uint8_t buskey[5];
    CryptKey( 2, i_variant, chal3, buskey );
                    
    // DISC KEY
                    
    uint8_t dvdkey_test[0x804];
    ret = UsbDvdGetDiscKey(0,resagid,dvdkey_test);
                    
                    
    uint8_t asf_ret[8];
    ret = GetASF(0,asf_ret);
    uint8_t myasf = asf_ret[7] & 1;   //ASF STATUS;
                    
    //printf("ASF FLAG: %hhu\r\n",myasf);
    if(myasf == 1)return 0;             
                    
    return -1;

    
    
}