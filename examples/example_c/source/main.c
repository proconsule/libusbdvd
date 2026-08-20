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

int sel_subclass = 0;

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
        printf(CONSOLE_ESC(4;2H)CONSOLE_ESC(0m)"drive subclass:%s 0x%02hhx\r\n"CONSOLE_ESC(0m),CONSOLE_ESC(1m),usbdvdctx->drive.subclass);
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
			
			
			
			int list_header_row = (usbdvdctx->dvd_protection.CSS || usbdvdctx->bluray_protection.AACS) ? 19 : 17;
			int list_status_row = list_header_row + 1;
			int list_start_row = list_header_row + 2;
			int list_end_row = 38; 
			int visible_rows = list_end_row - list_start_row + 1;
			if(visible_rows < 1) visible_rows = 1;

			uint32_t scroll_offset = 0;
			if(cursor_idx >= (uint32_t)visible_rows){
				scroll_offset = cursor_idx - (uint32_t)visible_rows + 1;
			}

			for(int r = list_status_row; r <= list_end_row; r++){
				printf(CONSOLE_ESC(%d;1H)CONSOLE_ESC(2K),r);
			}

			DIR *dir;
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
					
					bool is_selected = (fidx==cursor_idx);
					bool is_visible = (fidx >= scroll_offset && fidx < scroll_offset + (uint32_t)visible_rows);

					if(is_selected){
						memset(currentry.currname,0,sizeof(currentry.currname));
						sprintf(currentry.currname,"%s",dir->fileData.d_name);
						currentry.isdir = st.st_mode == S_IFDIR;
					}

					if(is_visible){
						char size_str[50];
						formatBytes(st.st_size,size_str,sizeof(size_str));
						
						char date_str[20];
						unixToDate(st.st_mtime, date_str, sizeof(date_str));

						char name_display[40];
						if(strlen(dir->fileData.d_name) >= sizeof(name_display)){
							snprintf(name_display,sizeof(name_display),"%.*s..",(int)sizeof(name_display)-3,dir->fileData.d_name);
						}else{
							snprintf(name_display,sizeof(name_display),"%s",dir->fileData.d_name);
						}

						int row = list_start_row + (int)(fidx - scroll_offset);
						printf(CONSOLE_ESC(%d;1H)"%s%-2s %-6s %-39s %s %10s"CONSOLE_ESC(0m),
						       row,
						       is_selected ? CONSOLE_ESC(1m) : CONSOLE_ESC(0m),
						       is_selected ? "->" : "  ",
						       st.st_mode == S_IFDIR ? "<dir>" : "<file>",
						       name_display,
						       date_str,
						       size_str);
					}
					
					fidx++;
				}
						
				closedir(dir);
				currlist_len = fidx;

				if(currlist_len > 0){
					uint32_t last_shown = scroll_offset + (uint32_t)visible_rows;
					if(last_shown > currlist_len) last_shown = currlist_len;
					printf(CONSOLE_ESC(%d;1H)"File %u-%u di %u%s%s",
					       list_status_row,
					       scroll_offset+1, last_shown, currlist_len,
					       scroll_offset > 0 ? "  (^ more above)" : "",
					       last_shown < currlist_len ? "  (v more bottom)" : "");
				}
			}
			
		}
		if(usbdvdctx->fs.mounted && strcmp(_path,"") != 0){
			printf(CONSOLE_ESC(40;5H)CONSOLE_ESC(1m)"Y"CONSOLE_ESC(0m)": Eject Drive "CONSOLE_ESC(1m)"X"CONSOLE_ESC(0m)": Mount Drive "CONSOLE_ESC(1m)"UP/Down"CONSOLE_ESC(0m)": Navigation "CONSOLE_ESC(1m)"A"CONSOLE_ESC(0m)": Select "CONSOLE_ESC(1m)"B"CONSOLE_ESC(0m)": Back\n");
			if(!currentry.isdir && strlen(currentry.currname) > 0){
				printf(CONSOLE_ESC(41;5H)CONSOLE_ESC(1m)"MINUS"CONSOLE_ESC(0m)": Speed Test su on selected file\n");
			}
		}else{
			
			printf(CONSOLE_ESC(40;25H)""CONSOLE_ESC(1m)"Y"CONSOLE_ESC(0m)": Eject Drive "CONSOLE_ESC(1m)"X"CONSOLE_ESC(0m)": Mount Drive\n");
		}
	}else{
        printf(CONSOLE_ESC(5;25H)CONSOLE_ESC(0m)"NO USB DVD/BD DRIVE FOUND"CONSOLE_ESC(0m));
		
        printf(CONSOLE_ESC(6;2H)CONSOLE_ESC(1m)"No dvice found with both 0x02 and 0x06 subclass unfiltered"CONSOLE_ESC(0m));
         
    }
	
	
}


