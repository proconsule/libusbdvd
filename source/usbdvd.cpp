#include "usbdvd.h"
#include "usbdvd_scsi.h"
#include <stdlib.h>

#include "switch_usb.h"
#include "cdaudio_devoptab.h"
#include "usbdvd_utils.h"
#include "audiocdfs.h"
#include "usbdvd_iso9660.h"
#include "iso9660_devoptab.h"
#include "usbdvd_udf.h"
#include "usbdvd_common.h"
#include "udf_devoptab.h"


int UsbDVDGuessFsType(CDDVD_TOC * mytoc);

CUSBDVD::~CUSBDVD(){
	if(USB_SCSI != nullptr){
		USB_SCSI->UsbDvdPreventMediumRemoval(0,0);
	}
	if(SWITCH_CDAUDIODEVOPTAB != nullptr)delete SWITCH_CDAUDIODEVOPTAB;
	if(SWITCH_ISO9660DEVOPTAB != nullptr)delete SWITCH_ISO9660DEVOPTAB;
	if(SWITCH_UDFDEVOPTAB != nullptr)delete SWITCH_UDFDEVOPTAB;
	
	
	if(CDAUDIOFS != nullptr)delete CDAUDIOFS;
	if(ISO9660FS != nullptr)delete ISO9660FS;
	if(USBDVD_UDFFS != nullptr)delete USBDVD_UDFFS;
	
	if(USB_SCSI != nullptr)delete USB_SCSI;
	if(SWITCH_USB!= nullptr)delete SWITCH_USB;
	
	

}
CUSBDVD::CUSBDVD(std::string _cuepath,std::string _binpath){
	cuepath = _cuepath;
	binpath = _binpath;
	usbdvd_drive_ctx.fileimage = true;
		
}

CUSBDVD::CUSBDVD(std::string _isofilepath){
	isofilepath = _isofilepath;
	usbdvd_drive_ctx.drive_found = false;
	usbdvd_drive_ctx.fileimage = true;
	
	memset(usbdvd_drive_ctx.vendor_id,0,sizeof(usbdvd_drive_ctx.vendor_id));
	memset(usbdvd_drive_ctx.product_id,0,sizeof(usbdvd_drive_ctx.product_id));
	memset(usbdvd_drive_ctx.product_revision,0,sizeof(usbdvd_drive_ctx.product_revision));
	memset(usbdvd_drive_ctx.serial_number,0,sizeof(usbdvd_drive_ctx.serial_number));
	memset(usbdvd_drive_ctx.disc_type,0,sizeof(usbdvd_drive_ctx.disc_type));
			
	memset(usbdvd_drive_ctx.fs.mountpoint,0,sizeof(usbdvd_drive_ctx.fs.mountpoint));
	memset(usbdvd_drive_ctx.fs.disc_fstype,0,sizeof(usbdvd_drive_ctx.fs.disc_fstype));
	memset(usbdvd_drive_ctx.fs.volid,0,sizeof(usbdvd_drive_ctx.fs.volid));
	

	strcpy(usbdvd_drive_ctx.vendor_id,"USBDVD");
	strncpy(usbdvd_drive_ctx.product_id,LIBUSBDVD_VERSION_STRING,sizeof(usbdvd_drive_ctx.product_id)-1);
	strcpy(usbdvd_drive_ctx.product_revision,"1");
	strcpy(usbdvd_drive_ctx.serial_number,"001");
	strcpy(usbdvd_drive_ctx.disc_type,"DISC IMAGE");
	
	ISO9660FS = new CUSBDVD_ISO9660FS(isofilepath);
	strncpy(usbdvd_drive_ctx.fs.volid,ISO9660FS->VolumeIdentifier.c_str(),sizeof(usbdvd_drive_ctx.fs.volid)-1);
			
	
	
	
	SWITCH_ISO9660DEVOPTAB = new SWITCH_ISO9660FS(ISO9660FS,"iso0","iso0:");
	strcpy(usbdvd_drive_ctx.fs.mountpoint,"iso0:");
	if(ISO9660FS->isjoliet){
		strcpy(usbdvd_drive_ctx.fs.disc_fstype,"ISO9660 + Joliet");	
	} else if(ISO9660FS->isrockridge){
		strcpy(usbdvd_drive_ctx.fs.disc_fstype,"ISO9660 + RockRidge");
	}else{
		strcpy(usbdvd_drive_ctx.fs.disc_fstype,"ISO9660");
	}
	usbdvd_drive_ctx.fs.mounted = true;
	
	
}

