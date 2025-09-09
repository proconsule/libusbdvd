#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <dirent.h>

#include <switch.h>
#include <usbdvd.h>


int getDvdRegion(unsigned char rmi_byte, int regions[]) {
    int count = 0;
    
    for (int i = 0; i < 8; i++) {
        unsigned char mask = 1 << i;
        if ((rmi_byte & mask) == 0) {
            regions[count] = i + 1;
            count++;
        }
    }
    
    return count;
}


void formatBytes(long long bytes, char *result, size_t result_size) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB", "PB"};
    int unit = 0;
    double size = (double)bytes;
    
    while (size >= 1024.0 && unit < 5) {
        size /= 1024.0;
        unit++;
    }
    
    if (unit == 0) {
        snprintf(result, result_size, "%lld %s", bytes, units[unit]);
    } else {
        snprintf(result, result_size, "%.2f %s", size, units[unit]);
    }
}

void unixToDate(time_t timestamp, char *result, size_t result_size) {
    struct tm *timeinfo = localtime(&timestamp);
    strftime(result, result_size, "%d/%m/%Y", timeinfo);
}

uint32_t cursor_idx = 0;
uint32_t currlist_len = 0;

typedef struct{
	char currname[2048];
	bool isdir;
}currentry_struct;

currentry_struct currentry;

