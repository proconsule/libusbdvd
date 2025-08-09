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
	
	usbdvd_obj* test = usbdvd_init();
	// usbdvd_obj* test = usbdvd_initimage("/pathtoiso.iso");  //FILE MOUNT
	usbdvd_drive_struct *drivectx = usbdvd_get_drivectx(test);
	
	printf("USBDVD Library Version: %s\r\n",usbdvd_version());
	
	
	printf("vendor_id: %s\r\n",drivectx->vendor_id);
	printf("product_id: %s\r\n",drivectx->product_id);
	printf("product_revision: %s\r\n",drivectx->product_revision);
	printf("serial_number: %s\r\n",drivectx->serial_number);
	printf("Disc Type: %s\r\n",drivectx->disc_type);
	char path[128];
		sprintf(path,"%s/",drivectx->fs.mountpoint);
		printf("PATH: %s\r\n",path);
	
	if(drivectx->fs.mounted){
		printf("Volume ID: %s\r\n",drivectx->fs.volid);
		printf("Disc FS: %s\r\n",drivectx->fs.disc_fstype);
		struct dirent *ent;
		char path[128];
		sprintf(path,"%s/",drivectx->fs.mountpoint);
		printf("PATH: %s\r\n",path);
		DIR *d;
		struct dirent *dir;
		d = opendir(path);
		if (d) {
			while ((dir = readdir(d)) != NULL) {
			printf("%s\n", dir->d_name);
			}
			closedir(d);
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
	
	usbdvd_destroy(test);
	consoleExit(NULL);
	romfsExit();
	appletUnlockExit();
	
	
	return 0;
}