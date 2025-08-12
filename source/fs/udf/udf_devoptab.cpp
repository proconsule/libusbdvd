#include "udf_devoptab.h"
#include <filesystem>

void udffsstat_entry(udf_dirlist_struct *_filedesc, struct stat *st);

SWITCH_UDFFS::SWITCH_UDFFS(CUSBDVD_UDFFS *_ctx,std::string _name,std::string _mount_name){

	this->UDFFS = _ctx;
	
	
	this->name       = _name;
    this->mount_name = _mount_name;
	
    this->devoptab = {
        .name         = SWITCH_UDFFS::name.data(),

        .structSize   = sizeof(SWITCH_UDFFSFile),
        .open_r       = SWITCH_UDFFS::udffs_open,
        .close_r      = SWITCH_UDFFS::udffs_close,
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
}

int  SWITCH_UDFFS::udffs_open     (struct _reent *r, void *fileStruct, const char *path, int flags, int mode){
	auto *priv      = static_cast<SWITCH_UDFFS     *>(r->deviceData);
    auto *priv_file = static_cast<SWITCH_UDFFSFile *>(fileStruct);

	if(std::string(path).empty()){
		return -1;
	}
	
	int fileret = priv->UDFFS->FindFile(&path[5]);
	if(fileret<0)return -1;
	
	priv_file->filelist_id = fileret;
	priv_file->offset = 0;
	
    
    return 0;
	
}

int  SWITCH_UDFFS::udffs_close    (struct _reent *r, void *fd){
	//auto *priv      = static_cast<SWITCH_UDFFS     *>(r->deviceData);
    //auto *priv_file = static_cast<SWITCH_UDFFSFile *>(fd);
	
	
    return 0;
}

ssize_t   SWITCH_UDFFS::udffs_read     (struct _reent *r, void *fd, char *ptr, size_t len){
	auto *priv      = static_cast<SWITCH_UDFFS     *>(r->deviceData);
    auto *priv_file = static_cast<SWITCH_UDFFSFile *>(fd);

    auto lk = std::scoped_lock(priv->session_mutex);
	
	udf_dirlist_struct * _filedesc = priv->UDFFS->GetFileDescFromIDX(priv_file->filelist_id);
	priv->UDFFS->ReadData(_filedesc,priv_file->offset,len,(uint8_t *)ptr);
	
	priv_file->offset=priv_file->offset+len;
	
	
	return len;

}

off_t     SWITCH_UDFFS::udffs_seek     (struct _reent *r, void *fd, off_t pos, int dir){
	auto *priv      = static_cast<SWITCH_UDFFS     *>(r->deviceData);
    auto *priv_file = static_cast<SWITCH_UDFFSFile *>(fd);

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

int       SWITCH_UDFFS::udffs_fstat    (struct _reent *r, void *fd, struct stat *st){
	auto *priv = static_cast<SWITCH_UDFFS *>(r->deviceData);
	auto *priv_file = static_cast<SWITCH_UDFFSFile *>(fd);
    auto lk = std::scoped_lock(priv->session_mutex);
	
	udf_dirlist_struct * _filedesc = priv->UDFFS->GetFileDescFromIDX(priv_file->filelist_id);
	udffsstat_entry(_filedesc,st);
	
	return 0;
}

int       SWITCH_UDFFS::udffs_stat     (struct _reent *r, const char *file, struct stat *st){
	auto *priv     = static_cast<SWITCH_UDFFS    *>(r->deviceData);
	
	
	udf_dirlist_struct myfiledesc;
	int ret = priv->UDFFS->GetFileDesc(&file[5],myfiledesc);
	
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
	for(int i=0;i<(int)priv->UDFFS->udf_dirlist.size();i++){
		std::filesystem::path epath{priv->UDFFS->udf_dirlist[i].fullpath};
		if(epath.parent_path().string() == std::string(&path[5])){
			priv->currdirlist.push_back(priv->UDFFS->udf_dirlist[i]);
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

void udffsstat_entry(udf_dirlist_struct *_filedesc, struct stat *st)
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