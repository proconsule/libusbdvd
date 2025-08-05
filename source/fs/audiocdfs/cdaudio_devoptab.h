#ifndef SWITCH_AUDIOCDFS_DEVOPTAB_H
#define SWITCH_AUDIOCDFS_DEVOPTAB_H

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sys/dirent.h>
#include <sys/iosupport.h>
#include <sys/param.h>
#include <unistd.h>

#include <mutex>
#include <vector>

#include "audiocdfs.h"
#include <switch.h>



class SWITCH_AUDIOCDFS{
public:
	SWITCH_AUDIOCDFS(CAUDIOCD_PSEUDOFS* _ctx,std::string _name,std::string _mount_name);
	~SWITCH_AUDIOCDFS();
	std::string name, mount_name;
	
	
	int unregister_fs() const {
		return RemoveDevice(this->mount_name.data());
	}
	
	int register_fs() const {
		
		auto id = FindDevice(this->mount_name.data());
		if (id < 0){
			id = AddDevice(&this->devoptab);
		}
		if (id < 0)
			return id;

		return 0;
	}
	
	static int       cdaudiofs_open     (struct _reent *r, void *fileStruct, const char *path, int flags, int mode);
	static int       cdaudiofs_close    (struct _reent *r, void *fd);
	static ssize_t   cdaudiofs_read     (struct _reent *r, void *fd, char *ptr, size_t len);
	static off_t     cdaudiofs_seek     (struct _reent *r, void *fd, off_t pos, int dir);
	static int       cdaudiofs_fstat    (struct _reent *r, void *fd, struct stat *st);
	static int       cdaudiofs_stat     (struct _reent *r, const char *file, struct stat *st);
	static int       cdaudiofs_chdir    (struct _reent *r, const char *name);
	static DIR_ITER *cdaudiofs_diropen  (struct _reent *r, DIR_ITER *dirState, const char *path);
	static int       cdaudiofs_dirreset (struct _reent *r, DIR_ITER *dirState);
	static int       cdaudiofs_dirnext  (struct _reent *r, DIR_ITER *dirState, char *filename, struct stat *filestat);
	static int       cdaudiofs_dirclose (struct _reent *r, DIR_ITER *dirState);
	static int       cdaudiofs_statvfs  (struct _reent *r, const char *path, struct statvfs *buf);
	static int       cdaudiofs_lstat    (struct _reent *r, const char *file, struct stat *st);
	
	bool is_connected = false;
	
	CAUDIOCD_PSEUDOFS *CDAUDIOPFS;
	
private:
	std::string connect_url;
	
	int connect();
	void disconnect();
	
	int find_fileentry(std::string filepath);

	
	struct SWITCH_AUDIOCDFSFile {
			
		int trackid = 0;
		off_t offset = 0;
	};

	struct SWITCH_AUDIOCDFSDir {
		char dirpath[256] = "";
		int dirnext_idx = 0;
	};
	
	std::string cwd = "";
	std::mutex session_mutex;
	
	
protected:
	devoptab_t devoptab = {};
	
};


#endif