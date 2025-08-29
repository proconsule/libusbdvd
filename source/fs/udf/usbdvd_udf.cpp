#include "usbdvd_udf.h"
#include "usbdvd_utils.h"

typedef struct {
    uint16_t tag_identifier;        
    uint16_t descriptor_version;    
    uint8_t  tag_checksum;          
    uint8_t  reserved;            
    uint16_t tag_serial_number;  
    uint16_t descriptor_crc;  
    uint16_t descriptor_crc_length; 
    uint32_t tag_location;          
} __attribute__((packed)) UDF_Tag;


typedef struct {
    uint32_t length;               
    uint32_t location;             
} __attribute__((packed)) UDF_ExtentAd;


typedef struct {
    UDF_Tag tag;                   
    UDF_ExtentAd main_vds_extent; 
    UDF_ExtentAd reserve_vds_extent;
    uint8_t reserved[480];     
} __attribute__((packed)) UDF_AnchorVolumeDescriptorPointer;


// Long Allocation Descriptor
typedef struct {
    uint32_t length;  
    uint32_t location;   
    uint16_t partition_reference;  
    uint8_t  reserved[6];     
} __attribute__((packed)) UDF_LongAd;

// Timestamp UDF
typedef struct {
    uint16_t type_and_timezone;    
    uint16_t year;           
    uint8_t  month;          
    uint8_t  day;           
    uint8_t  hour;            
    uint8_t  minute;        
    uint8_t  second;            
    uint8_t  centiseconds;       
    uint8_t  hundreds_microseconds; 
    uint8_t  microseconds;       
} __attribute__((packed)) UDF_Timestamp;

typedef struct {
    UDF_Tag tag;                    
    uint32_t vds_number;          
    uint32_t primary_vd_number; 
    uint8_t  volume_identifier[32];
    uint16_t volume_sequence_number;
    uint16_t maximum_volume_sequence_number; 
    uint16_t min_udf_version;    
    uint16_t max_udf_version;   
    uint32_t character_set_list;   
    uint32_t maximum_character_set_list;   
    uint8_t  volume_set_identifier[128];  
    uint8_t  descriptor_character_set[64];  
    uint8_t  explanatory_character_set[64]; 
    UDF_ExtentAd volume_abstract;   
    UDF_ExtentAd volume_copyright_notice;  
    uint8_t  application_identifier[32];  
    UDF_Timestamp recording_date_time;       
    uint8_t  implementation_identifier[32];  
    uint8_t  implementation_use[64]; 
    uint32_t predecessor_vds_location;      
    uint16_t flags;              
    uint8_t  reserved[22];         
} __attribute__((packed)) UDF_PrimaryVolumeDescriptor;

typedef struct {
    uint8_t  charset_type;
    uint8_t  charset_info[63];
} charspec_t;

typedef struct {
    uint8_t len;       
    uint8_t cid;     
    uint8_t str[30];   
} __attribute__((packed)) d_string32_t;

typedef struct {
    uint8_t  flags;
    uint8_t  identifier[23];
    uint8_t  identifier_suffix[8];
} regid_t;


typedef struct {
    uint8_t  partition_map_type;
    uint8_t  partition_map_length;
    
} partition_map_t;


typedef struct{
	uint8_t tag_id[2];
    uint8_t descriptor_version[2];
    uint8_t checksum;
    uint8_t reserved;
    uint8_t tag_serial_number[2];
    uint8_t descriptor_crc[2];
    uint8_t descriptor_crc_length[2];
    uint8_t tag_location[4];
} __attribute__((packed)) udf_file_set_descriptor_t;

typedef struct{
	UDF_ExtentAd main_vol_extent;
	UDF_ExtentAd reserve_vol_extent;
	
}fileset_desc_ptr_struct;

typedef struct {
    UDF_Tag tag;                 
    
    // LVD Content (496 bytes)
    uint32_t volume_descriptor_sequence_number;
    charspec_t descriptor_character_set;
    uint8_t  logical_volume_identifier[128];    
    uint32_t logical_block_size;              
    regid_t  domain_identifier;             
    UDF_LongAd logical_volume_contents_use;  
    uint32_t map_table_length;           
    uint32_t number_of_partition_maps;     
    regid_t  implementation_identifier;
    uint8_t  implementation_use[128];
    UDF_ExtentAd integrity_sequence_extent;
	
} __attribute__((packed)) logical_volume_descriptor_t;

typedef struct {
    UDF_Tag tag;                    
	uint32_t volume_descriptor_sequence_number;
    uint16_t partition_flags;             
    uint16_t partition_number;            
    regid_t  partition_contents;          
    uint8_t  partition_contents_use[128]; 
    uint32_t access_type;                
    uint32_t partition_starting_location;  
    uint32_t partition_length;            
    regid_t  implementation_identifier;    
    uint8_t  implementation_use[128];     
    uint8_t  reserved_area[156];          
} partition_descriptor_t;


typedef struct {
    uint8_t     map_type;              
    uint8_t     map_length;            
    uint8_t     reserved[2];
    regid_t     partition_type_identifier; 
    uint16_t    volume_sequence_number;
    uint16_t    partition_number;
    uint32_t    metadata_file_location; 
    uint32_t    metadata_mirror_location; 
    uint32_t    metadata_bitmap_location; 
    uint32_t    allocation_unit_size;   
    uint16_t    alignment_unit_size;    
    uint8_t     flags;                 
    uint8_t     reserved2[5];
} __attribute__((packed)) metadata_partition_map_t;



typedef struct {
    UDF_Tag tag;                   
	UDF_Timestamp recording_date_and_time;          
    uint16_t interchange_level;
    uint16_t max_interchange_level;
    uint32_t character_set_list;
    uint32_t maximum_file_set_descriptor_length;
    uint32_t file_set_identifier;
	uint32_t udf_fsd_num;

	UDF_LongAd root_directory_icb;                 
    
    regid_t domain_identifier;                     
    UDF_LongAd next_extent;                         
    UDF_LongAd system_stream_directory_icb;        
    uint8_t reserved2[32];                         
} file_set_descriptor_t;


