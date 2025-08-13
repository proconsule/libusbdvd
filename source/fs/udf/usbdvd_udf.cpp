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
    
} file_identifier_descriptor_t;

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

void CUSBDVD_UDFFS::Parse_FileEntry_Ptr(uint8_t * buffer,disc_dirlist_struct * _tmpfile){
	
	udf_file_entry testentry = {0};
	uint32_t fe_pos = 0;
	memcpy(&testentry,buffer,sizeof(testentry));
	fe_pos+=sizeof(udf_file_entry);
	_tmpfile->size = testentry.info_len;
	if(testentry.icb_tag.file_type == 0x04)_tmpfile->isdir = true;
	if(testentry.icb_tag.file_type == 0x05)_tmpfile->isdir = false;
	
	fe_pos+= testentry.u_extended_attr;
	
	
		
	UDF_ExtentAd testdesc = {0};
	memcpy(&testdesc,buffer+fe_pos,testentry.u_alloc_descs);
	_tmpfile->partlocation = testdesc.location;
	_tmpfile->partlocation = testdesc.location;
	_tmpfile->lba = partitionlba+testdesc.location;
		
	
}


void CUSBDVD_UDFFS::Parse_FID_Ptr(uint8_t * buffer,std::string _path){
	
	uint32_t fid_pos=0;
	
	while(true){
		if(fid_pos+sizeof(file_identifier_descriptor_t) >= DATA_SECOTR_SIZE)break;
		file_identifier_descriptor_t fid = {0};
		memcpy(&fid,buffer+fid_pos,sizeof(fid));
		if(fid.tag.tag_identifier != 0x0101)break;
		fid_pos+=sizeof(fid);
		fid_pos+=fid.length_of_implementation_use;
		
		disc_dirlist_struct tmpentry;
		
		std::string _filename = UTF16_Truncate(buffer+fid_pos+2,fid.length_of_file_identifier);
		tmpentry.name = _filename;
		if(_path == "/"){
			tmpentry.fullpath = _path + _filename;
		}
		else{
			tmpentry.fullpath = _path + "/" +_filename;
		}
		
		
		uint8_t fe_buffer[DATA_SECOTR_SIZE];
		ReadSector(partitionlba+fid.icb.location,fe_buffer);
		
		Parse_FileEntry_Ptr(fe_buffer,&tmpentry);
		if(tmpentry.isdir && tmpentry.name != ""){
			uint8_t recursive_buffer[DATA_SECOTR_SIZE];
			ReadSector(partitionlba+tmpentry.partlocation,recursive_buffer);
			Parse_FID_Ptr(recursive_buffer,tmpentry.fullpath.c_str());
			
		}
		if(tmpentry.name != ""){
			disc_dirlist.push_back(tmpentry);
		}
		
		fid_pos+=fid.length_of_file_identifier;
		uint32_t fid_padding = 4-((sizeof(fid)+fid.length_of_implementation_use+fid.length_of_file_identifier)%4);
		fid_pos+=fid_padding;
		
	}
	
}

CUSBDVD_UDFFS::CUSBDVD_UDFFS(CUSBSCSI * _usb_scsi_ctx,uint32_t _startlba,uint32_t _endlba) 
	: CUSBDVD_DATADISC(_usb_scsi_ctx,_startlba,_endlba){
	
	
	uint8_t udf_anchorvd[DATA_SECOTR_SIZE];
    
    ReadSector(256,udf_anchorvd);
	UDF_AnchorVolumeDescriptorPointer avdp = {0};
    memcpy(&avdp,udf_anchorvd,sizeof(avdp));
	
	
	
	printf(CONSOLE_ESC(13;2H));

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
	
	udf_version_string = getUDFVersionString(udfver);
	
	if(udfver> 0x0102){
		return;
	}
	
	
	uint32_t partlbalocation = partdesc.partition_starting_location+testlvd.logical_volume_contents_use.location;
	partitionlba = partlbalocation;
	uint8_t udf_file_set_descriptor[DATA_SECOTR_SIZE];
	ReadSector(partlbalocation,udf_file_set_descriptor);
	udf_fsd_s fsd = {0};
	memcpy(&fsd,udf_file_set_descriptor,sizeof(fsd));
	uint8_t root_fid[DATA_SECOTR_SIZE];
	ReadSector(partlbalocation+fsd.root_icb.location+1,root_fid);
	Parse_FID_Ptr(root_fid,"/");
	
}
