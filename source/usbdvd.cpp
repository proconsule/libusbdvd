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
#include "usbdvd_csscache.h"

#define VD_VENDOR_ID "USBDVD"
#define VD_PRODUCT_ID LIBUSBDVD_VERSION_STRING
#define VD_PRODUCT_REV "1"
#define VD_SERIAL_NUM "001"

#include <chrono>

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
    usbdvd_ctx.drive.drive_found = false;
    usbdvd_ctx.drive.fileimage = true;
    
    memset(&usbdvd_ctx,0,sizeof(usbdvd_ctx));



    strcpy(usbdvd_ctx.drive.vendor_id,VD_VENDOR_ID);
    strncpy(usbdvd_ctx.drive.product_id,VD_PRODUCT_ID,sizeof(usbdvd_ctx.drive.product_id)-1);
    strcpy(usbdvd_ctx.drive.product_revision,VD_PRODUCT_REV);
    strcpy(usbdvd_ctx.drive.serial_number,VD_SERIAL_NUM);
    strcpy(usbdvd_ctx.drive.disc_type,"DISC IMAGE");
    CDDVD_TOC toc;
    if (!cuebin_to_TOC(_cuepath,_binpath,&toc)) {
        return;
    }

    CDAUDIOFS = new CAUDIOCD_PSEUDOFS(toc,_binpath);
    if(CDAUDIOFS->CDAudioFound()){
        cdfs_init = true;
        SWITCH_CDAUDIODEVOPTAB = new SWITCH_AUDIOCDFS(CDAUDIOFS,"acd0","acd0:");
        pseudofs_init = true;
        strncpy(usbdvd_ctx.fs.mountpoint,"acd0:",sizeof(usbdvd_ctx.fs.mountpoint)-1);
        strncpy(usbdvd_ctx.fs.disc_fstype,"CD Audio",sizeof(usbdvd_ctx.fs.disc_fstype)-1);
        strncpy(usbdvd_ctx.fs.volid,"CD Audio",sizeof(usbdvd_ctx.fs.volid)-1);
        usbdvd_ctx.fs.mounted = true;
    }


}

CUSBDVD::CUSBDVD(std::string _isofilepath){
    isofilepath = _isofilepath;
    usbdvd_ctx.drive.drive_found = false;
    usbdvd_ctx.drive.fileimage = true;

    memset(usbdvd_ctx.drive.vendor_id,0,sizeof(usbdvd_ctx.drive.vendor_id));
    memset(usbdvd_ctx.drive.product_id,0,sizeof(usbdvd_ctx.drive.product_id));
    memset(usbdvd_ctx.drive.product_revision,0,sizeof(usbdvd_ctx.drive.product_revision));
    memset(usbdvd_ctx.drive.serial_number,0,sizeof(usbdvd_ctx.drive.serial_number));
    memset(usbdvd_ctx.drive.disc_type,0,sizeof(usbdvd_ctx.drive.disc_type));

    memset(usbdvd_ctx.fs.mountpoint,0,sizeof(usbdvd_ctx.fs.mountpoint));
    memset(usbdvd_ctx.fs.disc_fstype,0,sizeof(usbdvd_ctx.fs.disc_fstype));
    memset(usbdvd_ctx.fs.volid,0,sizeof(usbdvd_ctx.fs.volid));


    strcpy(usbdvd_ctx.drive.vendor_id,VD_VENDOR_ID);
    strncpy(usbdvd_ctx.drive.product_id,VD_PRODUCT_ID,sizeof(usbdvd_ctx.drive.product_id)-1);
    strcpy(usbdvd_ctx.drive.product_revision,VD_PRODUCT_REV);
    strcpy(usbdvd_ctx.drive.serial_number,VD_SERIAL_NUM);
    strcpy(usbdvd_ctx.drive.disc_type,"DISC IMAGE");


    uint8_t fssector[2048];

    FILE * myiso = fopen(isofilepath.c_str(),"rb");
    if(myiso == NULL)return;
    fseek(myiso,2048*16,SEEK_SET);
    fread(fssector, sizeof(char), sizeof(fssector), myiso);
    fclose(myiso);

    //USB_SCSI->UsbDvdReadCD_Data(0,16,1,fssector);

    if(fssector[0] == 0 && fssector[1] == 'B' && fssector[2] == 'E' && fssector[3] == 'A' && fssector[4] == '0' && fssector[5] == '1'){
        USBDVD_UDFFS = new CUSBDVD_UDFFS(isofilepath);
        strncpy(usbdvd_ctx.fs.volid,USBDVD_UDFFS->disc_lvd.VolumeIdentifier.c_str(),sizeof(usbdvd_ctx.fs.volid)-1);
        cdfs_init = true;
        SWITCH_UDFDEVOPTAB = new SWITCH_UDFFS(USBDVD_UDFFS,"iso0","iso0:");
        pseudofs_init = true;
        strcpy(usbdvd_ctx.fs.mountpoint,"iso0:");
        strcpy(usbdvd_ctx.fs.disc_fstype,USBDVD_UDFFS->disc_lvd.udf_version_string.c_str());
        if(pseudofs_init && cdfs_init && usb_init)usbdvd_ctx.fs.mounted = true;
    }

    if(fssector[0] == 1 && fssector[1] == 'C' && fssector[2] == 'D' && fssector[3] == '0' && fssector[4] == '0' && fssector[5] == '1'){

        ISO9660FS = new CUSBDVD_ISO9660FS(isofilepath);
        strncpy(usbdvd_ctx.fs.volid,ISO9660FS->VolumeIdentifier.c_str(),sizeof(usbdvd_ctx.fs.volid)-1);

        cdfs_init = true;
        usbdvd_ctx.fs.joliet_extended.version = ISO9660FS->jolietver;
        SWITCH_ISO9660DEVOPTAB = new SWITCH_ISO9660FS(ISO9660FS,"iso0","iso0:");
        pseudofs_init = true;
        strcpy(usbdvd_ctx.fs.mountpoint,"iso0:");

        if(pseudofs_init && cdfs_init && usb_init)usbdvd_ctx.fs.mounted = true;
        if(ISO9660FS->isjoliet){
            strcpy(usbdvd_ctx.fs.disc_fstype,"ISO9660 + Joliet");

            } else if(ISO9660FS->isrockridge){
                strcpy(usbdvd_ctx.fs.disc_fstype,"ISO9660 + RockRidge");

            }else{
                strcpy(usbdvd_ctx.fs.disc_fstype,"ISO9660");

            }
    }

}