void print_drive_info(usbdvd_obj* test,char * _path){
	
	printf( CONSOLE_ESC(2J) );
	
	//	Version can be retrived without lib obj ref
	printf(CONSOLE_ESC(3;25H) "USBDVD Library Version %s",usbdvd_version());
	
	usbdvd_struct *usbdvdctx = usbdvd_get_ctx(test);
	// Check if a drive is found or we are using a file image and print info
	if(usbdvdctx->drive.drive_found || usbdvdctx->drive.fileimage){
		printf(CONSOLE_ESC(5;2H)CONSOLE_ESC(0m)"vendor_id:%s %s\r\n"CONSOLE_ESC(0m),CONSOLE_ESC(1m),usbdvdctx->drive.vendor_id);
		printf(CONSOLE_ESC(6;2H)CONSOLE_ESC(0m)"product_id:%s %s\r\n"CONSOLE_ESC(0m),CONSOLE_ESC(1m),usbdvdctx->drive.product_id);
		printf(CONSOLE_ESC(7;2H)CONSOLE_ESC(0m)"product_revision:%s %s\r\n"CONSOLE_ESC(0m),CONSOLE_ESC(1m),usbdvdctx->drive.product_revision);
		printf(CONSOLE_ESC(8;2H)CONSOLE_ESC(0m)"serial_number:%s %s\r\n"CONSOLE_ESC(0m),CONSOLE_ESC(1m),usbdvdctx->drive.serial_number);
		printf(CONSOLE_ESC(9;2H)CONSOLE_ESC(0m)"Disc Type:%s %s\r\n"CONSOLE_ESC(0m),CONSOLE_ESC(1m),usbdvdctx->drive.disc_type);
		int printoff = 0;
		
		// Check if a supported filesystem was mounted
		if(usbdvdctx->fs.mounted && strcmp(_path,"") != 0){
			
			if(usbdvdctx->fs.disc_fsidx == USBDVD_FS_UTF){
				printf(CONSOLE_ESC(11;40H)"UDF Extended info");
				printf(CONSOLE_ESC(12;40H)"UDF Version num"CONSOLE_ESC(1m)" %x"CONSOLE_ESC(0m),usbdvdctx->fs.udf_extended.udfver);
				if(usbdvdctx->fs.udf_extended.udfver>=0x250){
					char size_str[50];
					formatBytes(usbdvdctx->fs.udf_extended.metadata_partition_size,size_str,sizeof(size_str));
			
					printf(CONSOLE_ESC(13;40H)"Number of partitions:"CONSOLE_ESC(1m)" %u"CONSOLE_ESC(0m),usbdvdctx->fs.udf_extended.number_of_partition_maps);
					printf(CONSOLE_ESC(14;40H)"Metadata Parition size:"CONSOLE_ESC(1m)" %s"CONSOLE_ESC(0m),size_str);
				}
				
				
			}
			
			
			char path[128];
			// usbdvdctx->fs.mountpoint is the mountpoint of current mounted fs
			sprintf(path,"%s",_path);
			printf(CONSOLE_ESC(11;2H)CONSOLE_ESC(0m)"MOUNT PATH:%s %s\r\n",CONSOLE_ESC(1m),usbdvdctx->fs.mountpoint);
			
			// Print various info
			printf(CONSOLE_ESC(12;2H)CONSOLE_ESC(0m)"Volume ID: %s %s"CONSOLE_ESC(0m),CONSOLE_ESC(1m),usbdvdctx->fs.volid);
			printf(CONSOLE_ESC(13;2H)CONSOLE_ESC(0m)"Disc FS: %s %s"CONSOLE_ESC(0m),CONSOLE_ESC(1m),usbdvdctx->fs.disc_fstype);
			printf(CONSOLE_ESC(14;2H)CONSOLE_ESC(0m)"Block Size: %s %u"CONSOLE_ESC(0m),CONSOLE_ESC(1m),usbdvdctx->fs.block_size);
			if(usbdvdctx->dvd_protection.CSS){
				printf(CONSOLE_ESC(15;2H)CONSOLE_ESC(1m)"Disc CSS Encrypted\r\n"CONSOLE_ESC(0m));
				int myregions[8];
				int rcnt = getDvdRegion(usbdvdctx->dvd_protection.regions,myregions);
				printf(CONSOLE_ESC(16;2H)CONSOLE_ESC(0m)"Disc Regions: "CONSOLE_ESC(0m));
				for(int i=0;i<rcnt;i++){
					printf("%d ",myregions[i]);
				}
				printf("\r\n");
				printf(CONSOLE_ESC(17;2H)CONSOLE_ESC(1m)"Enabled Transparent CSS Descramble\t"CONSOLE_ESC(0m)"Time taken for DECSS"CONSOLE_ESC(1m)" %.02f seconds\r\n"CONSOLE_ESC(0m),usbdvdctx->dvd_protection.decss_msecs/1000.0);
				printf(CONSOLE_ESC(18;2H)CONSOLE_ESC(0m)"Entry in CSS cache"CONSOLE_ESC(1m)" %u\r\n"CONSOLE_ESC(0m),usbdvdctx->dvd_protection.cache_keys);
                
			}
            
            if(usbdvdctx->bluray_protection.AACS){
                printf(CONSOLE_ESC(15;2H)CONSOLE_ESC(1m)"Disc AACS Encrypted\r\n"CONSOLE_ESC(0m));
            }
				
            if(usbdvdctx->dvd_protection.CSS || usbdvdctx->bluray_protection.AACS){
                printf(CONSOLE_ESC(19;2H)"File List: %s\n",path);
				printf(CONSOLE_ESC(20;2H)"\n");
            }else{
				printf(CONSOLE_ESC(17;2H)"File List: %s\n",path);
				printf(CONSOLE_ESC(18;2H)"\n");
			}
			
			
			
			// Standatd FS Directory Listing
			
			DIR *d;
			DIR *dir;
			struct stat file_stat;
			memset(currentry.currname,0,sizeof(currentry.currname));
			dir = opendir(path);
			uint32_t fidx = 0;
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
					
					char size_str[50];
					formatBytes(st.st_size,size_str,sizeof(size_str));
					
					char date_str[20];
					unixToDate(st.st_mtime, date_str, sizeof(date_str));
					
					
					if(fidx==cursor_idx){
						printf(CONSOLE_ESC(1m)"->\t%s %s %s %s\n"CONSOLE_ESC(0m),st.st_mode == S_IFDIR ? "<dir> ":"<file>",dir->fileData.d_name,date_str,size_str);
						memset(currentry.currname,0,sizeof(currentry.currname));
						sprintf(currentry.currname,"%s",dir->fileData.d_name);
						currentry.isdir = st.st_mode == S_IFDIR;
					}else{
						printf(CONSOLE_ESC(0m)"\t%s %s %s %s\n"CONSOLE_ESC(0m),st.st_mode == S_IFDIR ? "<dir> ":"<file>",dir->fileData.d_name,date_str,size_str);
						
					}
					
					fidx++;
				}
						
				closedir(dir);
				currlist_len = fidx;
			}
			
		}
		if(usbdvdctx->fs.mounted && strcmp(_path,"") != 0){
			printf(CONSOLE_ESC(40;5H)CONSOLE_ESC(1m)"Y"CONSOLE_ESC(0m)": Eject Drive "CONSOLE_ESC(1m)"X"CONSOLE_ESC(0m)": Mount Drive "CONSOLE_ESC(1m)"UP/Down"CONSOLE_ESC(0m)": Navigation "CONSOLE_ESC(1m)"A"CONSOLE_ESC(0m)": Select "CONSOLE_ESC(1m)"B"CONSOLE_ESC(0m)": Back\n");
		}else{
			
			printf(CONSOLE_ESC(40;25H)""CONSOLE_ESC(1m)"Y"CONSOLE_ESC(0m)": Eject Drive "CONSOLE_ESC(1m)"X"CONSOLE_ESC(0m)": Mount Drive\n");
		}
	}
	
	
}