char* get_directory(const char *path) {
    char *path_copy = strdup(path);  
    char *last_slash = strrchr(path_copy, '/');
    
    if (last_slash != NULL) {
        *(last_slash) = '\0';  
    }
    
    return path_copy;  
}

#define SPEEDTEST_SAMPLE_BYTES   (128ULL*1024*1024) 
#define SPEEDTEST_MAX_SECONDS    20.0
#define SPEEDTEST_SMALL_BUF      (128*1024)
#define SPEEDTEST_LARGE_BUF      (4*1024*1024)

typedef struct{
	char filename[2048];
	bool ran;
	bool small_buf_ok;
	bool large_buf_ok;
	uint64_t small_buf_bytes;
	uint64_t large_buf_bytes;
	double small_buf_secs;
	double large_buf_secs;
	double small_buf_mbps;
	double large_buf_mbps;
}speedtest_result_struct;

speedtest_result_struct speedtest_result;

bool speedtest_run_pass(const char *fullpath, size_t bufsize, int progress_row, const char *label,
		uint64_t *out_bytes, double *out_secs){
	FILE *f = fopen(fullpath,"rb");
	if(!f) return false;

	uint8_t *buf = (uint8_t *)malloc(bufsize);
	if(!buf){
		fclose(f);
		return false;
	}

	uint64_t total = 0;
	u64 start_tick = armGetSystemTick();
	u64 last_print_tick = start_tick;

	while(total < SPEEDTEST_SAMPLE_BYTES){
		size_t r = fread(buf,1,bufsize,f);
		if(r == 0) break;
			total += r;

		u64 now_tick = armGetSystemTick();
		double elapsed = armTicksToNs(now_tick - start_tick) / 1e9;

		if(armTicksToNs(now_tick - last_print_tick) / 1e9 >= 0.2){
			double cur_mbps = elapsed > 0 ? (total/1024.0/1024.0)/elapsed : 0;
			printf(CONSOLE_ESC(2K)CONSOLE_ESC(%d;2H)"%s: %llu MB letti, %.2f MB/s..."CONSOLE_ESC(0m),
				progress_row,label,(unsigned long long)(total/1024/1024),cur_mbps);
			consoleUpdate(NULL);
			last_print_tick = now_tick;
		}
		if(elapsed >= SPEEDTEST_MAX_SECONDS) break; // cap di sicurezza
	}

	u64 end_tick = armGetSystemTick();
	free(buf);
	fclose(f);

	*out_bytes = total;
	*out_secs = armTicksToNs(end_tick - start_tick) / 1e9;
	return total > 0;
}

void run_speed_test(const char *fullpath){
	memset(&speedtest_result,0,sizeof(speedtest_result));
	snprintf(speedtest_result.filename,sizeof(speedtest_result.filename),"%s",fullpath);

	printf( CONSOLE_ESC(2J) );
	printf(CONSOLE_ESC(3;2H)CONSOLE_ESC(1m)"SPEED TEST"CONSOLE_ESC(0m));
	printf(CONSOLE_ESC(5;2H)"File: %s",fullpath);
	consoleUpdate(NULL);

	speedtest_result.small_buf_ok = speedtest_run_pass(fullpath,SPEEDTEST_SMALL_BUF,8,
					"small buffer (128KB)",
					&speedtest_result.small_buf_bytes,
					&speedtest_result.small_buf_secs);
	if(speedtest_result.small_buf_ok && speedtest_result.small_buf_secs > 0){
		speedtest_result.small_buf_mbps = (speedtest_result.small_buf_bytes/1024.0/1024.0) / speedtest_result.small_buf_secs;
	}

	speedtest_result.large_buf_ok = speedtest_run_pass(fullpath,SPEEDTEST_LARGE_BUF,9,"large buffer (4MB)   ",
			&speedtest_result.large_buf_bytes,
			&speedtest_result.large_buf_secs);
	if(speedtest_result.large_buf_ok && speedtest_result.large_buf_secs > 0){
		speedtest_result.large_buf_mbps = (speedtest_result.large_buf_bytes/1024.0/1024.0) / speedtest_result.large_buf_secs;
	}
	
	speedtest_result.ran = true;
}

