#include "usbdvd.h"
#include "usbdvd_scsi.h"
#include <stdlib.h>

#include "switch_usb.h"
#include "cdaudio_devoptab.h"
#include "usbdvd_utils.h"
#include "audiocdfs.h"
#include "usbdvd_iso9660.h"
#include "iso9660_devoptab.h"
#include "usbdvd_common.h"


int UsbDVDGuessFsType(CDDVD_TOC * mytoc);

CUSBDVD::~CUSBDVD(){
	usbdvd_log("CUSBDVD DEINIT\r\n");
	if(USB_SCSI != nullptr){
		USB_SCSI->UsbDvdPreventMediumRemoval(0,0);
	}
	if(SWITCH_CDAUDIODEVOPTAB != nullptr)delete SWITCH_CDAUDIODEVOPTAB;
	if(SWITCH_ISO9660DEVOPTAB != nullptr)delete SWITCH_ISO9660DEVOPTAB;
	
	if(CDAUDIOFS != nullptr)delete CDAUDIOFS;
	if(ISO9660FS != nullptr)delete ISO9660FS;
	
	if(USB_SCSI != nullptr)delete USB_SCSI;
	if(SWITCH_USB!= nullptr)delete SWITCH_USB;

	

}
CUSBDVD::CUSBDVD(std::string _cuepath,std::string _binpath){
	cuepath = _cuepath;
	binpath = _binpath;
	fileimage = true;
		
}

CUSBDVD::CUSBDVD(std::string _isofilepath){
	isofilepath = _isofilepath;
	fileimage = true;
	
	ISO9660FS = new CUSBDVD_ISO9660FS(isofilepath);
	SWITCH_ISO9660DEVOPTAB = new SWITCH_ISO9660FS(ISO9660FS,"iso0","iso0:");
	this->mountpoint = "iso0:";
	if(pseudofs_init && cdfs_init && usb_init)datacd_init_ok = true;
	
	
}

CUSBDVD::CUSBDVD(){
	
	
	SWITCH_USB = new CSWITCH_USB();
	if(SWITCH_USB->device_found){
		USB_SCSI = new CUSBSCSI(SWITCH_USB);
	}
	
	if(USB_SCSI!=nullptr){
	
	
		
			
		ScsiInquiryStandardData test = {0};
		memset(&test,0,sizeof(ScsiInquiryStandardData));
		int ret = USB_SCSI->UsbDvdSendInquiry(0,sizeof(ScsiInquiryStandardData),(uint8_t *)&test);
		if(ret!=0)return;
		
		char chr_vendor_id[0x8+1];
		char chr_product_id[0x10+1];
		char chr_product_revision[0x4+1];
		char chr_serial_number[0x8+1];
		
		memset(chr_vendor_id,0,sizeof(chr_vendor_id));
		memset(chr_product_id,0,sizeof(chr_product_id));
		memset(chr_product_revision,0,sizeof(chr_product_revision));
		memset(chr_serial_number,0,sizeof(chr_serial_number));
		
		memcpy(chr_vendor_id,test.vendor_id,sizeof(test.vendor_id));
		memcpy(chr_product_id,test.product_id,sizeof(test.product_id));
		memcpy(chr_product_revision,test.product_revision,sizeof(test.product_revision));
		memcpy(chr_serial_number,test.serial_number,sizeof(test.serial_number));
		
		
		
		product_id = chr_product_id;
		product_revision = chr_product_revision;
		serial_number = chr_serial_number;
		vendor_id = chr_vendor_id;
		
		
		ret = USB_SCSI->UsbDvdUnitReady(0);
		drive_status = ret;
		
		
		if(drive_status!=0)return;
		
		usb_init = true;
		
		uint8_t *buf;
		int bufsize;
		
		uint8_t testconf[0x08];
		ret = USB_SCSI->UsbDvdGetConfig(0,testconf);
		if(ret == 0){
			switch (testconf[7]) {
				case ATAPI_PROFILE_CD_ROM:
				case ATAPI_PROFILE_CD_R:
				case ATAPI_PROFILE_CD_RW:
					disctype = "CD-ROM";
					break;
				case ATAPI_PROFILE_DVD_ROM:
				case ATAPI_PROFILE_DVD_R_SEQUENTIAL:
				case ATAPI_PROFILE_DVD_RAM:
				case ATAPI_PROFILE_DVD_RW_RESTRICTED_OVERWRITE:
				case ATAPI_PROFILE_DVD_RW_SEQUENTIAL:
				case ATAPI_PROFILE_DVD_R_DL_SEQUENTIAL:
				case ATAPI_PROFILE_DVD_R_DL_JUMP_RECORDING:
				case ATAPI_PROFILE_DVD_RW_DL:
				case ATAPI_PROFILE_DVD_DOWNLOAD_DISC:
					disctype = "DVD-ROM";
					break;
				default:
					disctype = "Unknown";
			}
		
		}
		
		USB_SCSI->UsbDvdPreventMediumRemoval(0,1);
		
		ret = USB_SCSI->UsbDvdSendTOC(0,(void **)&buf,&bufsize);
		if(ret != 0){
			if(buf)free(buf);
			return;
		}
		
		
		CDDVD_TOC toc;	
		memcpy(&toc,buf,bufsize);
    
		int myfstype = UsbDVDGuessFsType(&toc);
		
		
		
#ifdef DEBUG
		for(int i=0;i<toc.hdr.last_track;i++){
			usbdvd_log("Track %d %d\r\n",i+1,toc.tracks[i].tracktype);
		
		}
#endif
		
		
		if(myfstype == 1){
		
		
			CDAUDIOFS = new CAUDIOCD_PSEUDOFS(toc,USB_SCSI);
			if(buf)free(buf);
			if(CDAUDIOFS->CDAudioFound()){
				cdfs_init = true;

			
			SWITCH_CDAUDIODEVOPTAB = new SWITCH_AUDIOCDFS(CDAUDIOFS,"acd0","acd0:");
			pseudofs_init = true;
			this->mountpoint = "acd0:";

			}
			
			if(pseudofs_init && cdfs_init && usb_init)acd_init_ok = true;
		}else if(myfstype == 2){
			uint32_t mylba = ((toc.tracks[0].MIN*60)+toc.tracks[0].SEC)*75+toc.tracks[0].FRAME;
			uint32_t nextlba = ((toc.tracks[1].MIN*60)+toc.tracks[1].SEC)*75+toc.tracks[1].FRAME;
			
			ISO9660FS = new CUSBDVD_ISO9660FS(USB_SCSI,mylba,nextlba);
			cdfs_init = true;
			
			SWITCH_ISO9660DEVOPTAB = new SWITCH_ISO9660FS(ISO9660FS,"iso0","iso0:");
			pseudofs_init = true;
			this->mountpoint = "iso0:";
			if(pseudofs_init && cdfs_init && usb_init)datacd_init_ok = true;
			
		}
	
	}
	
}

int UsbDVDGuessFsType(CDDVD_TOC * mytoc){
	
	bool datatrack = false;
	bool audiotrack = false;
	for(int i=0;i<mytoc->hdr.last_track;i++){
		if(mytoc->tracks[i].tracktype == 1){
			datatrack = true;
		}else{
			audiotrack = true;
		}
	}
	
	if(datatrack && audiotrack)return -1;
	
	if(audiotrack) return 1;
	if(datatrack) return 2;
	
	
	
	return -1;
}

