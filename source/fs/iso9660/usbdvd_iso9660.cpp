#include "usbdvd_iso9660.h"
#include "usbdvd_utils.h"

#define DATA_SECOTR_SIZE 2048

#include <cstring>
#include <iostream>
#include <iomanip>
#include <algorithm>

typedef struct {
    uint8_t years_since_1900;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    int8_t timezone_offset;
}rd_record_time_struct;

typedef struct{
    uint8_t rd_len;
    uint8_t rd_ext;
    struct{
        uint8_t root_lba_le[4];
        uint8_t root_lba_be[4];
    };
    struct{
        uint8_t root_size_le[4];
        uint8_t root_size_be[4];
    };
    rd_record_time_struct datetime;
    uint8_t rd_type;
    uint8_t funitsize;
    uint8_t interleave;
    struct{
        uint8_t volume_seq_le[2];
        uint8_t volume_seq_be[2];
    };
    uint8_t name_length;
    
    
}__attribute__((__packed__)) rd_record_struct;

typedef struct{
    uint8_t escapeseq_unused[8];
    uint8_t escapeseq_type[3];
    uint8_t escapeseq_unused2[21];
    
}__attribute__((__packed__)) joliet_escapeseq_struct;

typedef struct{
    uint8_t vdtype;
    uint8_t stdid[5];
    uint8_t vd_ver;
    uint8_t unused1;
    uint8_t sys_id[32];
    uint8_t vol_id[32];
    uint8_t unused2[8];
    struct{
        uint8_t vd_space_le[4];
        uint8_t vd_space_be[4];
    };
    struct{
        uint8_t joliet_escapeseq_type[3];
        uint8_t joliet_escapeseq_unused[29];
    };
    uint8_t vdset_size[4];
    uint8_t vdseq_num[4];
    struct{
        uint8_t lbasize_le[2];
        uint8_t lbasize_be[2];
    };
    struct{
        uint8_t pathtable_size_le[4];
        uint8_t pathtable_size_be[4];
    };
    uint8_t lpath_table_lba[4];
    uint8_t lpath_opt_table_lba[4];
    uint8_t mpath_table_lba[4];
    uint8_t mpath_opt_table_lba[4];
    uint8_t rd_record[34];
    
    
    
}__attribute__((__packed__)) primary_vd_struct;



typedef struct{
    uint8_t sp_id[2];
    uint8_t len;
    uint8_t ver;
    
}rockridge_header_struct;


typedef struct{
    uint8_t px : 1;
    uint8_t pn : 1;
    uint8_t sl : 1;
    uint8_t nm : 1;
    uint8_t cl : 1;
    uint8_t pl : 1;
    uint8_t re : 1;
    uint8_t tf : 1;
}__attribute__((__packed__)) rockridge_rr_flags_struct;



std::string cleanFileName(const uint8_t* name, uint8_t length) {
	std::string result;
	for (int i = 0; i < length; i++) {
		if (name[i] == 0) break;
		if (name[i] == 1) {
			result = ".";
			break;
		}
		if (name[i] == 2) {
			result = "..";
			break;
		}
		result += static_cast<char>(name[i]);
	}
        
	size_t semicolon = result.find(';');
	if (semicolon != std::string::npos) {
		result = result.substr(0, semicolon);
	}
        
	return result;
}
	
std::string cleanFileName_Joliet(const uint8_t* _name, uint8_t length) {
        
	uint8_t name[length];
	int z=0;
	for(int i=1;i<length;i=i+2){
		name[z] = _name[i];
		z++;
	}
 
	std::string result;
	for (int i = 0; i < length/2; i++) {
		if (name[i] == 0) break;  
		if (name[i] == 1) {      
			result = ".";
			break;
		}
		if (name[i] == 2) {       
			result = "..";
			break;
		}
		result += static_cast<char>(name[i]);
	}
        
	size_t semicolon = result.find(';');
	if (semicolon != std::string::npos) {
		result = result.substr(0, semicolon);
	}
        
	return result;
}


time_t rd_record_to_unixtime(rd_record_time_struct * _date){
    
    struct tm timeinfo = {};
        timeinfo.tm_year = _date->years_since_1900;
        timeinfo.tm_mon = _date->month-1;
        timeinfo.tm_mday = _date->day;
        timeinfo.tm_hour = _date->hour;
        timeinfo.tm_min = _date->minute;
        timeinfo.tm_sec = _date->second;
        timeinfo.tm_isdst = -1;
        
        time_t local_time = mktime(&timeinfo);
        int timezone_seconds = _date->timezone_offset * 15 * 60;
        time_t utc_time = local_time - timezone_seconds;
    
    return utc_time;
}