void print_speed_test_result(usbdvd_obj* test){
	printf( CONSOLE_ESC(2J) );
	printf(CONSOLE_ESC(3;2H)CONSOLE_ESC(1m)"SPEED TEST - RESULTS"CONSOLE_ESC(0m));
	printf(CONSOLE_ESC(5;2H)"File: %s",speedtest_result.filename);

	if(speedtest_result.small_buf_ok){
	printf(CONSOLE_ESC(7;2H)"small buffer (128KB): "CONSOLE_ESC(1m)"%.2f MB/s"CONSOLE_ESC(0m)"  (%llu MB in %.2fs)",
		speedtest_result.small_buf_mbps,
		(unsigned long long)(speedtest_result.small_buf_bytes/1024/1024),
		speedtest_result.small_buf_secs);
	}else{
		printf(CONSOLE_ESC(7;2H)"small buffer (128KB): "CONSOLE_ESC(1m)"error reading"CONSOLE_ESC(0m));
	}

	if(speedtest_result.large_buf_ok){
		printf(CONSOLE_ESC(8;2H)"large buffer (4MB):    "CONSOLE_ESC(1m)"%.2f MB/s"CONSOLE_ESC(0m)"  (%llu MB in %.2fs)",
		speedtest_result.large_buf_mbps,
			(unsigned long long)(speedtest_result.large_buf_bytes/1024/1024),
			speedtest_result.large_buf_secs);
	}else{
		printf(CONSOLE_ESC(8;2H)"large buffer (4MB):    "CONSOLE_ESC(1m)"error reading"CONSOLE_ESC(0m));
	}

	if(speedtest_result.small_buf_ok && speedtest_result.large_buf_ok && speedtest_result.small_buf_mbps > 0){
		printf(CONSOLE_ESC(10;2H)"ratio large buffer / small buffer: "CONSOLE_ESC(1m)"%.2fx"CONSOLE_ESC(0m),
		speedtest_result.large_buf_mbps / speedtest_result.small_buf_mbps);
	}

	{
		const char *support_labels[] = {"not tested","SUPPORTED","not supported"};
		int ra = usbdvd_get_read_ahead_support(test);
		int sm = usbdvd_get_streaming_mode_support(test);
		if(ra < 0 || ra > 2) ra = 0;
		if(sm < 0 || sm > 2) sm = 0;
		printf(CONSOLE_ESC(12;2H)"SET READ AHEAD (0xA7): "CONSOLE_ESC(1m)"%s"CONSOLE_ESC(0m),support_labels[ra]);
		printf(CONSOLE_ESC(13;2H)"SET STREAMING  (0xB6): "CONSOLE_ESC(1m)"%s"CONSOLE_ESC(0m),support_labels[sm]);
	}

	printf(CONSOLE_ESC(40;2H)CONSOLE_ESC(1m)"B"CONSOLE_ESC(0m)": Back to browser");
	consoleUpdate(NULL);
}

bool init_end = false;