typedef struct 
{
  uint32_t  lba;
  uint16_t  partitionReferenceNum;
} __attribute__((packed)) udf_lb_addr;


typedef struct 
{
  uint32_t  prev_num_dirs;
  uint16_t  strat_type;
  uint16_t  strat_param;
  uint16_t  max_num_entries;
  uint8_t   reserved;
  uint8_t   file_type;
  udf_lb_addr parent_ICB;
  uint16_t  flags;
} __attribute__((packed)) udf_icbtag;

typedef struct 
{
  UDF_Tag tag;                    
  udf_icbtag    icb_tag;    
  uint32_t    uid;             
  uint32_t    gid;              
  uint32_t    permissions;  
  uint16_t    link_count;    
  uint8_t     rec_format;     
  uint8_t     rec_disp_attr;     
  uint32_t    rec_len;         
  uint64_t    info_len;  
  uint64_t    logblks_recorded;  
  UDF_Timestamp access_time;           
  UDF_Timestamp modification_time;
  UDF_Timestamp attribute_time;
  uint32_t    checkpoint;
  UDF_LongAd   ext_attr_ICB;
  regid_t     imp_id;
  uint64_t    unique_ID;
  uint32_t    u_extended_attr;
  uint32_t    u_alloc_descs;
  
} __attribute__((packed)) udf_file_entry;

typedef struct {
    UDF_Tag tag;                     // Tag = 266 for Extended File Entry
    udf_icbtag icb_tag;    
    uint32_t uid;             
    uint32_t gid;              
    uint32_t permissions;  
    uint16_t link_count;    
    uint8_t rec_format;     
    uint8_t rec_disp_attr;     
    uint32_t rec_len;         
    uint64_t info_len;  
    uint64_t object_size;          
    uint64_t logblks_recorded;  
    UDF_Timestamp access_time;           
    UDF_Timestamp modification_time;
    UDF_Timestamp creation_time;     
    UDF_Timestamp attribute_time;
    uint32_t checkpoint;
	uint32_t reserved1;
    UDF_LongAd ext_attr_ICB;
    UDF_LongAd stream_directory_icb; 
    regid_t imp_id;
    uint64_t unique_ID;
    uint32_t u_extended_attr;
    uint32_t u_alloc_descs;
} __attribute__((packed)) udf_extended_file_entry;

#define DATA_SECOTR_SIZE 2048

typedef struct{
	uint8_t len;
	uint8_t cid;
	uint8_t str[126];
}__attribute__((packed)) udf_dstring_128;

typedef struct{
	uint8_t len;
	uint8_t cid;
	uint8_t str[30];
}__attribute__((packed)) udf_dstring;

struct udf_fsd_s
{
  UDF_Tag       tag;
  UDF_Timestamp recording_time;
  uint16_t    interchange_lvl;
  uint16_t    maxInterchange_lvl;
  uint32_t    charset_list;
  uint32_t    max_charset_list;
  uint32_t    fileset_num;
  uint32_t    udf_fsd_num;
  charspec_t  logical_vol_id_charset;
  udf_dstring_128     logical_vol_id;
  charspec_t  fileset_charset;
  udf_dstring     fileSet_id;
  udf_dstring     copyright_file_id;
  udf_dstring     abstract_file_id;
  UDF_LongAd   root_icb;
  regid_t     domain_id;
  UDF_LongAd   next_ext;
  UDF_LongAd   stream_directory_ICB;
  uint8_t     reserved[32];
} GNUC_PACKED;


typedef struct {
    UDF_Tag       tag;
    uint16_t  file_version_num;
	uint8_t   file_characteristics;
	uint8_t  length_of_file_identifier;
    
	UDF_LongAd icb; 
    uint16_t length_of_implementation_use;
    
} __attribute__((packed))  file_identifier_descriptor_t;


std::string UTF16_Truncate(unsigned char * _str,uint32_t _len) {
        
	uint8_t string[_len-1];
	int z=0;
	for(uint32_t i=0;i<_len;i=i+2){
		string[z] = _str[i];
		z++;
	}
 
	std::string result;
	for (uint32_t i = 0; i < _len/2; i++) {
		result += static_cast<char>(string[i]);
	}
    
	return result;
}


std::string dstring_to_string(uint8_t *_dstring,uint8_t _len){
	if(_len==0)return "";
	if (_dstring[0] == 0x08 || _dstring[0] == 0x10) {
		if(_dstring[0] == 0x08){
			return std::string((char *)_dstring+1,_len-2);
		}else if(_dstring[0] == 0x10){
			return UTF16_Truncate(_dstring+2,_len-2);
		}
				
	}else{
		return std::string((char *)_dstring,_len);
	}
	return "";
}


std::string getUDFVersionString(uint16_t descriptorVersion) {
	switch (descriptorVersion) {
        case 0x0102:
            return "UDF 1.02" ;
           
        case 0x0150:
            return "UDF 1.50" ;
           
        case 0x0200:
            return "UDF 2.00" ;
           
        case 0x0201:
            return "UDF 2.01" ;
           
        case 0x0250:
            return "UDF 2.50" ;
;
        case 0x0260:
            return "UDF 2.60" ;
            
        default:
            return "???";
            
    }
}




time_t udf102_to_unix_timestamp(const UDF_Timestamp& udf_ts) {
    struct tm tm_time = {};
    tm_time.tm_year = udf_ts.year - 1900;  // tm_year è anni dal 1900
    tm_time.tm_mon = udf_ts.month - 1;     // tm_mon è 0-11
    tm_time.tm_mday = udf_ts.day;
    tm_time.tm_hour = udf_ts.hour;
    tm_time.tm_min = udf_ts.minute;
    tm_time.tm_sec = udf_ts.second;
    tm_time.tm_isdst = 0;   // Forza no DST per calcolo UTC
	
	time_t unix_time = mktime(&tm_time);
	
    
    return unix_time;
}


