#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <dirent.h>

#include <switch.h>
#include <usbdvd.h>



int main(int argc, const char* const* argv) {
	
	appletLockExit();
	romfsInit();
	
	consoleInit(NULL);
	padConfigureInput(1, HidNpadStyleSet_NpadStandard);
	
	PadState pad;
    padInitializeDefault(&pad);
	
	
	
	
	CUSBDVD *testusbdvd = new CUSBDVD();
	
	usbdvd_drive_struct *drivectx = &testusbdvd->usbdvd_drive_ctx;
	
	printf("USBDVD Library Version: %s\r\n",testusbdvd->get_version().c_str());
	printf("vendor_id: %s\r\n",drivectx->vendor_id);
	printf("product_id: %s\r\n",drivectx->product_id);
	printf("product_revision: %s\r\n",drivectx->product_revision);
	printf("serial_number: %s\r\n",drivectx->serial_number);
	printf("Disc Type: %s\r\n",drivectx->disc_type);
	
	if(drivectx->fs.mounted){
		printf("Disc FS: %s\r\n",drivectx->fs.disc_fstype);
		struct dirent *ent;
			DIR *dir;
			std::string path = drivectx->fs.mountpoint + std::string("/");
			printf("PATH: %s\r\n",path.c_str());
			if (!path.empty()) {
				if ((dir = opendir(path.c_str())) != nullptr) {
				
					auto *reent    = __syscall_getreent();
					auto *devoptab = devoptab_list[dir->dirData->device];	
					
					
					
					while (true) {
							reent->deviceData = devoptab->deviceData;
							struct stat st{0};
							if (devoptab->dirnext_r(reent, dir->dirData, dir->fileData.d_name, &st))
							break;
							
							if (( strlen(dir->fileData.d_name) == 1) && dir->fileData.d_name[0] == '.') {
								continue;
							}
							if (( strlen(dir->fileData.d_name) == 2) && dir->fileData.d_name[0] == '.' && dir->fileData.d_name[1] == '.') {
								continue;
							}
							
							
							printf("%s %d\r\n",dir->fileData.d_name,st.st_size);
							
							
						}
						
						closedir(dir);
			
						
						
					}
				
			}
			
	}else{
		printf("NO USB DVD FOUND\r\n");
	}
	
	while(appletMainLoop()){
		 padUpdate(&pad);

        // padGetButtonsDown returns the set of buttons that have been newly pressed in this frame compared to the previous one
        u64 kDown = padGetButtonsDown(&pad);

        if (kDown & HidNpadButton_Plus) break; // break in order to return to hbmenu

		if (kDown & HidNpadButton_B) {
			
		}
	
        consoleUpdate(NULL);
		
		
	}
	

	delete testusbdvd;
	
	consoleExit(NULL);
	romfsExit();
	appletUnlockExit();
	
	
	return 0;
}