CUSBDVD_ISO9660FS::CUSBDVD_ISO9660FS(CUSBSCSI * _usb_scsi_ctx,uint32_t _startlba,uint32_t _endlba) : CUSBDVD_DATADISC(_usb_scsi_ctx,_startlba,_endlba){
	
	uint8_t iso9660_rootsector[DATA_SECOTR_SIZE];
    
    ReadSector(16,iso9660_rootsector);
	primary_vd_struct primary_vd = {0};
    memcpy(&primary_vd,iso9660_rootsector,sizeof(primary_vd));
 
    
		SystemIdentifier = (const char *)primary_vd.sys_id;
		VolumeIdentifier = (const char *)primary_vd.vol_id;
		VolumeSpace = byte2u32_le(primary_vd.vd_space_le)*DATA_SECOTR_SIZE;
		VolumeSectors =  byte2u32_le(primary_vd.vd_space_le);
		rd_record_struct* root_record = reinterpret_cast<rd_record_struct*>(&primary_vd.rd_record);
		uint32_t root_sector = byte2u32_le(root_record->root_lba_le);
		
		uint8_t iso9660_rootjolietsector[DATA_SECOTR_SIZE];
	   
		ReadSector(17,iso9660_rootjolietsector);
		primary_vd_struct testjoilet = {0};
		memcpy(&testjoilet,iso9660_rootjolietsector,sizeof(testjoilet));
		
		if(testjoilet.vdtype == 0x02){
			isjoliet = true;
			if(testjoilet.joliet_escapeseq_type[0] == 0x25 && testjoilet.joliet_escapeseq_type[1] == 0x2f && testjoilet.joliet_escapeseq_type[2] == 0x40){
				jolietver = 1;
				
			}
			if(testjoilet.joliet_escapeseq_type[0] == 0x25 && testjoilet.joliet_escapeseq_type[1] == 0x2f && testjoilet.joliet_escapeseq_type[2] == 0x43){
				jolietver = 2;
				
			}
			if(testjoilet.joliet_escapeseq_type[0] == 0x25 && testjoilet.joliet_escapeseq_type[1] == 0x2f && testjoilet.joliet_escapeseq_type[2] == 0x45){
				jolietver = 3;
			}
			
		}

	   if(isjoliet){
			rd_record_struct* root_record = reinterpret_cast<rd_record_struct*>(&testjoilet.rd_record);
			uint32_t root_sector = byte2u32_le(root_record->root_lba_le);
			
		   list_dir_joliet(root_sector,"/");
	   }else{
			list_dir_iso9660(root_sector,"/");
		}
		
		for(int i=0;i<(int)disc_dirlist.size();i++){
			usbdvd_log("%s\r\n",disc_dirlist[i].name.c_str());
		}
	
}

CUSBDVD_ISO9660FS::CUSBDVD_ISO9660FS(std::string _filename) : CUSBDVD_DATADISC(_filename){
    
    uint8_t iso9660_rootsector[DATA_SECOTR_SIZE];
    
    ReadSector(16,iso9660_rootsector);
	primary_vd_struct primary_vd = {0};
    memcpy(&primary_vd,iso9660_rootsector,sizeof(primary_vd));
    
    
		SystemIdentifier = (const char *)primary_vd.sys_id;
		VolumeIdentifier = (const char *)primary_vd.vol_id;
		VolumeSpace = byte2u32_le(primary_vd.vd_space_le)*DATA_SECOTR_SIZE;
		VolumeSectors =  byte2u32_le(primary_vd.vd_space_le);
		rd_record_struct* root_record = reinterpret_cast<rd_record_struct*>(&primary_vd.rd_record);
		uint32_t root_sector = byte2u32_le(root_record->root_lba_le);
		
		uint8_t iso9660_rootjolietsector[DATA_SECOTR_SIZE];
	   
		ReadSector(17,iso9660_rootjolietsector);
		primary_vd_struct testjoilet = {0};
		memcpy(&testjoilet,iso9660_rootjolietsector,sizeof(testjoilet));
		
		if(testjoilet.vdtype == 0x02){
			isjoliet = true;
			if(testjoilet.joliet_escapeseq_type[0] == 0x25 && testjoilet.joliet_escapeseq_type[1] == 0x2f && testjoilet.joliet_escapeseq_type[2] == 0x40){
				jolietver = 1;
				
			}
			if(testjoilet.joliet_escapeseq_type[0] == 0x25 && testjoilet.joliet_escapeseq_type[1] == 0x2f && testjoilet.joliet_escapeseq_type[2] == 0x43){
				jolietver = 2;
				
			}
			if(testjoilet.joliet_escapeseq_type[0] == 0x25 && testjoilet.joliet_escapeseq_type[1] == 0x2f && testjoilet.joliet_escapeseq_type[2] == 0x45){
				jolietver = 3;
			}
			
		}

	   if(isjoliet){
			rd_record_struct* root_record = reinterpret_cast<rd_record_struct*>(&testjoilet.rd_record);
			uint32_t root_sector = byte2u32_le(root_record->root_lba_le);
			
		   list_dir_joliet(root_sector,"/");
	   }else{
			list_dir_iso9660(root_sector,"/");
		}
		
		for(int i=0;i<(int)disc_dirlist.size();i++){
			usbdvd_log("%s\r\n",disc_dirlist[i].name.c_str());
		}
	
}


