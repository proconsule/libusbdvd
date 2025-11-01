#include "udf_devoptab.h"
#include <filesystem>

void udffsstat_entry(disc_dirlist_struct *_filedesc, struct stat *st);

#define SCSI_IOCTL_DATA_OUT             0
#define SCSI_IOCTL_DATA_IN              1
#define SCSI_IOCTL_DATA_UNSPECIFIED     2
#define IOCTL_SCSI_PASS_THROUGH_DIRECT  0x4D014
#define MAX_SENSE_LEN                   18



SWITCH_UDFFS::SWITCH_UDFFS(CUSBDVD_UDFFS *_ctx,std::string _name,std::string _mount_name){

    this->UDFFS = _ctx;
    
    
    this->name       = _name;
    this->mount_name = _mount_name;
    
    this->devoptab = {
        .name         = SWITCH_UDFFS::name.data(),

        .structSize   = sizeof(SWITCH_UDFFSFile),
        .open_r       = SWITCH_UDFFS::udffs_open,
        .close_r      = SWITCH_UDFFS::udffs_close,
        .write_r      = SWITCH_UDFFS::udffs_write,
        .read_r       = SWITCH_UDFFS::udffs_read,
       
        .seek_r       = SWITCH_UDFFS::udffs_seek,
        .fstat_r      = SWITCH_UDFFS::udffs_fstat,

        .stat_r       = SWITCH_UDFFS::udffs_stat,
        .chdir_r      = SWITCH_UDFFS::udffs_chdir,

        .dirStateSize = sizeof(SWITCH_UDFFSDir),
        .diropen_r    = SWITCH_UDFFS::udffs_diropen,
        .dirreset_r   = SWITCH_UDFFS::udffs_dirreset,
        .dirnext_r    = SWITCH_UDFFS::udffs_dirnext,
        .dirclose_r   = SWITCH_UDFFS::udffs_dirclose,

        .statvfs_r    = SWITCH_UDFFS::udffs_statvfs,

        .deviceData   = this,

        .lstat_r      = SWITCH_UDFFS::udffs_stat,
    };
    
    if(connect() == 0){
        register_fs();
    }

}

int SWITCH_UDFFS::connect(){
    
    
    return 0;
}


SWITCH_UDFFS::~SWITCH_UDFFS(){
    
    unregister_fs();
    if(file_ioctl_buffer){
        free(file_ioctl_buffer);
        file_ioctl_buffer = NULL;
    }
}

int  SWITCH_UDFFS::udffs_open     (struct _reent *r, void *fileStruct, const char *path, int flags, int mode){
    auto *priv      = static_cast<SWITCH_UDFFS     *>(r->deviceData);
    auto *priv_file = static_cast<SWITCH_UDFFSFile *>(fileStruct);


    //printf("OPEN FILE: %s\r\n",path);

    if(std::string(path).empty()){
        return -1;
    }
    
    //std::string fullfilename = path;
    if(strcmp(path,"udf0:/ioctl") == 0){
        priv_file->file_ioctl = 1;
        priv_file->offset = 0;
        printf("OPENED IOCTL SPECIAL FILE\r\n");
        
        return 0;
    }
    
    int fileret = priv->UDFFS->FindFile(&path[5]);
    //printf("FILE RET: %s %d\r\n",path,fileret);
    priv_file->file_ioctl = 0;
    priv_file->filelist_id = -1;
    if(fileret<0)return -1;
    
    disc_dirlist_struct * _filedesc = priv->UDFFS->GetFileDescFromIDX(fileret);
    if(_filedesc == NULL)return -1;
    if(_filedesc->streaming){
        //printf("Open STREAMING Mode\r\n");
        //for(int i=0;i<(int)_filedesc->extents.size();i++){
            //priv->UDFFS->usb_scsi_ctx->UsbDvdSetStreamingMode(0,priv->UDFFS->partitionlba+_filedesc->extents[0].location,priv->UDFFS->partitionlba+_filedesc->extents[0].location+_filedesc->extents[0].length,27000,1000);
        //}
        //printf("STREAMING FILE\r\n");
    }
    
    priv_file->filelist_id = fileret;
    priv_file->offset = 0;
    
    
    return 0;
    
}

int  SWITCH_UDFFS::udffs_close    (struct _reent *r, void *fd){
    auto *priv      = static_cast<SWITCH_UDFFS     *>(r->deviceData);
    auto *priv_file = static_cast<SWITCH_UDFFSFile *>(fd);
    if(priv->file_ioctl_buffer) {
        free(priv->file_ioctl_buffer);
        priv->file_ioctl_buffer = nullptr;
    }
    if(priv_file->file_ioctl == 1)priv_file->file_ioctl=0;
    
    return 0;
}

