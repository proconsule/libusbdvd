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
	
	printf( CONSOLE_ESC(2J) );
	
	
	//	Version can be retrived also before lib obj ref is created
	printf(CONSOLE_ESC(4;5H) "USBDVD Library Version %s",usbdvd_version());
	
	// Library init (it will find first compatible drive and mount proper fs
	usbdvd_obj* test = usbdvd_init();
	// For ISO file mount use
	// usbdvd_obj* test = usbdvd_initimage("/pathtoiso.iso");
	
	// Retrive the drive struct ref
	usbdvd_drive_struct *drivectx = usbdvd_get_drivectx(test);
	
	
	// Check if a drive is found or we are using a file image and print info
	if(drivectx->drive_found || drivectx->fileimage){
		printf(CONSOLE_ESC(6;2H)CONSOLE_ESC(0m)"vendor_id:%s %s\r\n"CONSOLE_ESC(0m),CONSOLE_ESC(1m),drivectx->vendor_id);
		printf(CONSOLE_ESC(7;2H)CONSOLE_ESC(0m)"product_id:%s %s\r\n"CONSOLE_ESC(0m),CONSOLE_ESC(1m),drivectx->product_id);
		printf(CONSOLE_ESC(8;2H)CONSOLE_ESC(0m)"product_revision:%s %s\r\n"CONSOLE_ESC(0m),CONSOLE_ESC(1m),drivectx->product_revision);
		printf(CONSOLE_ESC(9;2H)CONSOLE_ESC(0m)"serial_number:%s %s\r\n"CONSOLE_ESC(0m),CONSOLE_ESC(1m),drivectx->serial_number);
		
	
	
		// Check if a supported filesystem was mounted
		if(drivectx->fs.mounted){
			char path[128];
			// drivectx->fs.mountpoint is the mountpoint of current mounted fs
			sprintf(path,"%s/",drivectx->fs.mountpoint);
			printf(CONSOLE_ESC(11;2H)"MOUNT PATH: %s\r\n",path);
			
			// Print various info
			printf(CONSOLE_ESC(12;2H)CONSOLE_ESC(0m)"Volume ID:%s %s"CONSOLE_ESC(0m),CONSOLE_ESC(1m),drivectx->fs.volid);
			printf(CONSOLE_ESC(13;2H)CONSOLE_ESC(0m)"Disc FS:%s %s"CONSOLE_ESC(0m),CONSOLE_ESC(1m),drivectx->fs.disc_fstype);
			printf(CONSOLE_ESC(15;2H)"File List: \n");
			
			// Standatd FS Directory Listing
			
			DIR *d;
			DIR *dir;
			struct stat file_stat;
			
			dir = opendir(path);
			
			if (dir) {
				
				struct _reent *reent    = __syscall_getreent();
				devoptab_t *devoptab = devoptab_list[dir->dirData->device];	
				while (true) {
					reent->deviceData = devoptab->deviceData;
					struct stat st;
					memset(&st,0,sizeof(st));
					if (devoptab->dirnext_r(reent, dir->dirData, dir->fileData.d_name, &st))
						break;
							
					if (( strlen(dir->fileData.d_name) == 1) && dir->fileData.d_name[0] == '.') {
						continue;
					}
					if (( strlen(dir->fileData.d_name) == 2) && dir->fileData.d_name[0] == '.' && dir->fileData.d_name[1] == '.') {
						continue;
					}
					printf(CONSOLE_ESC(1m)"%s %d\n"CONSOLE_ESC(0m),dir->fileData.d_name,st.st_size);
				}
						
				closedir(dir);
			
			}
		}
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
	
	
	// Destory and cleanup the library
	usbdvd_destroy(test);
	consoleExit(NULL);
	romfsExit();
	appletUnlockExit();
	
	
	return 0;
}