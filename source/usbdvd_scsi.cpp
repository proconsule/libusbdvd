#include "usbdvd_scsi.h"
#include "usbdvd_utils.h"


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
#define SCSI_ASC_MEDIUM_NOT_PRESENT	0x3A
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