ssize_t SWITCH_UDFFS::udffs_write(struct _reent *r, void *fd, const char *ptr, size_t len) {
    auto *priv = static_cast<SWITCH_UDFFS *>(r->deviceData);
    auto *priv_file = static_cast<SWITCH_UDFFSFile *>(fd);
    
    printf("WRITE OPERATION ON UDF DEVOPTAB %lu\r\n",len);
    
    if(priv_file->file_ioctl){
        
        struct {
            SCSI_PASS_THROUGH_DIRECT sptd;
            uint8_t                    SenseBuf[MAX_SENSE_LEN];
        } sptd_sb;
        
        
        if(priv->file_ioctl_buffer) {
            free(priv->file_ioctl_buffer);
            priv->file_ioctl_buffer = nullptr;
        }
        
        if (len < sizeof(SCSI_PASS_THROUGH_DIRECT)) {
            printf("[ERROR] Invalid command size: %zu < %zu\n", len, sizeof(SCSI_PASS_THROUGH_DIRECT));
            __errno_r(r) = EINVAL;
            return -1;
        }
        
        memcpy(&sptd_sb, ptr, sizeof(sptd_sb));
        printf("NEW CBW: ");
      
        
        
        uint8_t *write_data = nullptr;
        size_t write_data_size = 0;
        
        
        if(sptd_sb.sptd.Cdb[0] == 0xa4 && sptd_sb.sptd.Cdb[7] == 0x02 && sptd_sb.sptd.Cdb[9] == 0x74 && sptd_sb.sptd.Cdb[10] == 0x38){
            sptd_sb.sptd.DataTransferLength = 0x60;
            sptd_sb.sptd.Cdb[9] = 0x60;
        }
        if(sptd_sb.sptd.Cdb[0] == 0xa4 && sptd_sb.sptd.Cdb[7] == 0x02 && sptd_sb.sptd.Cdb[9] == 0x02 && (sptd_sb.sptd.Cdb[10] == 0x3f || sptd_sb.sptd.Cdb[10] == 0x7f || sptd_sb.sptd.Cdb[10] == 0xbf || sptd_sb.sptd.Cdb[10] == 0xff)){
            sptd_sb.sptd.DataTransferLength = 0x00;
            sptd_sb.sptd.Cdb[9] = 0x00;
        }
        for(int i = 0; i < sptd_sb.sptd.CdbLength; i++) {
            printf("%02hhx ", sptd_sb.sptd.Cdb[i]);
        }
        printf("\r\n");
        
        
        if(sptd_sb.sptd.DataIn == SCSI_IOCTL_DATA_IN && sptd_sb.sptd.DataTransferLength > 0) {
            priv->file_ioctl_buffer = (uint8_t *)malloc(sptd_sb.sptd.DataTransferLength);
            if(!priv->file_ioctl_buffer) {
                printf("[ERROR] Failed to allocate response buffer (%zu bytes)\n", sptd_sb.sptd.DataTransferLength);
                __errno_r(r) = ENOMEM;
                return -1;
            }
            printf("[ALLOC] New response buffer allocated (size: %zu)\n", sptd_sb.sptd.DataTransferLength);
        }
        
        if(sptd_sb.sptd.DataIn == SCSI_IOCTL_DATA_OUT && 
               len > sizeof(SCSI_PASS_THROUGH_DIRECT) + MAX_SENSE_LEN) {
            write_data_size = len - (sizeof(sptd_sb));
            write_data = (uint8_t*)ptr + sizeof(sptd_sb);
            printf("[WRITE] Command has %zu bytes of write data\n", write_data_size);
        }
        
        
        int usb_result = 0;
        printf("[USB] Executing SCSI command (DataIn: %d, DataTransferLength: %zu)...\n", 
               sptd_sb.sptd.DataIn, sptd_sb.sptd.DataTransferLength);
        
        if(sptd_sb.sptd.DataIn == SCSI_IOCTL_DATA_IN && priv->file_ioctl_buffer) {
            
            usb_result = priv->UDFFS->usb_scsi_ctx->UsbDvdSCSI_PASSTROUGHT(0, 
                                                                 sptd_sb.sptd.Cdb, 
                                                                 sptd_sb.sptd.CdbLength, 
                                                                 sptd_sb.sptd.DataTransferLength, 
                                                                 sptd_sb.sptd.DataIn, 
                                                                 priv->file_ioctl_buffer);
                                                                 
                                                     
        } else if(sptd_sb.sptd.DataIn == SCSI_IOCTL_DATA_OUT && write_data) {
            
            usb_result = priv->UDFFS->usb_scsi_ctx->UsbDvdSCSI_PASSTROUGHT(0, 
                                                                 sptd_sb.sptd.Cdb, 
                                                                 sptd_sb.sptd.CdbLength, 
                                                                 sptd_sb.sptd.DataTransferLength, 
                                                                 sptd_sb.sptd.DataIn, 
                                                                 write_data);
        } else if(sptd_sb.sptd.DataIn == SCSI_IOCTL_DATA_UNSPECIFIED) {
            
            usb_result = priv->UDFFS->usb_scsi_ctx->UsbDvdSCSI_PASSTROUGHT(0, 
                                                                 sptd_sb.sptd.Cdb, 
                                                                 sptd_sb.sptd.CdbLength, 
                                                                 0, 
                                                                 sptd_sb.sptd.DataIn, 
                                                                 nullptr);
        }
        
        if(usb_result >= 0) {
            
            if(sptd_sb.sptd.DataIn == SCSI_IOCTL_DATA_IN && priv->file_ioctl_buffer) {
                // READ command completato
                priv->file_ioctl_size = sptd_sb.sptd.DataTransferLength;
                priv_file->offset = 0;
                
                printf("RES: ");
                for(uint64_t i = 0; i < sptd_sb.sptd.DataTransferLength; i++) {
                    printf("%02hhx ", priv->file_ioctl_buffer[i]);
                }
                printf("\r\n");
                
                printf("[SUCCESS] READ operation completed (response size: %zu)\n", priv->file_ioctl_size);
            } else if(sptd_sb.sptd.DataIn == SCSI_IOCTL_DATA_OUT) {
                // WRITE command completato
                priv->file_ioctl_size = 0; // Nessuna risposta per write commands
                priv_file->offset = 0;
                printf("[SUCCESS] WRITE operation completed (%zu bytes sent)\n", write_data_size);
            } else {
                // Command senza dati completato
                priv->file_ioctl_size = 0;
                priv_file->offset = 0;
                printf("[SUCCESS] No-data command completed\n");
            }
            return len;
        } else {
            // Errore USB - cleanup immediato
            printf("[ERROR] USB operation failed (result: %d)\n", usb_result);
            if(priv->file_ioctl_buffer) {
                printf("[CLEANUP-ERROR] Freeing failed operation buffer\n");
                free(priv->file_ioctl_buffer);
                priv->file_ioctl_buffer = nullptr;
            }
            priv->file_ioctl_size = 0;
            priv_file->offset = 0;
            
            //__errno_r(r) = EIO;
            return -1;
        }
        
        
        
        
    }
    
    return -1;
}