int main(int argc, const char* const* argv) {
	
	appletLockExit();
	romfsInit();
	
    //socketInitializeDefault();
    //nxlinkStdio();
    
	consoleInit(NULL);
	padConfigureInput(1, HidNpadStyleSet_NpadStandard);
	
	PadState pad;
    padInitializeDefault(&pad);
	
	// Library init (it will find first compatible drive and mount proper fs)
    // 0 means only 0x02 subclass 1 means 0x02 and 0x06 
	usbdvd_obj* test = usbdvd_init_verbose(1);
    // Retrive the drive struct ref
    usbdvd_struct *usbdvdctx = usbdvd_get_ctx(test);
	
	// For ISO file mount use
	// usbdvd_obj* test = usbdvd_initimage("/pathtoiso.iso");
	// For CUE/BIN mount use
	//usbdvd_obj* test = usbdvd_initcuebin("/pathtofile.cue","/pathtofile.bin");
	
	
    
	char openpath[2048];
	char basepath[2048];
	memset(openpath,0,sizeof(openpath));
    memset(basepath,0,sizeof(basepath));
	bool viewing_speedtest = false;
    
    
    /*
    if(usbdvdctx->drive.drive_found){
        usbdvd_mountdisc(test);
        
        memset(openpath,0,sizeof(openpath));
        if(usbdvdctx->fs.mounted)sprintf(openpath,"%s/",usbdvdctx->fs.mountpoint);
        if(usbdvdctx->fs.mounted)sprintf(basepath,"%s/",usbdvdctx->fs.mountpoint);
    
    }
    */
    
    printf("PRESS B BUTTON TO CONTINUE\n");
    
    //print_drive_info(test,openpath);
	
    
    
	while(appletMainLoop()){
		 padUpdate(&pad);

        // padGetButtonsDown returns the set of buttons that have been newly pressed in this frame compared to the previous one
        u64 kDown = padGetButtonsDown(&pad);

        if (kDown & HidNpadButton_Plus) break; // break in order to return to hbmenu

		if (kDown & HidNpadButton_Y) {
            if(init_end && !viewing_speedtest){
                svcSleepThread(10000000ULL);
                if(usbdvdctx->drive.drive_found){
                    usbdvd_eject(test);
                    memset(openpath,0,sizeof(openpath));
                    print_drive_info(test,openpath);
                }
            }
		}
		
		if (kDown & HidNpadButton_X) {
            if(init_end && !viewing_speedtest){
                svcSleepThread(10000000ULL);
                if(usbdvdctx->drive.drive_found){
                    usbdvd_mountdisc(test);
                    memset(openpath,0,sizeof(openpath));
                    if(usbdvdctx->fs.mounted)sprintf(openpath,"%s/",usbdvdctx->fs.mountpoint);
                    if(usbdvdctx->fs.mounted)sprintf(basepath,"%s/",usbdvdctx->fs.mountpoint);
                    print_drive_info(test,openpath);
                }
            }
		}
		
		if (kDown & HidNpadButton_A) {
			if(init_end && !viewing_speedtest){
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
		}
		
		if (kDown & HidNpadButton_B) {
            if(init_end==false)init_end=true;
			if(viewing_speedtest){
				viewing_speedtest = false;
				print_drive_info(test,openpath);
				consoleUpdate(NULL);
				continue;
			}
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
            if(init_end && !viewing_speedtest){
                svcSleepThread(10000000ULL);
                if(cursor_idx==0){
                    cursor_idx = currlist_len-1;
                }else{
                    cursor_idx-=1;
                }
                
                print_drive_info(test,openpath);
            }
		}
		
		if (kDown & HidNpadButton_Down) {
			if(!viewing_speedtest){
				svcSleepThread(10000000ULL);
				cursor_idx+=1;
				if(cursor_idx>=currlist_len)cursor_idx=0;
				print_drive_info(test,openpath);
			}
		}
        
        if (kDown & HidNpadButton_Minus) {
			if(init_end && !viewing_speedtest){
				if(usbdvdctx->fs.mounted && strlen(openpath) > 0 && !currentry.isdir && strlen(currentry.currname) > 0){
					char fullpath[4096];
					if(strcmp(openpath,basepath) == 0){
						snprintf(fullpath,sizeof(fullpath),"%s%s",openpath,currentry.currname);
					}else{
						snprintf(fullpath,sizeof(fullpath),"%s/%s",openpath,currentry.currname);
					}
					run_speed_test(fullpath);
					print_speed_test_result(test);
					viewing_speedtest = true;
				}
			}
		}
	
        consoleUpdate(NULL);
		
		
	}
	
	
	// Destory and cleanup the library
    if(test){
        usbdvd_destroy(test);
	}
    consoleExit(NULL);
	romfsExit();
	appletUnlockExit();
	//socketExit();
	
	return 0;
}