std::vector<sector_range_struct> udf_extents_to_sectors(
    std::vector<udf_extent_struct> extents,    
    uint64_t _pos,                 
    uint64_t _len                  
) {
    std::vector<sector_range_struct> sector_ranges;
    
    if (extents.size() == 0 || _len == 0) {
        return sector_ranges;
    }
    
    uint64_t current_logical_pos = 0;  
    uint64_t remaining_to_read = _len; 
    uint64_t current_read_pos = _pos;  
    

    for (size_t i = 0; i < extents.size() && remaining_to_read > 0; ++i) {
        if (extents[i].length == 0) {
            break; // Fine degli extent
        }
        
        uint64_t extent_end = current_logical_pos + extents[i].length;
        
        
        if (current_read_pos < extent_end && current_logical_pos < (_pos + _len)) {
            uint64_t offset_in_extent = 0;
            if (current_read_pos > current_logical_pos) {
                offset_in_extent = current_read_pos - current_logical_pos;
            }
            
            uint64_t bytes_from_this_extent = std::min(
                extents[i].length - offset_in_extent,  
                remaining_to_read                     
            );
            
            if (bytes_from_this_extent > 0) {
                uint64_t physical_byte_start = (extents[i].location * DATA_SECOTR_SIZE);
				if(sector_ranges.size()==0)physical_byte_start = (extents[i].location * DATA_SECOTR_SIZE) + _pos;
                
                uint64_t start_sector = physical_byte_start / DATA_SECOTR_SIZE;
                
                uint64_t physical_byte_end = physical_byte_start + bytes_from_this_extent - 1;
                uint64_t end_sector = physical_byte_end / DATA_SECOTR_SIZE;
                
                uint64_t sectors_to_read = end_sector - start_sector + 1;
                
                sector_ranges.push_back({start_sector, sectors_to_read});
                
                remaining_to_read -= bytes_from_this_extent;
                current_read_pos += bytes_from_this_extent;
            }
        }
        
        current_logical_pos = extent_end;
    }
    
    return sector_ranges;
}

void CUSBDVD_UDFFS::Parse_FileEntry_Ptr(uint8_t * buffer,disc_dirlist_struct * _tmpfile){
	
	
	
	udf_file_entry testentry = {0};
	uint32_t fe_pos = 0;
	memcpy(&testentry,buffer,sizeof(testentry));
	if(testentry.tag.tag_identifier == 261){
	
	
		fe_pos+=sizeof(udf_file_entry);
		_tmpfile->size = testentry.info_len;
		if(testentry.icb_tag.file_type == 0x04)_tmpfile->isdir = true;
		if(testentry.icb_tag.file_type == 0x05)_tmpfile->isdir = false;
		
		_tmpfile->access_time = udf102_to_unix_timestamp(testentry.access_time);
		_tmpfile->modification_time = udf102_to_unix_timestamp(testentry.modification_time);
		_tmpfile->attribute_time = udf102_to_unix_timestamp(testentry.attribute_time);
		
			
		UDF_ExtentAd testdesc = {0};
		memcpy(&testdesc,buffer+fe_pos+testentry.u_extended_attr,testentry.u_alloc_descs);
		_tmpfile->part_lba = testdesc.location;
		_tmpfile->lba = partitionlba+testdesc.location;
		udf_extent_struct tmpextent = {0};
		tmpextent.location =  testdesc.location;
		tmpextent.length =  testdesc.length;
		_tmpfile->extents.push_back(tmpextent);
		
	
	} else if(testentry.tag.tag_identifier == 266) {
		udf_extended_file_entry udf_extfileentry = {0};
		if(udf_extfileentry.icb_tag.file_type == 0x04)_tmpfile->isdir = true;
		if(udf_extfileentry.icb_tag.file_type == 0x05)_tmpfile->isdir = false;
		UDF_ExtentAd testdesc = {0};
		fe_pos+=sizeof(udf_extended_file_entry);
		memcpy(&testdesc,buffer+fe_pos+udf_extfileentry.u_extended_attr,udf_extfileentry.u_alloc_descs);
		_tmpfile->part_lba = testdesc.location;
		_tmpfile->lba = partitionlba+testdesc.location;
		
	}
		
}


void CUSBDVD_UDFFS::Parse_ExtendedFileEntry_Ptr(uint8_t * buffer,disc_dirlist_struct * _tmpfile){
    uint32_t fe_pos = 0;
    udf_extended_file_entry testentry = {0};
    memcpy(&testentry,buffer,sizeof(udf_extended_file_entry));
	if(testentry.tag.tag_identifier != 266)return;
	if(testentry.icb_tag.file_type == 0x04)_tmpfile->isdir = true;
    if(testentry.icb_tag.file_type == 0x05)_tmpfile->isdir = false;
    
	
    fe_pos+=sizeof(udf_extended_file_entry);
	
	uint8_t *rrrr = buffer+fe_pos+testentry.u_extended_attr;
	
	
	uint8_t _alloctype = testentry.icb_tag.flags & 0x0007;
    _tmpfile->size = testentry.info_len;
	
	
    if(_alloctype == 0){
		for(int i=0;i<(int)(testentry.u_alloc_descs/8);i++){
			UDF_ExtentAd testdesc = {0};
			memcpy(&testdesc,rrrr+(i*8),sizeof(UDF_ExtentAd));
			_tmpfile->meta_len = testdesc.length;
			_tmpfile->lba = partitionlba+testdesc.location;
			_tmpfile->part_lba = testdesc.location;
			udf_extent_struct tmpextent = {0};
			tmpextent.location = testdesc.location;
			tmpextent.length = testdesc.length;
			_tmpfile->extents.push_back(tmpextent);
		}
	}
	if(_alloctype == 1){
		for(int i=0;i<(int)(testentry.u_alloc_descs/16);i++){
			UDF_LongAd testdesc = {0};
			memcpy(&testdesc,rrrr+(i*16),sizeof(UDF_LongAd));
			_tmpfile->meta_len = testdesc.length;
			_tmpfile->lba = partitionlba+testdesc.location;
			_tmpfile->part_lba = testdesc.location;
			udf_extent_struct tmpextent = {0};
			tmpextent.location = testdesc.location;
			tmpextent.length = testdesc.length;
			tmpextent.partition_reference = testdesc.partition_reference;
			_tmpfile->extents.push_back(tmpextent);
		}
	}
	
	usbdvd_log("File %s have %lu extents\r\n",_tmpfile->name.c_str(),_tmpfile->extents.size());
	if(_tmpfile->extents.size()>1){
		
		_tmpfile->isdir = false;
		_tmpfile->size = 0;
		for(unsigned int i=0;i<_tmpfile->extents.size();i++){
			_tmpfile->size += _tmpfile->extents[i].length;
			
		}
	}
	_tmpfile->access_time = udf102_to_unix_timestamp(testentry.access_time);
	_tmpfile->modification_time = udf102_to_unix_timestamp(testentry.modification_time);
	_tmpfile->attribute_time = udf102_to_unix_timestamp(testentry.attribute_time);
	
}