ssize_t   SWITCH_UDFFS::udffs_read     (struct _reent *r, void *fd, char *ptr, size_t len){
    auto *priv      = static_cast<SWITCH_UDFFS     *>(r->deviceData);
    auto *priv_file = static_cast<SWITCH_UDFFSFile *>(fd);

    auto lk = std::scoped_lock(priv->session_mutex);
    
    //printf("READ REQUEST %lu %lu\r\n",(uint64_t)fd,len);
    
    if(priv_file->file_ioctl == 1){
        if (priv->file_ioctl_buffer == NULL) {
            
            return 0;  
        }
        size_t response_len = priv->file_ioctl_size;
        size_t remaining = response_len - priv_file->offset;
        
        if (remaining == 0) {
            return 0;  // EOF
        }
        
        size_t to_copy = (len < remaining) ? len : remaining;
        memcpy(ptr, priv->file_ioctl_buffer + priv_file->offset, to_copy);
        priv_file->offset += to_copy;
        
        printf("[IOCTL-READ] Returned %zu bytes\n", to_copy);
        return to_copy;
       
    }
    //printf("READ FILE ID: %d\r\n",priv_file->filelist_id);
    if(priv_file->filelist_id>-1){
        disc_dirlist_struct * _filedesc = priv->UDFFS->GetFileDescFromIDX(priv_file->filelist_id);
        //priv->UDFFS->ReadData(_filedesc,priv_file->offset,len,(uint8_t *)ptr);
        if(_filedesc == NULL)return -1;
        priv->UDFFS->UDFReadData(_filedesc,priv_file->offset,len,(uint8_t *)ptr);
        
        //UDFReadData(disc_dirlist_struct * _filedesc,uint32_t pos,uint32_t size,uint8_t * buf)
        
        priv_file->offset=priv_file->offset+len;
    }
    
    return len;

}

off_t     SWITCH_UDFFS::udffs_seek     (struct _reent *r, void *fd, off_t pos, int dir){
    auto *priv      = static_cast<SWITCH_UDFFS     *>(r->deviceData);
    auto *priv_file = static_cast<SWITCH_UDFFSFile *>(fd);

    
    if(priv_file->file_ioctl == 1){
        return priv_file->offset;
    }

    
    if(priv_file->filelist_id<0){
        return -1;
    }
    
    auto lk = std::scoped_lock(priv->session_mutex);

    off_t offset;
    switch (dir) {
        default:
        case SEEK_SET:
            offset = 0;
            break;
        case SEEK_CUR:
            offset = priv_file->offset;
            break;
        case SEEK_END:
            offset = priv->UDFFS->disc_dirlist[priv_file->filelist_id].size;
            break;
    }

    
    
    
    priv_file->offset = offset + pos;
    
    
    
    return priv_file->offset;
}

