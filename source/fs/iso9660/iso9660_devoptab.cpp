#include "iso9660_devoptab.h"
#include <filesystem>

void iso9660fsstat_entry(iso9660_dirlist_struct *_filedesc, struct stat *st);

SWITCH_ISO9660FS::SWITCH_ISO9660FS(CUSBDVD_ISO9660FS *_ctx,std::string _name,std::string _mount_name){

	this->ISO9660FS = _ctx;
	
	
	this->name       = _name;
    this->mount_name = _mount_name;
	
    this->devoptab = {
        .name         = SWITCH_ISO9660FS::name.data(),

        .structSize   = sizeof(SWITCH_ISO9660FSFile),
        .open_r       = SWITCH_ISO9660FS::iso9660fs_open,
        .close_r      = SWITCH_ISO9660FS::iso9660fs_close,
        .read_r       = SWITCH_ISO9660FS::iso9660fs_read,
        .seek_r       = SWITCH_ISO9660FS::iso9660fs_seek,
        .fstat_r      = SWITCH_ISO9660FS::iso9660fs_fstat,

        .stat_r       = SWITCH_ISO9660FS::iso9660fs_stat,
        .chdir_r      = SWITCH_ISO9660FS::iso9660fs_chdir,

        .dirStateSize = sizeof(SWITCH_ISO9660FSDir),
        .diropen_r    = SWITCH_ISO9660FS::iso9660fs_diropen,
        .dirreset_r   = SWITCH_ISO9660FS::iso9660fs_dirreset,
        .dirnext_r    = SWITCH_ISO9660FS::iso9660fs_dirnext,
        .dirclose_r   = SWITCH_ISO9660FS::iso9660fs_dirclose,

        .statvfs_r    = SWITCH_ISO9660FS::iso9660fs_statvfs,

        .deviceData   = this,

        .lstat_r      = SWITCH_ISO9660FS::iso9660fs_stat,
    };
	
	if(connect() == 0){
		register_fs();
	}

}

int SWITCH_ISO9660FS::connect(){
	
	
	return 0;
}


SWITCH_ISO9660FS::~SWITCH_ISO9660FS(){
	
	unregister_fs();
}

int  SWITCH_ISO9660FS::iso9660fs_open     (struct _reent *r, void *fileStruct, const char *path, int flags, int mode){
	auto *priv      = static_cast<SWITCH_ISO9660FS     *>(r->deviceData);
    auto *priv_file = static_cast<SWITCH_ISO9660FSFile *>(fileStruct);

	if(std::string(path).empty()){
		return -1;
	}
	
	int fileret = priv->ISO9660FS->FindFile(&path[5]);
	if(fileret<0)return -1;
	
	priv_file->filelist_id = fileret;
	priv_file->offset = 0;
	
    
    return 0;
	
}

int  SWITCH_ISO9660FS::iso9660fs_close    (struct _reent *r, void *fd){
	//auto *priv      = static_cast<SWITCH_ISO9660FS     *>(r->deviceData);
    //auto *priv_file = static_cast<SWITCH_ISO9660FSFile *>(fd);
	
	
    return 0;
}

ssize_t   SWITCH_ISO9660FS::iso9660fs_read     (struct _reent *r, void *fd, char *ptr, size_t len){
	auto *priv      = static_cast<SWITCH_ISO9660FS     *>(r->deviceData);
    auto *priv_file = static_cast<SWITCH_ISO9660FSFile *>(fd);

    auto lk = std::scoped_lock(priv->session_mutex);
	
	iso9660_dirlist_struct * _filedesc = priv->ISO9660FS->GetFileDescFromIDX(priv_file->filelist_id);
	priv->ISO9660FS->ReadData(_filedesc,priv_file->offset,len,(uint8_t *)ptr);
	
	priv_file->offset=priv_file->offset+len;
	
	
	return len;

}

off_t     SWITCH_ISO9660FS::iso9660fs_seek     (struct _reent *r, void *fd, off_t pos, int dir){
	auto *priv      = static_cast<SWITCH_ISO9660FS     *>(r->deviceData);
    auto *priv_file = static_cast<SWITCH_ISO9660FSFile *>(fd);

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
            offset = priv->currdirlist[priv_file->filelist_id].size;
            break;
    }

    
	auto lk = std::scoped_lock(priv->session_mutex);
	
	priv_file->offset = offset + pos;
	
	
	
    return priv_file->offset;
}