void CUSBDVD_UDFFS::Parse_FID_Ptr(uint8_t * buffer,std::string _path,uint32_t _fidsize){
	
	uint32_t fid_pos=0;
	while(true){
		
		if(fid_pos+sizeof(file_identifier_descriptor_t) >= _fidsize)break;
		file_identifier_descriptor_t fid = {0};
		memcpy(&fid,buffer+fid_pos,sizeof(fid));
		if(fid.tag.tag_identifier != 0x0101)break;
		fid_pos+=sizeof(fid);
		fid_pos+=fid.length_of_implementation_use;
		std::string _filename = "";
		disc_dirlist_struct tmpentry;
		if(fid.tag.descriptor_version == 2 && fid.length_of_file_identifier >0){
			_filename = dstring_to_string(buffer+fid_pos,fid.length_of_file_identifier+1);
		}
		if(fid.tag.descriptor_version == 3 && fid.length_of_file_identifier >0){
			_filename = dstring_to_string(buffer+fid_pos,fid.length_of_file_identifier+1);
		}
		
		
		tmpentry.name = _filename;
		if(_path == "/"){
			tmpentry.fullpath = _path + _filename;
		}
		else{
			tmpentry.fullpath = _path + "/" +_filename;
		}
		
		tmpentry.fullpath = trim(tmpentry.fullpath);
		
		
		if(fid.tag.descriptor_version == 2){
			uint8_t fe_buffer[DATA_SECOTR_SIZE];
			ReadSector(partitionlba+fid.icb.location,fe_buffer);
			Parse_FileEntry_Ptr(fe_buffer,&tmpentry);
		} else if(fid.tag.descriptor_version == 3){
			
			uint8_t * fe_buffer = metadata_partition_buffer+(fid.icb.location*DATA_SECOTR_SIZE);
			//uint8_t fe_buffer[fid.icb.length];
			//printf("ENTRY: %u %u %u\r\n",fid.icb.location,fid.icb.length,disc_lvd.meta_partition_location+fid.icb.location);
			//ReadSector(partitionlba+metadata_fsd_offset+fid.icb.location,fe_buffer);
			Parse_ExtendedFileEntry_Ptr(fe_buffer,&tmpentry);
			
		}
		
		if(tmpentry.isdir && tmpentry.name != ""){
			if(fid.tag.descriptor_version == 2){
				uint8_t recursive_buffer[DATA_SECOTR_SIZE];
				ReadSector(tmpentry.lba,recursive_buffer);
				Parse_FID_Ptr(recursive_buffer,tmpentry.fullpath.c_str(),DATA_SECOTR_SIZE);
			}else if(fid.tag.descriptor_version == 3){
				//uint8_t recursive_buffer[tmpentry.meta_len];
				//ReadSectorsLen(tmpentry.lba,tmpentry.meta_len,recursive_buffer);
				uint8_t * recursive_buffer =  metadata_partition_buffer+(tmpentry.extents[0].location*DATA_SECOTR_SIZE);
				
				Parse_FID_Ptr(recursive_buffer,tmpentry.fullpath.c_str(),tmpentry.meta_len);
				
			}
		}
		if(tmpentry.name != ""){
			disc_dirlist.push_back(tmpentry);
		}
		
		fid_pos+=fid.length_of_file_identifier;
		uint32_t fid_padding = 4-((sizeof(fid)+fid.length_of_implementation_use+fid.length_of_file_identifier)%4);
		fid_pos+=fid_padding;
		
	}
	
}