CUSBDVD::CUSBDVD(){
    usbdvd_ctx.drive.drive_found = false;
    usbdvd_ctx.drive.fileimage = false;
    usbdvd_ctx.fs.mounted = false;
    SWITCH_USB = new CSWITCH_USB();
    if(SWITCH_USB->device_found){
        USB_SCSI = new CUSBSCSI(SWITCH_USB);


        if(USB_SCSI!=nullptr){
            ScsiInquiryStandardData test = {0};
            memset(&test,0,sizeof(ScsiInquiryStandardData));
            int ret = USB_SCSI->UsbDvdSendInquiry(0,sizeof(ScsiInquiryStandardData),(uint8_t *)&test);
            if(ret!=0)return;
            
            memset(&usbdvd_ctx,0,sizeof(usbdvd_ctx));

            memcpy(usbdvd_ctx.drive.vendor_id,test.vendor_id,sizeof(test.vendor_id));
            memcpy(usbdvd_ctx.drive.product_id,test.product_id,sizeof(test.product_id));
            memcpy(usbdvd_ctx.drive.product_revision,test.product_revision,sizeof(test.product_revision));
            memcpy(usbdvd_ctx.drive.serial_number,test.serial_number,sizeof(test.serial_number));

            usbdvd_ctx.dvd_protection.CSS = false;
            
            usbdvd_ctx.drive.drive_found = true;
            ret = USB_SCSI->UsbDvdUnitReady(0);
            drive_status = ret;
            std::string disctype  = "Unknown";
            usb_init = true;
            if(drive_status!=0)return;

            //MountDisc();


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

        cdfs_init = false;
        pseudofs_init = false;
        usbdvd_ctx.fs.mounted = false;
        memset(&usbdvd_ctx.fs,0,sizeof(usbdvd_ctx.fs));
        memset(&usbdvd_ctx.dvd_protection,0,sizeof(usbdvd_ctx.dvd_protection));
        memset(usbdvd_ctx.drive.disc_type,0,sizeof(usbdvd_ctx.drive.disc_type));
        memset(&usbdvd_ctx.bluray_protection,0,sizeof(usbdvd_ctx.bluray_protection));

    }

}


int CUSBDVD::MountDisc(){

    if(usbdvd_ctx.fs.mounted)return -1;

    int ret = USB_SCSI->UsbDvdUnitReady(0);
    drive_status = ret;
    std::string disctype  = "Unknown";
    if(drive_status!=0)return -1;

    bool isbluray = false;
    bool isdvd = false;
    bool iscdrom = false;

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
                        isbluray = true;
                        break;
                    case ATAPI_PROFILE_BD_R_SEQUENTIAL:
                    case ATAPI_PROFILE_BD_R_RANDOM:
                        disctype = "BD-R";
                        isbluray = true;
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
            usbdvd_ctx.drive.disc_size = byte2u32_be(testsize.size)*byte2u32_be(testsize.blocksize);
            strncpy(usbdvd_ctx.drive.disc_type,disctype.c_str(),sizeof(usbdvd_ctx.drive.disc_type)-1);
            USB_SCSI->UsbDvdPreventMediumRemoval(0,1);

            if(iscdrom){
                uint8_t *buf;
                int bufsize;
                ret = USB_SCSI->UsbDvdSendTOC(0,(void **)&buf,&bufsize);
                if(ret != 0){
                    if(buf)free(buf);
                    return -1;
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
                        strncpy(usbdvd_ctx.fs.mountpoint,"acd0:",sizeof(usbdvd_ctx.fs.mountpoint)-1);
                        strncpy(usbdvd_ctx.fs.disc_fstype,"CD Audio",sizeof(usbdvd_ctx.fs.disc_fstype)-1);
                        strncpy(usbdvd_ctx.fs.volid,"CD Audio",sizeof(usbdvd_ctx.fs.volid)-1);
                        usbdvd_ctx.fs.disc_fsidx = USBDVD_FS_CDAUDIOFS;
                        usbdvd_ctx.fs.mounted = true;
                        usbdvd_ctx.fs.block_size = 2352;
                    }

                } else if(myfstype == 2){
                    uint32_t mylba = ((toc.tracks[0].MIN*60)+toc.tracks[0].SEC)*75+toc.tracks[0].FRAME;
                    uint32_t nextlba = ((toc.tracks[1].MIN*60)+toc.tracks[1].SEC)*75+toc.tracks[1].FRAME;

                    uint8_t fssector[2048];
                    USB_SCSI->UsbDvdReadCD_Data(0,16,1,fssector);
                    if(fssector[0] == 0 && fssector[1] == 'B' && fssector[2] == 'E' && fssector[3] == 'A' && fssector[4] == '0' && fssector[5] == '1'){
                        USBDVD_UDFFS = new CUSBDVD_UDFFS(USB_SCSI,mylba,nextlba);
                        strncpy(usbdvd_ctx.fs.volid,USBDVD_UDFFS->disc_lvd.VolumeIdentifier.c_str(),sizeof(usbdvd_ctx.fs.volid)-1);

                        cdfs_init = true;
                        SWITCH_UDFDEVOPTAB = new SWITCH_UDFFS(USBDVD_UDFFS,"udf0","udf0:");
                        usbdvd_ctx.fs.block_size = 2048;
                        pseudofs_init = true;
                        strcpy(usbdvd_ctx.fs.mountpoint,"udf0:");
                        strcpy(usbdvd_ctx.fs.disc_fstype,USBDVD_UDFFS->disc_lvd.udf_version_string.c_str());
                        usbdvd_ctx.fs.disc_fsidx = USBDVD_FS_UTF;
                        usbdvd_ctx.fs.udf_extended.udfver = USBDVD_UDFFS->disc_lvd.udfver;
                        usbdvd_ctx.fs.udf_extended.metadata_partition_size = USBDVD_UDFFS->disc_lvd.metadata_partition_size;
                        usbdvd_ctx.fs.udf_extended.number_of_partition_maps = USBDVD_UDFFS->disc_lvd.number_of_partition_maps;
                        
                        
                        if(pseudofs_init && cdfs_init && usb_init)usbdvd_ctx.fs.mounted = true;
                        return 0;
                    }

                    if(fssector[0] == 1 && fssector[1] == 'C' && fssector[2] == 'D' && fssector[3] == '0' && fssector[4] == '0' && fssector[5] == '1'){

                        ISO9660FS = new CUSBDVD_ISO9660FS(USB_SCSI,mylba,nextlba);
                        strncpy(usbdvd_ctx.fs.volid,ISO9660FS->VolumeIdentifier.c_str(),sizeof(usbdvd_ctx.fs.volid)-1);
                        usbdvd_ctx.fs.block_size = 2048;
                        cdfs_init = true;
                        usbdvd_ctx.fs.joliet_extended.version = ISO9660FS->jolietver;
                        SWITCH_ISO9660DEVOPTAB = new SWITCH_ISO9660FS(ISO9660FS,"cdr0","cdr0:");
                        pseudofs_init = true;
                        strcpy(usbdvd_ctx.fs.mountpoint,"cdr0:");

                        if(pseudofs_init && cdfs_init && usb_init)usbdvd_ctx.fs.mounted = true;
                        if(ISO9660FS->isjoliet){
                            
                            strcpy(usbdvd_ctx.fs.disc_fstype,"ISO9660 + Joliet");
                            usbdvd_ctx.fs.disc_fsidx = USBDVD_FS_ISO9660_JOLIET;

                        } else if(ISO9660FS->isrockridge){
                            strcpy(usbdvd_ctx.fs.disc_fstype,"ISO9660 + RockRidge");
                            usbdvd_ctx.fs.disc_fsidx = USBDVD_FS_ISO9660_RR;

                        }else{
                            strcpy(usbdvd_ctx.fs.disc_fstype,"ISO9660");
                            usbdvd_ctx.fs.disc_fsidx = USBDVD_FS_ISO9660;

                        }
                        return 0;
                    }
                }

            }

            if(isdvd || isbluray){
                
                uint32_t mylba = 0;
                uint32_t nextlba = byte2u32_be(testsize.size);

                uint8_t fssector[2048];
                USB_SCSI->UsbDvdReadCD_Data(0,16,1,fssector);

                if(fssector[0] == 0 && fssector[1] == 'B' && fssector[2] == 'E' && fssector[3] == 'A' && fssector[4] == '0' && fssector[5] == '1'){
                    USBDVD_UDFFS = new CUSBDVD_UDFFS(USB_SCSI,mylba,nextlba);
                    strncpy(usbdvd_ctx.fs.volid,USBDVD_UDFFS->disc_lvd.VolumeIdentifier.c_str(),sizeof(usbdvd_ctx.fs.volid)-1);

                    cdfs_init = true;
                    SWITCH_UDFDEVOPTAB = new SWITCH_UDFFS(USBDVD_UDFFS,"udf0","udf0:");
                    pseudofs_init = true;
                    strcpy(usbdvd_ctx.fs.mountpoint,"udf0:");
                    strcpy(usbdvd_ctx.fs.disc_fstype,USBDVD_UDFFS->disc_lvd.udf_version_string.c_str());
                    usbdvd_ctx.fs.block_size = 2048;
                    usbdvd_ctx.fs.disc_fsidx = USBDVD_FS_UTF;
                    usbdvd_ctx.fs.udf_extended.udfver = USBDVD_UDFFS->disc_lvd.udfver;
                    usbdvd_ctx.fs.udf_extended.metadata_partition_size = USBDVD_UDFFS->disc_lvd.metadata_partition_size;
                    usbdvd_ctx.fs.udf_extended.number_of_partition_maps = USBDVD_UDFFS->disc_lvd.number_of_partition_maps;
                        

                    if(pseudofs_init && cdfs_init && usb_init)usbdvd_ctx.fs.mounted = true;
                    //return 0;
                }

                if(fssector[0] == 1 && fssector[1] == 'C' && fssector[2] == 'D' && fssector[3] == '0' && fssector[4] == '0' && fssector[5] == '1'){

                    std::string devmountstring = "";
                    if(isdvd){
                        devmountstring = "dvd0";
                    }
                    if(isbluray){
                        devmountstring = "bdr0";
                    }
                    std::string devmountstring2 = devmountstring+":";
                    ISO9660FS = new CUSBDVD_ISO9660FS(USB_SCSI,mylba,nextlba);
                    usbdvd_ctx.fs.block_size = 2048;
                    strncpy(usbdvd_ctx.fs.volid,ISO9660FS->VolumeIdentifier.c_str(),sizeof(usbdvd_ctx.fs.volid)-1);

                    cdfs_init = true;
                    usbdvd_ctx.fs.joliet_extended.version = ISO9660FS->jolietver;
                    SWITCH_ISO9660DEVOPTAB = new SWITCH_ISO9660FS(ISO9660FS,devmountstring,devmountstring2);
                    pseudofs_init = true;
                    strcpy(usbdvd_ctx.fs.mountpoint,devmountstring2.c_str());

                    if(pseudofs_init && cdfs_init && usb_init)usbdvd_ctx.fs.mounted = true;
                    if(ISO9660FS->isjoliet){
                            
                        strcpy(usbdvd_ctx.fs.disc_fstype,"ISO9660 + Joliet");
                        usbdvd_ctx.fs.disc_fsidx = USBDVD_FS_ISO9660_JOLIET;

                    } else if(ISO9660FS->isrockridge){
                        strcpy(usbdvd_ctx.fs.disc_fstype,"ISO9660 + RockRidge");
                        usbdvd_ctx.fs.disc_fsidx = USBDVD_FS_ISO9660_RR;

                    }else{
                        strcpy(usbdvd_ctx.fs.disc_fstype,"ISO9660");
                        usbdvd_ctx.fs.disc_fsidx = USBDVD_FS_ISO9660;

                    }
                    //return 0;
                }
                if(isdvd){
                    
                    dvd_copyright_info_t copyright;
                    USB_SCSI->UsbDvdReadDVDStructure(0,0x01,sizeof(copyright),&copyright);
                    usbdvd_ctx.dvd_protection.CSS = copyright.CSS == 0x01 ? true : false;
                    usbdvd_ctx.dvd_protection.regions = copyright.region_info;
                    
                    if(usbdvd_ctx.dvd_protection.CSS){
                        
                        if(ISO9660FS!=nullptr){
                            ISO9660FS->DVD_CSS = true;
                            
                            CUSBDVD_CSSCache testcache;
                            usbdvd_ctx.dvd_protection.cache_keys = testcache.countEntrys();
                            testcache.printCache();
                            std::string key = testcache.getKey(ISO9660FS->disc_hash);
                            if(key!= ""){
                                usbdvd_log("Key Found!");
                                std::vector<key_storage_struct> cache_list = testcache.hexStringToKeys(key);
                                for(unsigned int i=0;i<cache_list.size();i++){
                                    css_titlekey_struct tmpentry;
                                    tmpentry.titlenum = i;
                                    memcpy(tmpentry.key,cache_list[i].key,5);
                                    ISO9660FS->titlekeys.push_back(tmpentry);
                                }
                                
                            }else{
                            
                            
                                auto start_time = std::chrono::high_resolution_clock::now();
                                ISO9660FS->DVDGetAllCSSKeys();
                                auto end_time = std::chrono::high_resolution_clock::now();
                                auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
                                usbdvd_ctx.dvd_protection.decss_msecs = duration.count();
                                std::vector<key_storage_struct> tmpstorage;
                                for(unsigned int i=0;i<ISO9660FS->titlekeys.size();i++){
                                    key_storage_struct tmpentry;
                                    memcpy(tmpentry.key,ISO9660FS->titlekeys[i].key,5);
                                    tmpstorage.push_back(tmpentry);
                                    
                                }
                                std::string keyconcat = testcache.keyToHexString(tmpstorage);
                                testcache.addKey(ISO9660FS->disc_hash,keyconcat);
                            
                            }
                        }
                        
                    }
                    
                    
                    
                }
                if(isbluray){
                    if(USBDVD_UDFFS!=nullptr){
                        USBDVD_UDFFS->ACSS = USBDVD_UDFFS->isAACSProtected(usbdvd_ctx.fs.mountpoint + std::string("/"));
                        usbdvd_ctx.bluray_protection.AACS = USBDVD_UDFFS->ACSS;
                    }
                    
                    
                }
                return 0;
            }
        return -1;
}


void CUSBDVD::Cache_IFO_Files(){
	if(ISO9660FS != nullptr){
		ISO9660FS->Cache_IFO_Files();
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

usbdvd_obj* usbdvd_initcuebin(const char * _cuepath,const char * _binpath){
    CUSBDVD* obj = new CUSBDVD(_cuepath,_binpath);
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

int usbdvd_mountdisc(usbdvd_obj* obj) {
    if (!obj)return -1;
    return cast_to_cpp(obj)->MountDisc();
}

void usbdvd_eject(usbdvd_obj* obj) {
    if (!obj)return;
    cast_to_cpp(obj)->Eject();
}

usbdvd_struct * usbdvd_get_ctx(usbdvd_obj* obj){
    if (!obj)return NULL;
    return &cast_to_cpp(obj)->usbdvd_ctx;
}

const char* usbdvd_version(void){
    static char version[32];
    snprintf(version, sizeof(version), "%s",LIBUSBDVD_VERSION_STRING);
    return version;
}

void usbdvd_cache_ifo_files(usbdvd_obj* obj){
	if (!obj)return;
    cast_to_cpp(obj)->Cache_IFO_Files();
}

#ifdef __cplusplus
}
#endif
