#ifndef SWITCH_UDF_DEVOPTAB_H
#define SWITCH_UDF_DEVOPTAB_H

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

#include "usbdvd_udf.h"
#include <switch.h>



class SWITCH_UDFFS{
public:
	SWITCH_UDFFS(CUSBDVD_UDFFS* _ctx,std::string _name,std::string _mount_name);
	~SWITCH_UDFFS();
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
	
	static int       udffs_open     (struct _reent *r, void *fileStruct, const char *path, int flags, int mode);
	static int       udffs_close    (struct _reent *r, void *fd);
	static ssize_t   udffs_read     (struct _reent *r, void *fd, char *ptr, size_t len);
	static off_t     udffs_seek     (struct _reent *r, void *fd, off_t pos, int dir);
	static int       udffs_fstat    (struct _reent *r, void *fd, struct stat *st);
	static int       udffs_stat     (struct _reent *r, const char *file, struct stat *st);
	static int       udffs_chdir    (struct _reent *r, const char *name);
	static DIR_ITER *udffs_diropen  (struct _reent *r, DIR_ITER *dirState, const char *path);
	static int       udffs_dirreset (struct _reent *r, DIR_ITER *dirState);
	static int       udffs_dirnext  (struct _reent *r, DIR_ITER *dirState, char *filename, struct stat *filestat);
	static int       udffs_dirclose (struct _reent *r, DIR_ITER *dirState);
	static int       udffs_statvfs  (struct _reent *r, const char *path, struct statvfs *buf);
	static int       udffs_lstat    (struct _reent *r, const char *file, struct stat *st);
	
	bool is_connected = false;
	
	CUSBDVD_UDFFS *UDFFS;
	
private:
	std::string connect_url;
	
	int connect();
	void disconnect();
	
	int find_fileentry(std::string filepath);

	
	struct SWITCH_UDFFSFile {
		//iso9660_dirlist_struct filedesc;
		int filelist_id;
		off_t offset = 0;
	};

	struct SWITCH_UDFFSDir {
		
		char dirpath[256] = "";
		int dirnext_idx = 0;
	};
	
	std::string cwd = "";
	std::mutex session_mutex;
	
	std::vector<udf_dirlist_struct> currdirlist;
	
protected:
	devoptab_t devoptab = {};
	
};


#endif