CUSBDVD_UDFFS::CUSBDVD_UDFFS(std::string _filename) 
	: CUSBDVD_DATADISC(_filename){
	
	uint8_t udf_anchorvd[DATA_SECOTR_SIZE];
    
	ReadSector(256,udf_anchorvd);
	UDF_AnchorVolumeDescriptorPointer avdp = {0};
	memcpy(&avdp,udf_anchorvd,sizeof(avdp));
	
	if(avdp.tag.descriptor_version == 2){
		udf15plus = false;
	}else if(avdp.tag.descriptor_version == 3){
		udf15plus = true;
	}
	
	uint8_t udf_pvd[DATA_SECOTR_SIZE];
	uint8_t udf_pvd_2[DATA_SECOTR_SIZE];
	uint8_t udf_partdesc[DATA_SECOTR_SIZE];
	uint8_t udf_lvd[DATA_SECOTR_SIZE];
    
	ReadSector(avdp.main_vds_extent.location,udf_pvd);
	ReadSector(avdp.main_vds_extent.location+1,udf_pvd_2);
	ReadSector(avdp.main_vds_extent.location+2,udf_partdesc);
	ReadSector(avdp.main_vds_extent.location+3,udf_lvd);
	
	
	UDF_PrimaryVolumeDescriptor pvd = {0};
	partition_descriptor_t partdesc = {0};
	memcpy(&pvd,udf_pvd,sizeof(pvd));
	
	
	
	memcpy(&partdesc,udf_partdesc,sizeof(partdesc));
	
	logical_volume_descriptor_t testlvd = {0};
	memcpy(&testlvd,udf_lvd,sizeof(logical_volume_descriptor_t));
	
	uint16_t udfver = (testlvd.domain_identifier.identifier_suffix[1] << 8 ) |  testlvd.domain_identifier.identifier_suffix[0];
	
	
	disc_lvd.udf_version_string = getUDFVersionString(udfver);
	
	
	disc_lvd.VolumeIdentifier = dstring_to_string(testlvd.logical_volume_identifier,128);
	
	disc_lvd.location = testlvd.logical_volume_contents_use.location;
	disc_lvd.length = testlvd.logical_volume_contents_use.length;
	disc_lvd.number_of_partition_maps = testlvd.number_of_partition_maps;
        
	disc_lvd.metadata_partition_size = 0;
	disc_lvd.udfver = udfver;
    
	uint32_t maplen = 0;
	for(uint32_t i=0;i<disc_lvd.number_of_partition_maps;i++){
		uint8_t maptest[2];
		memcpy(maptest,udf_lvd+sizeof(logical_volume_descriptor_t)+maplen,2);
		if(maptest[0] == 0x02){
			metadata_partition_map_t meta_part = {0};
                
			memcpy(&meta_part,udf_lvd+sizeof(logical_volume_descriptor_t)+maplen,sizeof(metadata_partition_map_t));
			disc_lvd.meta_partition_location = meta_part.metadata_file_location;
					
		}
            
            
		maplen+=maptest[1];
	}
        
        
	uint32_t partlbalocation = partdesc.partition_starting_location+testlvd.logical_volume_contents_use.location;
	partitionlba = partlbalocation;
	if(disc_lvd.number_of_partition_maps>1){
		metadata_partition_lba = partdesc.partition_starting_location+testlvd.logical_volume_contents_use.location+disc_lvd.meta_partition_location;
	}
        
	uint8_t udf_first_descriptor[DATA_SECOTR_SIZE];
	if(disc_lvd.number_of_partition_maps>1){
		ReadSector(metadata_partition_lba,udf_first_descriptor);
	}else{
		ReadSector(partlbalocation,udf_first_descriptor);
	}
	
        
	if(udf_first_descriptor[0] == 0x00 && udf_first_descriptor[1] == 0x01){
		udf_fsd_s fsd = {0};
		memcpy(&fsd,udf_first_descriptor,sizeof(fsd));
            
		udf_file_entry root_entry = {0};
		uint8_t root_fileentry[DATA_SECOTR_SIZE];
		ReadSector(partlbalocation+fsd.root_icb.location,root_fileentry);
		memcpy(&root_entry,root_fileentry,sizeof(udf_file_entry));
		UDF_ExtentAd testdesc = {0};
		memcpy(&testdesc,root_fileentry+sizeof(udf_file_entry)+root_entry.u_extended_attr,root_entry.u_alloc_descs);
            
		uint8_t root_fid[DATA_SECOTR_SIZE];
		ReadSector(partlbalocation+testdesc.location,root_fid);
		Parse_FID_Ptr(root_fid,"/",DATA_SECOTR_SIZE);
            
        }
	if(udf_first_descriptor[0] == 0x0a && udf_first_descriptor[1] == 0x01){
			udf_extended_file_entry root_filentry = {0};
            memcpy(&root_filentry,udf_first_descriptor,sizeof(udf_extended_file_entry));
            
			uint8_t _alloctype = root_filentry.icb_tag.flags & 0x0007;
			
			metadata_partition_buffer = (uint8_t *)malloc(root_filentry.info_len*sizeof(uint8_t));
			disc_lvd.metadata_partition_size = root_filentry.info_len;
			std::vector<udf_extent_struct> metadata_extents;
			if(_alloctype == 0x00){
				
				
				for(int i=0;i<(int)(root_filentry.u_alloc_descs/8);i++){
					UDF_ExtentAd firsttestdesc = {0};
					memcpy(&firsttestdesc,udf_first_descriptor+sizeof(udf_extended_file_entry)+root_filentry.u_extended_attr+(i*8),root_filentry.u_alloc_descs);
					udf_extent_struct tmpextent = {0};
					tmpextent.location = firsttestdesc.location;
					tmpextent.length = firsttestdesc.length;
					tmpextent.partition_reference = 0;
					
					metadata_extents.push_back(tmpextent);
					//ReadSectorsLen(partitionlba+firsttestdesc.location,firsttestdesc.length,&metadata_partition_buffer[_buffoff]);
					//_buffoff+=firsttestdesc.length;
					
					
				}
				
			}else if(_alloctype == 0x01){
				UDF_LongAd firsttestdesc = {0};
				memcpy(&firsttestdesc,udf_first_descriptor+sizeof(udf_extended_file_entry)+root_filentry.u_extended_attr,root_filentry.u_alloc_descs);
				udf_extent_struct tmpextent = {0};
				tmpextent.location = firsttestdesc.location;
				tmpextent.length = firsttestdesc.length;
				tmpextent.partition_reference = firsttestdesc.partition_reference;
				metadata_extents.push_back(tmpextent);
			}
			uint32_t _buffoff = 0;
			usbdvd_log("Metadata Partition Extents: %lu\r\n",metadata_extents.size());
			for(unsigned int i=0;i<metadata_extents.size();i++){
				usbdvd_log("Reading Metadata Partition: LBA %u OFF %u SECTORS: %u\r\n",partitionlba,metadata_extents[i].location,metadata_extents[i].length/DATA_SECOTR_SIZE);
				ReadSectorsLen(partitionlba+metadata_extents[i].location,metadata_extents[i].length,&metadata_partition_buffer[_buffoff]);
				_buffoff+=metadata_extents[i].length;	
				
			}
			
			
			if(metadata_partition_buffer==NULL)return;
			
			// DOING SEQUENTIAL READ OF METADATA PARTITION 
			//uint8_t bigbuffer[root_filentry.info_len];
			//metadata_partition_buffer = (uint8_t *)malloc(root_filentry.info_len);
			//ReadSectorsLen(metadata_partition_lba+_firstseclocation,root_filentry.info_len,metadata_partition_buffer);
			
	
            //uint8_t udf_file_set_descriptor[DATA_SECOTR_SIZE];
            //ReadSector(metadata_partition_lba+firsttestdesc.location,udf_file_set_descriptor);
            uint8_t * udf_file_set_descriptor = &metadata_partition_buffer[0];
			
			
			metadata_fsd_offset = metadata_extents[0].location;
            udf_fsd_s fsd = {0};
            memcpy(&fsd,udf_file_set_descriptor,sizeof(fsd));
            
            udf_extended_file_entry root_extendedentry = {0};
            
			//uint8_t udf_extended_file_entrymem[DATA_SECOTR_SIZE];
            //ReadSector(metadata_partition_lba+firsttestdesc.location+fsd.root_icb.location,udf_extended_file_entrymem);
            
			uint8_t * udf_extended_file_entrymem = udf_file_set_descriptor+(fsd.root_icb.location*DATA_SECOTR_SIZE);
			memcpy(&root_extendedentry,udf_extended_file_entrymem,sizeof(udf_extended_file_entry));
            
            _alloctype = root_extendedentry.icb_tag.flags & 0x0007;
			std::vector<udf_extent_struct> rootextended_extents;
			if(_alloctype == 0){
				for(int i=0;i<(int)(root_extendedentry.u_alloc_descs/8);i++){
					UDF_ExtentAd extendedtestdesc = {0};
					memcpy(&extendedtestdesc,udf_extended_file_entrymem+sizeof(udf_extended_file_entry)+root_extendedentry.u_extended_attr,root_extendedentry.u_alloc_descs);
					udf_extent_struct tmpextent = {0};
					tmpextent.location = extendedtestdesc.location;
					tmpextent.length = extendedtestdesc.length;
					tmpextent.partition_reference = 0;
					rootextended_extents.push_back(tmpextent);
				}
				
			}
			if(_alloctype == 1){
				UDF_LongAd extendedtestdesc = {0};
				memcpy(&extendedtestdesc,udf_extended_file_entrymem+sizeof(udf_extended_file_entry)+root_extendedentry.u_extended_attr,root_extendedentry.u_alloc_descs);
				uint8_t * root_fid = metadata_partition_buffer+(extendedtestdesc.location*DATA_SECOTR_SIZE);
				Parse_FID_Ptr(root_fid,"/",DATA_SECOTR_SIZE);
			}
			
			if(rootextended_extents.size()>0){
				uint8_t * root_fid = metadata_partition_buffer+(rootextended_extents[0].location*DATA_SECOTR_SIZE);
				Parse_FID_Ptr(root_fid,"/",DATA_SECOTR_SIZE);
			}
			
            
        }
#ifdef DEBUG
		for(unsigned int i=0;i<disc_dirlist.size();i++){
            usbdvd_log("%s: %s %lu\r\n",disc_dirlist[i].isdir ? "DIR":"FILE",disc_dirlist[i].fullpath.c_str(),disc_dirlist[i].size);
            
        }
#endif
	
}

