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
	
	printf("vendor_id: %s\r\n",testusbdvd->vendor_id.c_str());
	printf("product_id: %s\r\n",testusbdvd->product_id.c_str());
	printf("product_revision: %s\r\n",testusbdvd->product_revision.c_str());
	printf("serial_number: %s\r\n",testusbdvd->serial_number.c_str());
	printf("Disc Type: %s\r\n",testusbdvd->disctype.c_str());
	

	
	if(testusbdvd->acd_init_ok || testusbdvd->datacd_init_ok){
	
		struct dirent *ent;
			DIR *dir;
			std::string path = testusbdvd->mountpoint + std::string("/");
			//std::string path = "iso0:/";
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
	/*
	std::string aaatest = testusbdvd->mountpoint + std::string("/")+"2003 - Splinter/02. The Noose.mp3";
	
	FILE *aaa = fopen(aaatest.c_str(),"rb");
	char buff[0x1000];
	fread(buff,sizeof(char),0x100,aaa);
	fclose(aaa);
*/
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