int       SWITCH_ISO9660FS::iso9660fs_fstat    (struct _reent *r, void *fd, struct stat *st){
	auto *priv = static_cast<SWITCH_ISO9660FS *>(r->deviceData);
	auto *priv_file = static_cast<SWITCH_ISO9660FSFile *>(fd);
    auto lk = std::scoped_lock(priv->session_mutex);
	
	iso9660fsstat_entry(&priv->currdirlist[priv_file->filelist_id],st);
	return 0;
}

int       SWITCH_ISO9660FS::iso9660fs_stat     (struct _reent *r, const char *file, struct stat *st){
	auto *priv     = static_cast<SWITCH_ISO9660FS    *>(r->deviceData);
	
	
	iso9660_dirlist_struct myfiledesc;
	int ret = priv->ISO9660FS->GetFileDesc(&file[5],myfiledesc);
	
	iso9660fsstat_entry(&myfiledesc,st);
	return ret;

}

int       SWITCH_ISO9660FS::iso9660fs_chdir    (struct _reent *r, const char *name){
	
    return 0;
}

DIR_ITER * SWITCH_ISO9660FS::iso9660fs_diropen  (struct _reent *r, DIR_ITER *dirState, const char *path){
	auto *priv = static_cast<SWITCH_ISO9660FS *>(r->deviceData);
	auto *priv_dir = static_cast<SWITCH_ISO9660FSDir *>(dirState->dirStruct);
	
	
	priv->currdirlist.clear();
	for(int i=0;i<(int)priv->ISO9660FS->iso9660_dirlist.size();i++){
		std::filesystem::path epath{priv->ISO9660FS->iso9660_dirlist[i].fullpath};
		if(epath.parent_path().string() == std::string(&path[5])){
			priv->currdirlist.push_back(priv->ISO9660FS->iso9660_dirlist[i]);
		}
                
	}
	
	
	priv_dir->dirnext_idx = 0;
	
	return dirState;
}

int   SWITCH_ISO9660FS::iso9660fs_dirreset (struct _reent *r, DIR_ITER *dirState){
	__errno_r(r) = ENOSYS;
    return -1;
}

int       SWITCH_ISO9660FS::iso9660fs_dirnext  (struct _reent *r, DIR_ITER *dirState, char *filename, struct stat *filestat){
	auto *priv     = static_cast<SWITCH_ISO9660FS    *>(r->deviceData);
    auto *priv_dir = static_cast<SWITCH_ISO9660FSDir *>(dirState->dirStruct);

    auto lk = std::scoped_lock(priv->session_mutex);
	
	if(priv_dir->dirnext_idx >= (int)priv->currdirlist.size()){
		return -1;
	}
	memset(filename, 0, NAME_MAX);
	
	memcpy(filename,priv->currdirlist[priv_dir->dirnext_idx].name.c_str(),255);
	iso9660fsstat_entry(&priv->currdirlist[priv_dir->dirnext_idx],filestat);
	
	priv_dir->dirnext_idx +=1;
	
	return 0;
}

int       SWITCH_ISO9660FS::iso9660fs_dirclose (struct _reent *r, DIR_ITER *dirState){
	auto *priv     = static_cast<SWITCH_ISO9660FS    *>(r->deviceData);
    
	auto lk = std::scoped_lock(priv->session_mutex);
	
	
	
	return 0;
}

int       SWITCH_ISO9660FS::iso9660fs_statvfs  (struct _reent *r, const char *path, struct statvfs *buf){
			
	
	return 0;
}

void iso9660fsstat_entry(iso9660_dirlist_struct *_filedesc, struct stat *st)
{
	*st = {};
	
	st->st_mode =  _filedesc->isdir ? S_IFDIR : S_IFREG;
	st->st_nlink = 1;
	st->st_uid = 1;
	st->st_gid = 2;
	st->st_size = _filedesc->size;
	st->st_atime = _filedesc->utc_time;
	st->st_mtime = _filedesc->utc_time;
	st->st_ctime = _filedesc->utc_time;
	
}