CUSBDVD_UDFFS::CUSBDVD_UDFFS(CUSBSCSI * _usb_scsi_ctx,uint32_t _startlba,uint32_t _endlba) 
	: CUSBDVD_DATADISC(_usb_scsi_ctx,_startlba,_endlba){
	
	
	// SAME ON ALL UDF VERSIONS
	
	uint8_t udf_anchorvd[DATA_SECOTR_SIZE];
    
	ReadSector(256,udf_anchorvd);
	UDF_AnchorVolumeDescriptorPointer avdp = {0};
	memcpy(&avdp,udf_anchorvd,sizeof(avdp));
	
	if(avdp.tag.descriptor_version == 2){
		udf15plus = false;
	}else if(avdp.tag.descriptor_version == 3){
		udf15plus = true;
	}
	
	uint8_t udf_pvd[DATA_SECOTR_SIZE];
	uint8_t udf_pvd_2[DATA_SECOTR_SIZE];
	uint8_t udf_partdesc[DATA_SECOTR_SIZE];
	uint8_t udf_lvd[DATA_SECOTR_SIZE];
    
	ReadSector(avdp.main_vds_extent.location,udf_pvd);
	ReadSector(avdp.main_vds_extent.location+1,udf_pvd_2);
	ReadSector(avdp.main_vds_extent.location+2,udf_partdesc);
	ReadSector(avdp.main_vds_extent.location+3,udf_lvd);
	
	
	UDF_PrimaryVolumeDescriptor pvd = {0};
	partition_descriptor_t partdesc = {0};
	memcpy(&pvd,udf_pvd,sizeof(pvd));
	
	
	
	memcpy(&partdesc,udf_partdesc,sizeof(partdesc));
	
	logical_volume_descriptor_t testlvd = {0};
	memcpy(&testlvd,udf_lvd,sizeof(logical_volume_descriptor_t));
	
	uint16_t udfver = (testlvd.domain_identifier.identifier_suffix[1] << 8 ) |  testlvd.domain_identifier.identifier_suffix[0];
	
	
	disc_lvd.udf_version_string = getUDFVersionString(udfver);
	
	
	disc_lvd.VolumeIdentifier = dstring_to_string(testlvd.logical_volume_identifier,128);
	
	disc_lvd.location = testlvd.logical_volume_contents_use.location;
	disc_lvd.length = testlvd.logical_volume_contents_use.length;
	disc_lvd.number_of_partition_maps = testlvd.number_of_partition_maps;
        
	disc_lvd.metadata_partition_size = 0;
	disc_lvd.udfver = udfver;
    
	uint32_t maplen = 0;
	for(uint32_t i=0;i<disc_lvd.number_of_partition_maps;i++){
		uint8_t maptest[2];
		memcpy(maptest,udf_lvd+sizeof(logical_volume_descriptor_t)+maplen,2);
		if(maptest[0] == 0x02){
			metadata_partition_map_t meta_part = {0};
                
			memcpy(&meta_part,udf_lvd+sizeof(logical_volume_descriptor_t)+maplen,sizeof(metadata_partition_map_t));
			disc_lvd.meta_partition_location = meta_part.metadata_file_location;
					
		}
            
            
		maplen+=maptest[1];
	}
        
        
	uint32_t partlbalocation = partdesc.partition_starting_location+testlvd.logical_volume_contents_use.location;
	partitionlba = partlbalocation;
	if(disc_lvd.number_of_partition_maps>1){
		metadata_partition_lba = partdesc.partition_starting_location+testlvd.logical_volume_contents_use.location+disc_lvd.meta_partition_location;
	}
        
	uint8_t udf_first_descriptor[DATA_SECOTR_SIZE];
	if(disc_lvd.number_of_partition_maps>1){
		ReadSector(metadata_partition_lba,udf_first_descriptor);
	}else{
		ReadSector(partlbalocation,udf_first_descriptor);
	}
	
        
	if(udf_first_descriptor[0] == 0x00 && udf_first_descriptor[1] == 0x01){
		udf_fsd_s fsd = {0};
		memcpy(&fsd,udf_first_descriptor,sizeof(fsd));
            
		udf_file_entry root_entry = {0};
		uint8_t root_fileentry[DATA_SECOTR_SIZE];
		ReadSector(partlbalocation+fsd.root_icb.location,root_fileentry);
		memcpy(&root_entry,root_fileentry,sizeof(udf_file_entry));
		UDF_ExtentAd testdesc = {0};
		memcpy(&testdesc,root_fileentry+sizeof(udf_file_entry)+root_entry.u_extended_attr,root_entry.u_alloc_descs);
            
		uint8_t root_fid[DATA_SECOTR_SIZE];
		ReadSector(partlbalocation+testdesc.location,root_fid);
		Parse_FID_Ptr(root_fid,"/",DATA_SECOTR_SIZE);
            
        }
	if(udf_first_descriptor[0] == 0x0a && udf_first_descriptor[1] == 0x01){
			udf_extended_file_entry root_filentry = {0};
            memcpy(&root_filentry,udf_first_descriptor,sizeof(udf_extended_file_entry));
            
			uint8_t _alloctype = root_filentry.icb_tag.flags & 0x0007;
			
			metadata_partition_buffer = (uint8_t *)malloc(root_filentry.info_len*sizeof(uint8_t));
			disc_lvd.metadata_partition_size = root_filentry.info_len;
			std::vector<udf_extent_struct> metadata_extents;
			if(_alloctype == 0x00){
				
				
				for(int i=0;i<(int)(root_filentry.u_alloc_descs/8);i++){
					UDF_ExtentAd firsttestdesc = {0};
					memcpy(&firsttestdesc,udf_first_descriptor+sizeof(udf_extended_file_entry)+root_filentry.u_extended_attr+(i*8),root_filentry.u_alloc_descs);
					udf_extent_struct tmpextent = {0};
					tmpextent.location = firsttestdesc.location;
					tmpextent.length = firsttestdesc.length;
					tmpextent.partition_reference = 0;
					
					metadata_extents.push_back(tmpextent);
					//ReadSectorsLen(partitionlba+firsttestdesc.location,firsttestdesc.length,&metadata_partition_buffer[_buffoff]);
					//_buffoff+=firsttestdesc.length;
					
					
				}
				
			}else if(_alloctype == 0x01){
				UDF_LongAd firsttestdesc = {0};
				memcpy(&firsttestdesc,udf_first_descriptor+sizeof(udf_extended_file_entry)+root_filentry.u_extended_attr,root_filentry.u_alloc_descs);
				udf_extent_struct tmpextent = {0};
				tmpextent.location = firsttestdesc.location;
				tmpextent.length = firsttestdesc.length;
				tmpextent.partition_reference = firsttestdesc.partition_reference;
				metadata_extents.push_back(tmpextent);
			}
			uint32_t _buffoff = 0;
			usbdvd_log("Metadata Partition Extents: %lu\r\n",metadata_extents.size());
			for(unsigned int i=0;i<metadata_extents.size();i++){
				usbdvd_log("Reading Metadata Partition: LBA %u OFF %u SECTORS: %u\r\n",partitionlba,metadata_extents[i].location,metadata_extents[i].length/DATA_SECOTR_SIZE);
				ReadSectorsLen(partitionlba+metadata_extents[i].location,metadata_extents[i].length,&metadata_partition_buffer[_buffoff]);
				_buffoff+=metadata_extents[i].length;	
				
			}
			
			
			if(metadata_partition_buffer==NULL)return;
			
			// DOING SEQUENTIAL READ OF METADATA PARTITION 
			//uint8_t bigbuffer[root_filentry.info_len];
			//metadata_partition_buffer = (uint8_t *)malloc(root_filentry.info_len);
			//ReadSectorsLen(metadata_partition_lba+_firstseclocation,root_filentry.info_len,metadata_partition_buffer);
			
	
            //uint8_t udf_file_set_descriptor[DATA_SECOTR_SIZE];
            //ReadSector(metadata_partition_lba+firsttestdesc.location,udf_file_set_descriptor);
            uint8_t * udf_file_set_descriptor = &metadata_partition_buffer[0];
			
			
			metadata_fsd_offset = metadata_extents[0].location;
            udf_fsd_s fsd = {0};
            memcpy(&fsd,udf_file_set_descriptor,sizeof(fsd));
            
            udf_extended_file_entry root_extendedentry = {0};
            
			//uint8_t udf_extended_file_entrymem[DATA_SECOTR_SIZE];
            //ReadSector(metadata_partition_lba+firsttestdesc.location+fsd.root_icb.location,udf_extended_file_entrymem);
            
			uint8_t * udf_extended_file_entrymem = udf_file_set_descriptor+(fsd.root_icb.location*DATA_SECOTR_SIZE);
			memcpy(&root_extendedentry,udf_extended_file_entrymem,sizeof(udf_extended_file_entry));
            
            _alloctype = root_extendedentry.icb_tag.flags & 0x0007;
			std::vector<udf_extent_struct> rootextended_extents;
			if(_alloctype == 0){
				for(int i=0;i<(int)(root_extendedentry.u_alloc_descs/8);i++){
					UDF_ExtentAd extendedtestdesc = {0};
					memcpy(&extendedtestdesc,udf_extended_file_entrymem+sizeof(udf_extended_file_entry)+root_extendedentry.u_extended_attr,root_extendedentry.u_alloc_descs);
					udf_extent_struct tmpextent = {0};
					tmpextent.location = extendedtestdesc.location;
					tmpextent.length = extendedtestdesc.length;
					tmpextent.partition_reference = 0;
					rootextended_extents.push_back(tmpextent);
				}
				
			}
			if(_alloctype == 1){
				UDF_LongAd extendedtestdesc = {0};
				memcpy(&extendedtestdesc,udf_extended_file_entrymem+sizeof(udf_extended_file_entry)+root_extendedentry.u_extended_attr,root_extendedentry.u_alloc_descs);
				uint8_t * root_fid = metadata_partition_buffer+(extendedtestdesc.location*DATA_SECOTR_SIZE);
				Parse_FID_Ptr(root_fid,"/",DATA_SECOTR_SIZE);
			}
			
			if(rootextended_extents.size()>0){
				uint8_t * root_fid = metadata_partition_buffer+(rootextended_extents[0].location*DATA_SECOTR_SIZE);
				Parse_FID_Ptr(root_fid,"/",DATA_SECOTR_SIZE);
			}
			
            
        }
#ifdef DEBUG
		for(unsigned int i=0;i<disc_dirlist.size();i++){
            usbdvd_log("%s: %s %lu\r\n",disc_dirlist[i].isdir ? "DIR":"FILE",disc_dirlist[i].fullpath.c_str(),disc_dirlist[i].size);
            
        }
#endif
}


