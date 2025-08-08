#include "cdaudio_devoptab.h"
#include <filesystem>

void cdaudiostat_entry(int _tracksize, struct stat *st);

SWITCH_AUDIOCDFS::SWITCH_AUDIOCDFS(CAUDIOCD_PSEUDOFS *_ctx,std::string _name,std::string _mount_name){

	this->CDAUDIOPFS = _ctx;
	
	
	this->name       = _name;
    this->mount_name = _mount_name;
	
    this->devoptab = {
        .name         = SWITCH_AUDIOCDFS::name.data(),

        .structSize   = sizeof(SWITCH_AUDIOCDFSFile),
        .open_r       = SWITCH_AUDIOCDFS::cdaudiofs_open,
        .close_r      = SWITCH_AUDIOCDFS::cdaudiofs_close,
        .read_r       = SWITCH_AUDIOCDFS::cdaudiofs_read,
        .seek_r       = SWITCH_AUDIOCDFS::cdaudiofs_seek,
        .fstat_r      = SWITCH_AUDIOCDFS::cdaudiofs_fstat,

        .stat_r       = SWITCH_AUDIOCDFS::cdaudiofs_stat,
        .chdir_r      = SWITCH_AUDIOCDFS::cdaudiofs_chdir,

        .dirStateSize = sizeof(SWITCH_AUDIOCDFSDir),
        .diropen_r    = SWITCH_AUDIOCDFS::cdaudiofs_diropen,
        .dirreset_r   = SWITCH_AUDIOCDFS::cdaudiofs_dirreset,
        .dirnext_r    = SWITCH_AUDIOCDFS::cdaudiofs_dirnext,
        .dirclose_r   = SWITCH_AUDIOCDFS::cdaudiofs_dirclose,

        .statvfs_r    = SWITCH_AUDIOCDFS::cdaudiofs_statvfs,

        .deviceData   = this,

        .lstat_r      = SWITCH_AUDIOCDFS::cdaudiofs_stat,
    };
	
	if(connect() == 0){
		register_fs();
	}

}

int SWITCH_AUDIOCDFS::connect(){
	
	
	return 0;
}


SWITCH_AUDIOCDFS::~SWITCH_AUDIOCDFS(){
	
	unregister_fs();
}

int  SWITCH_AUDIOCDFS::cdaudiofs_open     (struct _reent *r, void *fileStruct, const char *path, int flags, int mode){
	auto *priv      = static_cast<SWITCH_AUDIOCDFS     *>(r->deviceData);
    auto *priv_file = static_cast<SWITCH_AUDIOCDFSFile *>(fileStruct);

	if(std::string(path).empty()){
		return -1;
	}
	
	std::filesystem::path const p(path);
	
	int _tid = priv->CDAUDIOPFS->audiocdfs_File2Track(p.filename().c_str());
	if(_tid<0)return -1;
	priv_file->trackid = _tid;
	priv_file->offset = 0;
	
    
    return 0;
	
}

int  SWITCH_AUDIOCDFS::cdaudiofs_close    (struct _reent *r, void *fd){
	
    return 0;
}

ssize_t   SWITCH_AUDIOCDFS::cdaudiofs_read     (struct _reent *r, void *fd, char *ptr, size_t len){
	auto *priv      = static_cast<SWITCH_AUDIOCDFS     *>(r->deviceData);
    auto *priv_file = static_cast<SWITCH_AUDIOCDFSFile *>(fd);

    auto lk = std::scoped_lock(priv->session_mutex);
	
	priv->CDAUDIOPFS->audiocdfs_readdata(priv_file->trackid,priv_file->offset,len,(uint8_t *)ptr);
	
	
	priv_file->offset=priv_file->offset+len;
	
	
	return len;

}

off_t     SWITCH_AUDIOCDFS::cdaudiofs_seek     (struct _reent *r, void *fd, off_t pos, int dir){
	auto *priv      = static_cast<SWITCH_AUDIOCDFS     *>(r->deviceData);
    auto *priv_file = static_cast<SWITCH_AUDIOCDFSFile *>(fd);

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
            offset = priv->CDAUDIOPFS->audiocdfs_gettrackfilesize(priv_file->trackid);
            break;
    }

    
	auto lk = std::scoped_lock(priv->session_mutex);
	
	priv_file->offset = offset + pos;
	
	
	
    return priv_file->offset;
}