char* get_directory(const char *path) {
    char *path_copy = strdup(path);  // Crea una copia
    char *last_slash = strrchr(path_copy, '/');
    
    if (last_slash != NULL) {
        *(last_slash) = '\0';  // Termina la stringa all'ultimo slash
    }
    
    return path_copy;  // Ricorda di fare free() dopo l'uso
}

int main(int argc, const char* const* argv) {
	
	appletLockExit();
	romfsInit();
	
    //socketInitializeDefault();
    //nxlinkStdio();
    
	consoleInit(NULL);
	padConfigureInput(1, HidNpadStyleSet_NpadStandard);
	
	PadState pad;
    padInitializeDefault(&pad);
	
	// Library init (it will find first compatible drive and mount proper fs
	usbdvd_obj* test = usbdvd_init();
	// For ISO file mount use
	// usbdvd_obj* test = usbdvd_initimage("/pathtoiso.iso");
	// For CUE/BIN mount use
	//usbdvd_obj* test = usbdvd_initcuebin("/pathtofile.cue","/pathtofile.bin");
	
	// Retrive the drive struct ref
    usbdvd_mountdisc(test);
	usbdvd_struct *usbdvdctx = usbdvd_get_ctx(test);
	
	char openpath[2048];
	char basepath[2048];
	memset(openpath,0,sizeof(openpath));
	if(usbdvdctx->fs.mounted)sprintf(openpath,"%s/",usbdvdctx->fs.mountpoint);
	if(usbdvdctx->fs.mounted)sprintf(basepath,"%s/",usbdvdctx->fs.mountpoint);
	print_drive_info(test,openpath);
	
	while(appletMainLoop()){
		 padUpdate(&pad);

        // padGetButtonsDown returns the set of buttons that have been newly pressed in this frame compared to the previous one
        u64 kDown = padGetButtonsDown(&pad);

        if (kDown & HidNpadButton_Plus) break; // break in order to return to hbmenu

		if (kDown & HidNpadButton_Y) {
			svcSleepThread(10000000ULL);
			if(usbdvdctx->drive.drive_found){
				usbdvd_eject(test);
				memset(openpath,0,sizeof(openpath));
				print_drive_info(test,openpath);
			}
		}
		
		if (kDown & HidNpadButton_X) {
			svcSleepThread(10000000ULL);
			if(usbdvdctx->drive.drive_found){
				usbdvd_mountdisc(test);
				memset(openpath,0,sizeof(openpath));
				if(usbdvdctx->fs.mounted)sprintf(openpath,"%s/",usbdvdctx->fs.mountpoint);
				if(usbdvdctx->fs.mounted)sprintf(basepath,"%s/",usbdvdctx->fs.mountpoint);
				print_drive_info(test,openpath);
			}
		}
		
		if (kDown & HidNpadButton_A) {
			svcSleepThread(10000000ULL);
			if(!currentry.isdir)continue;
			//memset(openpath,0,sizeof(openpath));
			if(strcmp(openpath,basepath) == 0){
				if(usbdvdctx->fs.mounted)sprintf(openpath,"%s%s",openpath,currentry.currname);
			}else{
				if(usbdvdctx->fs.mounted)sprintf(openpath,"%s/%s",openpath,currentry.currname);
			}
			
			cursor_idx=0;
			print_drive_info(test,openpath);
		}
		
		if (kDown & HidNpadButton_B) {
			cursor_idx=0;
			if(strcmp(openpath,basepath) == 0){
				
			}else{
				char *backdir = get_directory(openpath);
				memset(openpath,0,sizeof(openpath));
				sprintf(openpath,"%s",backdir);
				free(backdir);
				
			}
			if(strcmp(openpath,usbdvdctx->fs.mountpoint) == 0){
				sprintf(openpath,"%s/",usbdvdctx->fs.mountpoint);
			}
			print_drive_info(test,openpath);
		}
		
		if (kDown & HidNpadButton_Up) {
			svcSleepThread(10000000ULL);
			if(cursor_idx==0){
				cursor_idx = currlist_len-1;
			}else{
				cursor_idx-=1;
			}
			
			print_drive_info(test,openpath);
		}
		
		if (kDown & HidNpadButton_Down) {
			svcSleepThread(10000000ULL);
			cursor_idx+=1;
			if(cursor_idx>=currlist_len)cursor_idx=0;
			print_drive_info(test,openpath);
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