CUSBDVD_UDFFS::~CUSBDVD_UDFFS(){
	if(metadata_partition_buffer!=NULL)free(metadata_partition_buffer);
}


int CUSBDVD_UDFFS::ReadExtents(std::vector<sector_range_struct> _seclist,uint32_t _len,uint8_t * buffer){
	
	uint32_t buffoff = 0;
	for(unsigned int i=0;i<_seclist.size();i++){
		
		for(unsigned int y=0;y<_seclist[i].len;i++){
			ReadSector(_seclist[i].sector+y,&buffer[buffoff]);
			buffoff+=DATA_SECOTR_SIZE;
		}
	}
	
	return 0;
}


/*

int CUSBDVD_UDFFS::UDFReadData(disc_dirlist_struct * _filedesc,uint32_t pos,uint32_t size,uint8_t * buf){
    
	size_t offset_firstsector = pos%DATA_SECOTR_SIZE;
	std::vector<sector_range_struct> test = udf_extents_to_sectors(_filedesc->extents,pos,size);
	size_t buffosff = 0;
	size_t remread = size;
	for(unsigned int i=0;i<test.size();i++){
		uint32_t startblock = partitionlba+test[i].sector;
		uint32_t endblock = startblock+test[i].len;
		
		for(size_t numblock = startblock;numblock<=endblock && remread > 0;numblock++){
			size_t toread;
			
			if(i==0 && numblock == startblock){
				toread = std::min(remread,(size_t)DATA_SECOTR_SIZE-offset_firstsector);
			}else{
				toread = std::min(remread,(size_t)DATA_SECOTR_SIZE);
			}
			
			if(numblock != drive_readbuffer.read_sector){
				ReadSector(numblock,drive_readbuffer.read_buffer);
				drive_readbuffer.read_sector = numblock;
			}
			uint32_t offsetinblock = (i==0 && numblock == startblock) ? offset_firstsector : 0;
			memcpy(buf+buffosff,drive_readbuffer.read_buffer+offsetinblock,toread);
			buffosff+=toread;
			remread-=toread;
		}
		
	}
	
	return 0;
	

}

*/