void CUSBDVD_ISO9660FS::list_dir_iso9660(uint32_t sector, const std::string& path = "/"){
    uint8_t buffer[2048];
    ReadSector(sector,buffer);
    size_t offset = 0;
    while (offset < 2048) {
        rd_record_struct* record = reinterpret_cast<rd_record_struct*>(buffer+offset);
         if (record->rd_len == 0) {
                break;
            }
        const uint8_t* name_ptr = &buffer[offset + sizeof(rd_record_struct)];
        std::string filename = cleanFileName(name_ptr, record->name_length);
		
        if (filename != "." && filename != "..") {
                std::string full_path = path;
                if (path != "/") full_path += "/";
                full_path += filename;
                
                bool is_directory = (record->rd_type & 0x02) != 0;
                
                
                int mypadding = 0;
                if(record->name_length% 2 == 0)mypadding=mypadding+1;
				
                const uint8_t* rr_ptr = name_ptr+record->name_length+mypadding;
                rockridge_header_struct test_rr;
                memcpy(&test_rr,rr_ptr,sizeof(test_rr));
                if(test_rr.sp_id[0] == 'R' && test_rr.sp_id[1] == 'R'){
					isrockridge = true;
                    rockridge_rr_flags_struct rockridge_rr_flags;
                    memcpy(&rockridge_rr_flags,&rr_ptr[5],1);
                    if(rockridge_rr_flags.nm){
                        
                        const uint8_t* nm_ptr = rr_ptr+test_rr.len;
                        rockridge_header_struct test_nm;
                        memcpy(&test_nm,nm_ptr,4);
                        uint8_t testname[test_nm.len-4];
                        memset(testname,0,test_nm.len-4);
                        memcpy(testname,&nm_ptr[5],test_nm.len-5);
                        filename = (const char *)testname;
                        std::string full_path_rr = path;
                        if (path != "/") full_path_rr += "/";
                        full_path_rr+=filename;
                        disc_dirlist_struct tmp;
                        
                        tmp.name = (const char *)testname;
                        tmp.size =  byte2u32_le(record->root_size_le);
                        tmp.lba = byte2u32_le(record->root_lba_le);
                        tmp.fullpath = full_path_rr;
                        tmp.isdir = is_directory;
                        if(tmp.isdir && filename == ""){
                           
                        }else{
                            disc_dirlist.push_back(tmp);
                        }
                        
                        if (is_directory && filename != "" && byte2u32_le(record->root_lba_le) > 0) {
							list_dir_iso9660(byte2u32_le(record->root_lba_le), full_path_rr);
						}
                        
                    }
                }else{
                    disc_dirlist_struct tmp;
                    tmp.name = filename;
                    tmp.size =  byte2u32_le(record->root_size_le);
                    tmp.lba = byte2u32_le(record->root_lba_le);
                    tmp.fullpath = full_path;
                    tmp.isdir = is_directory;
                    tmp.time = rd_record_to_unixtime(&record->datetime);
                    if(tmp.isdir && filename == ""){
                        
                    }else{
                        disc_dirlist.push_back(tmp);
                    }
                    
                    if (is_directory && filename != "" && byte2u32_le(record->root_lba_le) > 0) {
                        list_dir_iso9660(byte2u32_le(record->root_lba_le), full_path);
                    }
                    
                }
                
            }
            
            offset += record->rd_len;
            if (offset % 2 != 0) offset++;
            
    }
    
}

void CUSBDVD_ISO9660FS::list_dir_joliet(uint32_t sector, const std::string& path = "/"){
    uint8_t buffer[2048];
    ReadSector(sector,buffer);
    size_t offset = 0;
	while (offset < 2048) {
        rd_record_struct* record = reinterpret_cast<rd_record_struct*>(buffer+offset);
         if (record->rd_len == 0) {
               
                break;
            }
        const uint8_t* name_ptr = &buffer[offset + sizeof(rd_record_struct)];
        
        std::string filename = cleanFileName_Joliet(name_ptr, record->name_length);
        if (filename != "." && filename != "..") {
                std::string full_path = path;
                if (path != "/") full_path += "/";
                full_path += filename;
                
                bool is_directory = (record->rd_type & 0x02) != 0;
                disc_dirlist_struct tmp;
                tmp.name = filename;
                tmp.size =  byte2u32_le(record->root_size_le);
                tmp.lba = byte2u32_le(record->root_lba_le);
                tmp.fullpath = full_path;
                tmp.isdir = is_directory;
                tmp.time = rd_record_to_unixtime(&record->datetime);
                if(tmp.isdir && filename == ""){
                   
                }else{
                    disc_dirlist.push_back(tmp);
                }
                
                if (is_directory && filename != "" && byte2u32_le(record->root_lba_le) > 0) {
                    list_dir_joliet(byte2u32_le(record->root_lba_le), full_path);
                }
            }
            
            offset += record->rd_len;
            if (offset % 2 != 0) offset++;
            
    }
    
}