int       SWITCH_AUDIOCDFS::cdaudiofs_fstat    (struct _reent *r, void *fd, struct stat *st){
	auto *priv = static_cast<SWITCH_AUDIOCDFS *>(r->deviceData);
	auto *priv_file = static_cast<SWITCH_AUDIOCDFSFile *>(fd);
    auto lk = std::scoped_lock(priv->session_mutex);
	
	cdaudiostat_entry(priv->CDAUDIOPFS->audiocdfs_gettrackfilesize(priv_file->trackid),st);
	
	return 0;
}

int       SWITCH_AUDIOCDFS::cdaudiofs_stat     (struct _reent *r, const char *file, struct stat *st){
	auto *priv     = static_cast<SWITCH_AUDIOCDFS    *>(r->deviceData);
	
	std::filesystem::path const p(file);
	
	int _tid = priv->CDAUDIOPFS->audiocdfs_File2Track(p.filename().c_str());
	
	if(_tid<0){
		
		return -1;
	}
	
	cdaudiostat_entry(priv->CDAUDIOPFS->audiocdfs_gettrackfilesize(_tid), st);
	return 0;

}

int       SWITCH_AUDIOCDFS::cdaudiofs_chdir    (struct _reent *r, const char *name){
	
    return 0;
}

DIR_ITER * SWITCH_AUDIOCDFS::cdaudiofs_diropen  (struct _reent *r, DIR_ITER *dirState, const char *path){
	
	auto *priv_dir = static_cast<SWITCH_AUDIOCDFSDir *>(dirState->dirStruct);
	
	priv_dir->dirnext_idx = 0;
	
	return dirState;
}

int   SWITCH_AUDIOCDFS::cdaudiofs_dirreset (struct _reent *r, DIR_ITER *dirState){
	__errno_r(r) = ENOSYS;
    return -1;
}

int       SWITCH_AUDIOCDFS::cdaudiofs_dirnext  (struct _reent *r, DIR_ITER *dirState, char *filename, struct stat *filestat){
	auto *priv     = static_cast<SWITCH_AUDIOCDFS    *>(r->deviceData);
    auto *priv_dir = static_cast<SWITCH_AUDIOCDFSDir *>(dirState->dirStruct);

    auto lk = std::scoped_lock(priv->session_mutex);
	
	if(priv_dir->dirnext_idx >= priv->CDAUDIOPFS->audiocdfs_gettracknums()){
		priv_dir->dirnext_idx = 0;
		return -1;
	}
	
	
	memset(filename, 0, NAME_MAX);
	
	memcpy(filename,priv->CDAUDIOPFS->audiocdfs_Track2Name(priv_dir->dirnext_idx).c_str(),255);
	cdaudiostat_entry(priv->CDAUDIOPFS->audiocdfs_gettrackfilesize(priv_dir->dirnext_idx),filestat);
	
	priv_dir->dirnext_idx +=1;
	
	return 0;
}

int       SWITCH_AUDIOCDFS::cdaudiofs_dirclose (struct _reent *r, DIR_ITER *dirState){
	auto *priv     = static_cast<SWITCH_AUDIOCDFS    *>(r->deviceData);
    
	auto lk = std::scoped_lock(priv->session_mutex);
	
	
	
	return 0;
}

int       SWITCH_AUDIOCDFS::cdaudiofs_statvfs  (struct _reent *r, const char *path, struct statvfs *buf){
			
	
	return 0;
}

void cdaudiostat_entry(int _tracksize, struct stat *st)
{
	*st = {};
	
	st->st_mode =   S_IFREG;
	st->st_nlink = 1;
	st->st_uid = 1;
	st->st_gid = 2;
	st->st_size = _tracksize;
	st->st_atime = 0;
	st->st_mtime = 0;
	st->st_ctime = 0;
	
}