int       SWITCH_UDFFS::udffs_fstat    (struct _reent *r, void *fd, struct stat *st){
    auto *priv = static_cast<SWITCH_UDFFS *>(r->deviceData);
    auto *priv_file = static_cast<SWITCH_UDFFSFile *>(fd);
    auto lk = std::scoped_lock(priv->session_mutex);
    
    if (priv_file->file_ioctl == 1) {
        st->st_mode = S_IFREG | 0666;  // File regolare
        st->st_size = priv->file_ioctl_size;
        return 0;
    } 
    
    if(priv_file->filelist_id<0){
        return -1;
    }
    
    disc_dirlist_struct * _filedesc = priv->UDFFS->GetFileDescFromIDX(priv_file->filelist_id);
    udffsstat_entry(_filedesc,st);
    
    return 0;
}

int       SWITCH_UDFFS::udffs_stat     (struct _reent *r, const char *file, struct stat *st){
    auto *priv     = static_cast<SWITCH_UDFFS    *>(r->deviceData);
    auto lk = std::scoped_lock(priv->session_mutex);
    
    std::string filename = file;
    
    if (strcmp(file,"udf0:/ioctl") == 0) {
        st->st_mode = S_IFREG | 0666;  // File regolare
        st->st_size = 0;
        return 0;
    }
    
    disc_dirlist_struct myfiledesc;
    int ret = priv->UDFFS->GetFileDesc(&file[5],myfiledesc);
    if(ret <0)return -1;
    udffsstat_entry(&myfiledesc,st);
    return ret;

}

int       SWITCH_UDFFS::udffs_chdir    (struct _reent *r, const char *name){
    
    return 0;
}

DIR_ITER * SWITCH_UDFFS::udffs_diropen  (struct _reent *r, DIR_ITER *dirState, const char *path){
    auto *priv = static_cast<SWITCH_UDFFS *>(r->deviceData);
    auto *priv_dir = static_cast<SWITCH_UDFFSDir *>(dirState->dirStruct);
    
    
    priv->currdirlist.clear();
    for(int i=0;i<(int)priv->UDFFS->disc_dirlist.size();i++){
        std::filesystem::path epath{priv->UDFFS->disc_dirlist[i].fullpath};
        if(epath.parent_path().string() == std::string(&path[5])){
            priv->currdirlist.push_back(priv->UDFFS->disc_dirlist[i]);
        }
                
    }
    
    
    priv_dir->dirnext_idx = 0;
    
    return dirState;
}

int   SWITCH_UDFFS::udffs_dirreset (struct _reent *r, DIR_ITER *dirState){
    __errno_r(r) = ENOSYS;
    return -1;
}

int       SWITCH_UDFFS::udffs_dirnext  (struct _reent *r, DIR_ITER *dirState, char *filename, struct stat *filestat){
    auto *priv     = static_cast<SWITCH_UDFFS    *>(r->deviceData);
    auto *priv_dir = static_cast<SWITCH_UDFFSDir *>(dirState->dirStruct);

    auto lk = std::scoped_lock(priv->session_mutex);
    
    if(priv_dir->dirnext_idx >= (int)priv->currdirlist.size()){
        return -1;
    }
    memset(filename, 0, NAME_MAX);
    
    memcpy(filename,priv->currdirlist[priv_dir->dirnext_idx].name.c_str(),255);
    udffsstat_entry(&priv->currdirlist[priv_dir->dirnext_idx],filestat);
    
    priv_dir->dirnext_idx +=1;
    
    return 0;
}

int       SWITCH_UDFFS::udffs_dirclose (struct _reent *r, DIR_ITER *dirState){
    auto *priv     = static_cast<SWITCH_UDFFS    *>(r->deviceData);
    
    auto lk = std::scoped_lock(priv->session_mutex);
    
    return 0;
}

int       SWITCH_UDFFS::udffs_statvfs  (struct _reent *r, const char *path, struct statvfs *buf){
            
    
    return 0;
}

void udffsstat_entry(disc_dirlist_struct *_filedesc, struct stat *st)
{
    *st = {};
    
    st->st_mode =  _filedesc->isdir ? S_IFDIR : S_IFREG;
    st->st_nlink = 1;
    st->st_uid = 1;
    st->st_gid = 2;
    st->st_size = _filedesc->size;
    st->st_atime = _filedesc->access_time;
    st->st_mtime = _filedesc->modification_time;
    st->st_ctime = _filedesc->attribute_time;
    st->st_blksize = 2048;
}