int CUSBDVD_UDFFS::UDFReadData(disc_dirlist_struct * _filedesc,uint32_t pos,uint32_t size,uint8_t * buf){
    
	std::vector<sector_range_struct> test = udf_extents_to_sectors(_filedesc->extents,pos,size);
	if(isofile){
		uint32_t buffoff = 0;
		uint64_t remread = size;
		for(unsigned int i=0;i<test.size();i++){
			uint32_t startblock = partitionlba+test[i].sector;
			size_t offinbinary = (startblock*DATA_SECOTR_SIZE)+pos;
			fseek(isofp,offinbinary,SEEK_SET);
			uint32_t readsize = std::min(remread,test[i].len*DATA_SECOTR_SIZE);
			fread(buf+buffoff, sizeof(uint8_t), readsize,isofp);
			buffoff+=readsize;
			remread-=readsize;
		}
		return 0;
	}
	
	size_t offset_firstsector = pos%DATA_SECOTR_SIZE;
	
	size_t buffosff = 0;
	size_t remread = size;
	for(unsigned int i=0;i<test.size();i++){
		uint32_t startblock = partitionlba+test[i].sector;
		//uint32_t endblock = startblock+test[i].len;
		uint32_t readsize = std::min(remread,test[i].len*DATA_SECOTR_SIZE);
		size_t offsetinblock = 0;
		if(i==0)offsetinblock = offset_firstsector;
		size_t interbuffsize =  readsize;
		uint8_t interbuf[interbuffsize];
	
		ReadSectorsLen(startblock,interbuffsize,interbuf);
		memcpy(buf+buffosff,interbuf+offsetinblock,readsize-offsetinblock);
		remread-=readsize-offsetinblock;
		buffosff+=readsize-offsetinblock;
	}
	
	return 0;
	

}



/*

int CUSBDVD_UDFFS::UDFReadData(disc_dirlist_struct * _filedesc,uint32_t pos,uint32_t size,uint8_t * buf){
    
	size_t offset_firstsector = pos%DATA_SECOTR_SIZE;
	std::vector<sector_range_struct> test = udf_extents_to_sectors(_filedesc->extents,pos,size);
	size_t buffosff = 0;
	size_t remread = size;
	for(unsigned int i=0;i<test.size();i++){
		uint32_t startblock = partitionlba+test[i].sector;
		uint32_t endblock = startblock+test[i].len;
		size_t interbuffsize =  (endblock-startblock)*DATA_SECOTR_SIZE;
		size_t buffwritesize = std::min(remread,interbuffsize);
		uint8_t interbuf[interbuffsize];
		uint32_t offsetinblock = 0;
		if(i==0)offsetinblock=offset_firstsector;
		ReadSectorsLen(startblock,interbuffsize,interbuf);
		memcpy(buf+buffosff,interbuf+offsetinblock,buffwritesize);
		remread-=buffwritesize;
		buffosff+=buffwritesize;
		
	}
	
	return 0;
	

}

*/