CUSBDVD::CUSBDVD(){
	usbdvd_drive_ctx.drive_found = false;
	usbdvd_drive_ctx.fileimage = false;
	usbdvd_drive_ctx.fs.mounted = false;
	SWITCH_USB = new CSWITCH_USB();
	if(SWITCH_USB->device_found){
		USB_SCSI = new CUSBSCSI(SWITCH_USB);
	
	
		if(USB_SCSI!=nullptr){
	
	
		
			
			ScsiInquiryStandardData test = {0};
			memset(&test,0,sizeof(ScsiInquiryStandardData));
			int ret = USB_SCSI->UsbDvdSendInquiry(0,sizeof(ScsiInquiryStandardData),(uint8_t *)&test);
			if(ret!=0)return;
			
			memset(usbdvd_drive_ctx.vendor_id,0,sizeof(usbdvd_drive_ctx.vendor_id));
			memset(usbdvd_drive_ctx.product_id,0,sizeof(usbdvd_drive_ctx.product_id));
			memset(usbdvd_drive_ctx.product_revision,0,sizeof(usbdvd_drive_ctx.product_revision));
			memset(usbdvd_drive_ctx.serial_number,0,sizeof(usbdvd_drive_ctx.serial_number));
			memset(usbdvd_drive_ctx.disc_type,0,sizeof(usbdvd_drive_ctx.disc_type));
			
			memset(usbdvd_drive_ctx.fs.mountpoint,0,sizeof(usbdvd_drive_ctx.fs.mountpoint));
			memset(usbdvd_drive_ctx.fs.disc_fstype,0,sizeof(usbdvd_drive_ctx.fs.disc_fstype));
			memset(usbdvd_drive_ctx.fs.volid,0,sizeof(usbdvd_drive_ctx.fs.volid));
			
			
			memcpy(usbdvd_drive_ctx.vendor_id,test.vendor_id,sizeof(test.vendor_id));
			memcpy(usbdvd_drive_ctx.product_id,test.product_id,sizeof(test.product_id));
			memcpy(usbdvd_drive_ctx.product_revision,test.product_revision,sizeof(test.product_revision));
			memcpy(usbdvd_drive_ctx.serial_number,test.serial_number,sizeof(test.serial_number));
			
			
			
			
			ret = USB_SCSI->UsbDvdUnitReady(0);
			drive_status = ret;
			usbdvd_drive_ctx.drive_found = true;
			if(drive_status!=0)return;
			
			usb_init = true;
			
			
			bool isbluray = false;
			bool isdvd = false;
			bool iscdrom = false;
			
			
			std::string disctype  = "Unknown";
			uint8_t testconf[0x08];
			ret = USB_SCSI->UsbDvdGetConfig(0,testconf);
			if(ret == 0){
				switch (testconf[7]) {
					case ATAPI_PROFILE_CD_ROM:
						disctype = "CD-ROM";
						iscdrom = true;
						break;
					case ATAPI_PROFILE_CD_R:
						disctype = "CD-R";
						iscdrom = true;
						break;
					case ATAPI_PROFILE_CD_RW:
						disctype = "CD-RW";
						iscdrom = true;
						break;
					case ATAPI_PROFILE_DVD_ROM:
						disctype = "DVD-ROM";
						isdvd = true;
						break;
					case ATAPI_PROFILE_DVD_R_SEQUENTIAL:
						disctype = "DVD-R";
						isdvd = true;
						break;
					case ATAPI_PROFILE_DVD_RAM:
						disctype = "DVD-RAM";
						isdvd = true;
						break;
					case ATAPI_PROFILE_DVD_RW_RESTRICTED_OVERWRITE:
						disctype = "DVD-RW";
						isdvd = true;
						break;
					case ATAPI_PROFILE_DVD_RW_SEQUENTIAL:
						disctype = "DVD-RW";
						isdvd = true;
						break;
					case ATAPI_PROFILE_DVD_R_DL_SEQUENTIAL:
						disctype = "DVD-R DL";
						isdvd = true;
						break;
					case ATAPI_PROFILE_DVD_R_DL_JUMP_RECORDING:
						disctype = "DVD-R DL";
						isdvd = true;
						break;
					case ATAPI_PROFILE_DVD_RW_DL:
						disctype = "DVD-RW DL";
						isdvd = true;
						break;
					case ATAPI_PROFILE_DVD_DOWNLOAD_DISC:
						disctype = "DVD-DL";
						isdvd = true;
						break;
					case ATAPI_PROFILE_BD_ROM:
						disctype = "BD-ROM";
						break;
					case ATAPI_PROFILE_BD_R_SEQUENTIAL:
					case ATAPI_PROFILE_BD_R_RANDOM:
						disctype = "BD-R";
						break;
					case ATAPI_PROFILE_BD_R_DL_SEQUENTIAL:
					case ATAPI_PROFILE_BD_R_DL_JUMP_RECORDING:
					case ATAPI_PROFILE_BD_RE_DL:
						disctype = "BD-R DL";
						isbluray = true;
						break;
					
					default:
						disctype = "Unknown";
				}
			
			}
			
			disccapacity_struct testsize;
			USB_SCSI->UsbDvdGetCapacity(0,(uint8_t *)&testsize);
			
			strncpy(usbdvd_drive_ctx.disc_type,disctype.c_str(),sizeof(usbdvd_drive_ctx.disc_type)-1);
			
			USB_SCSI->UsbDvdPreventMediumRemoval(0,1);
			
			if(iscdrom){
				uint8_t *buf;
				int bufsize;
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
						strncpy(usbdvd_drive_ctx.fs.mountpoint,"acd0:",sizeof(usbdvd_drive_ctx.fs.mountpoint)-1);
						strncpy(usbdvd_drive_ctx.fs.disc_fstype,"CD Audio",sizeof(usbdvd_drive_ctx.fs.disc_fstype)-1);
						strncpy(usbdvd_drive_ctx.fs.volid,"CD Audio",sizeof(usbdvd_drive_ctx.fs.volid)-1);
						usbdvd_drive_ctx.fs.mounted = true;
					}
			
				} else if(myfstype == 2){
					uint32_t mylba = ((toc.tracks[0].MIN*60)+toc.tracks[0].SEC)*75+toc.tracks[0].FRAME;
					uint32_t nextlba = ((toc.tracks[1].MIN*60)+toc.tracks[1].SEC)*75+toc.tracks[1].FRAME;
					
					uint8_t fssector[2048];
					USB_SCSI->UsbDvdReadCD_Data(0,16,1,fssector);
					if(fssector[0] == 0 && fssector[1] == 'B' && fssector[2] == 'E' && fssector[3] == 'A' && fssector[4] == '0' && fssector[5] == '1'){
						USBDVD_UDFFS = new CUSBDVD_UDFFS(USB_SCSI,mylba,nextlba);
						cdfs_init = true;
						SWITCH_UDFDEVOPTAB = new SWITCH_UDFFS(USBDVD_UDFFS,"iso0","iso0:");
						pseudofs_init = true;
						strcpy(usbdvd_drive_ctx.fs.mountpoint,"iso0:");
						strcpy(usbdvd_drive_ctx.fs.disc_fstype,USBDVD_UDFFS->udf_version_string.c_str());
						
						if(pseudofs_init && cdfs_init && usb_init)usbdvd_drive_ctx.fs.mounted = true;
					}
					
					if(fssector[0] == 1 && fssector[1] == 'C' && fssector[2] == 'D' && fssector[3] == '0' && fssector[4] == '0' && fssector[5] == '1'){
					
						ISO9660FS = new CUSBDVD_ISO9660FS(USB_SCSI,mylba,nextlba);
						strncpy(usbdvd_drive_ctx.fs.volid,ISO9660FS->VolumeIdentifier.c_str(),sizeof(usbdvd_drive_ctx.fs.volid)-1);
					
						cdfs_init = true;
						usbdvd_drive_ctx.fs.jolietver = ISO9660FS->jolietver;
						SWITCH_ISO9660DEVOPTAB = new SWITCH_ISO9660FS(ISO9660FS,"iso0","iso0:");
						pseudofs_init = true;
						strcpy(usbdvd_drive_ctx.fs.mountpoint,"iso0:");
						
						if(pseudofs_init && cdfs_init && usb_init)usbdvd_drive_ctx.fs.mounted = true;
						if(ISO9660FS->isjoliet){
							strcpy(usbdvd_drive_ctx.fs.disc_fstype,"ISO9660 + Joliet");	
					
						} else if(ISO9660FS->isrockridge){
							strcpy(usbdvd_drive_ctx.fs.disc_fstype,"ISO9660 + RockRidge");
					
						}else{
							strcpy(usbdvd_drive_ctx.fs.disc_fstype,"ISO9660");
					
						}
					}
				}
			
			}
			
			if(isdvd || isbluray){
			
				uint32_t mylba = 0;
				//uint32_t nextlba = ((toc.tracks[1].MIN*60)+toc.tracks[1].SEC)*75+toc.tracks[1].FRAME;
				uint32_t nextlba = byte2u32_be(testsize.size);
				
				uint8_t fssector[2048];
				USB_SCSI->UsbDvdReadCD_Data(0,16,1,fssector);
				
				if(fssector[0] == 0 && fssector[1] == 'B' && fssector[2] == 'E' && fssector[3] == 'A' && fssector[4] == '0' && fssector[5] == '1'){
					USBDVD_UDFFS = new CUSBDVD_UDFFS(USB_SCSI,mylba,nextlba);
					cdfs_init = true;
					SWITCH_UDFDEVOPTAB = new SWITCH_UDFFS(USBDVD_UDFFS,"iso0","iso0:");
					pseudofs_init = true;
					strcpy(usbdvd_drive_ctx.fs.mountpoint,"iso0:");
					strcpy(usbdvd_drive_ctx.fs.disc_fstype,USBDVD_UDFFS->udf_version_string.c_str());
					
					if(pseudofs_init && cdfs_init && usb_init)usbdvd_drive_ctx.fs.mounted = true;
				}
				
				if(fssector[0] == 1 && fssector[1] == 'C' && fssector[2] == 'D' && fssector[3] == '0' && fssector[4] == '0' && fssector[5] == '1'){
				
					ISO9660FS = new CUSBDVD_ISO9660FS(USB_SCSI,mylba,nextlba);
					strncpy(usbdvd_drive_ctx.fs.volid,ISO9660FS->VolumeIdentifier.c_str(),sizeof(usbdvd_drive_ctx.fs.volid)-1);
				
					cdfs_init = true;
					usbdvd_drive_ctx.fs.jolietver = ISO9660FS->jolietver;
					SWITCH_ISO9660DEVOPTAB = new SWITCH_ISO9660FS(ISO9660FS,"iso0","iso0:");
					pseudofs_init = true;
					strcpy(usbdvd_drive_ctx.fs.mountpoint,"iso0:");
					
					if(pseudofs_init && cdfs_init && usb_init)usbdvd_drive_ctx.fs.mounted = true;
					if(ISO9660FS->isjoliet){
						strcpy(usbdvd_drive_ctx.fs.disc_fstype,"ISO9660 + Joliet");	
				
					} else if(ISO9660FS->isrockridge){
						strcpy(usbdvd_drive_ctx.fs.disc_fstype,"ISO9660 + RockRidge");
				
					}else{
						strcpy(usbdvd_drive_ctx.fs.disc_fstype,"ISO9660");
				
					}
				}
			}
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


std::string CUSBDVD::get_version(){
	return LIBUSBDVD_VERSION_STRING;
}


void CUSBDVD::Eject(){
	
	if(USB_SCSI != nullptr){
		usbdvd_drive_ctx.fs.mounted = false;
		if(SWITCH_CDAUDIODEVOPTAB != nullptr){
			delete SWITCH_CDAUDIODEVOPTAB;
			SWITCH_CDAUDIODEVOPTAB = nullptr;
		}
		if(SWITCH_ISO9660DEVOPTAB != nullptr){
			delete SWITCH_ISO9660DEVOPTAB;
			SWITCH_ISO9660DEVOPTAB = nullptr;
			
		}
		if(SWITCH_UDFDEVOPTAB != nullptr){
			delete SWITCH_UDFDEVOPTAB;
			SWITCH_UDFDEVOPTAB = nullptr;
		}
		
		
		if(CDAUDIOFS != nullptr){
			delete CDAUDIOFS;
			CDAUDIOFS = nullptr;
		}
		if(ISO9660FS != nullptr){
			delete ISO9660FS;
			ISO9660FS = nullptr;
		}
		if(USBDVD_UDFFS != nullptr){
			delete USBDVD_UDFFS;
			USBDVD_UDFFS = nullptr;
		}
		USB_SCSI->UsbDvdPreventMediumRemoval(0,0);
		USB_SCSI->UsbDvd_Eject(0);
	}
	
}


#ifdef __cplusplus
extern "C" {
#endif

static CUSBDVD* cast_to_cpp(usbdvd_obj* obj) {
    return reinterpret_cast<CUSBDVD*>(obj);
}

static usbdvd_obj* cast_to_c(CUSBDVD* obj) {
    return reinterpret_cast<usbdvd_obj*>(obj);
}

usbdvd_obj* usbdvd_initimage(const char * _path){
	CUSBDVD* obj = new CUSBDVD(_path);
	return cast_to_c(obj);
}

usbdvd_obj* usbdvd_init() {
	CUSBDVD* obj = new CUSBDVD();
	return cast_to_c(obj);
}

void usbdvd_destroy(usbdvd_obj* obj) {
    if (obj) {
        delete cast_to_cpp(obj);
    }
}


void usbdvd_eject(usbdvd_obj* obj) {
    if (!obj)return;
	cast_to_cpp(obj)->Eject();
}

usbdvd_drive_struct * usbdvd_get_drivectx(usbdvd_obj* obj){
	if (!obj)return NULL;
	return &cast_to_cpp(obj)->usbdvd_drive_ctx; 
}

const char* usbdvd_version(void){
	static char version[32];
    snprintf(version, sizeof(version), "%s",LIBUSBDVD_VERSION_STRING);
    return version;
}
#ifdef __cplusplus
}
#endif

