#include "usbdvd_datadisc.h"
#include "usbdvd_utils.h"
#include "usbdvd_css.h"
#include "usbdvd_utils.h"

const uint32_t MAX_READ_SECTORS = 16;


CUSBDVD_DATADISC::~CUSBDVD_DATADISC(){
    if(isofile){
        if(isofp)fclose(isofp);
    }
    cleanup_ifo_cache();
    
}

CUSBDVD_DATADISC::CUSBDVD_DATADISC(CUSBSCSI * _usb_scsi_ctx,uint32_t _startlba,uint32_t _endlba){
    
    absstartlba = _startlba;
    absendlba = _endlba;
    usb_scsi_ctx = _usb_scsi_ctx;
    isofile = false;

}

CUSBDVD_DATADISC::CUSBDVD_DATADISC(std::string _filename){
    
    filename = _filename;
    isofp = fopen(filename.c_str(),"rb");
    isofile = true;
    
}

uint32_t CUSBDVD_DATADISC::GetFileSize(std::string _filename){
    for(unsigned int i=0;i<disc_dirlist.size();i++){
        if(_filename == disc_dirlist[i].fullpath)return disc_dirlist[i].size;
    }
    return 0;
}

int CUSBDVD_DATADISC::GetFileDesc(std::string _filename,disc_dirlist_struct & _filedesc){
    for(unsigned int i=0;i<disc_dirlist.size();i++){
        if(_filename == disc_dirlist[i].fullpath || _filename == disc_dirlist[i].fullpath + "/"){
            _filedesc = disc_dirlist[i];
            return 0;
        }
    }
    return -1;
}

disc_dirlist_struct * CUSBDVD_DATADISC::GetFileDescFromIDX(int idx){
    if(idx>=(int)disc_dirlist.size())return NULL;
    return &disc_dirlist[idx];
}

int CUSBDVD_DATADISC::FindFile(std::string _filename){
    for(unsigned int i=0;i<disc_dirlist.size();i++){
        if(_filename == disc_dirlist[i].fullpath || _filename == disc_dirlist[i].fullpath + "/"){
           return i;
        }
    }
    return -1;
}


int getTitoloFromFilename(const std::string& filename) {
    size_t pos = filename.find("VTS_");
    if (pos != std::string::npos && pos + 6 < filename.length()) {
        std::string num = filename.substr(pos + 4, 2);
        return std::atoi(num.c_str());
    }
    return -1;
}


int CUSBDVD_DATADISC::FindTitleKey_IDX(std::string _fullpath){
    if (_fullpath.find("VIDEO_TS.VOB") != std::string::npos) {
        return 0;  
    }
       
    size_t pos = _fullpath.find("VTS_");
    if (pos != std::string::npos && pos + 6 < _fullpath.length()) {
        std::string num = _fullpath.substr(pos + 4, 2);
        return std::atoi(num.c_str());
    }
    return -1;
}

int CUSBDVD_DATADISC::ReadSector(uint32_t sector,uint8_t * buffer){
    
    if(isofile){
        isofile_filesectorread(sector,buffer);
        return 0;
    }else{
        return usb_scsi_ctx->UsbDvdReadCD_Data(0,sector,1,buffer);
    }
    return -1;
}

int CUSBDVD_DATADISC::ReadNumSectors(uint32_t startsector,uint16_t numblocks,uint8_t * buffer){
    
    if(isofile){
        //isofile_filesectorread(sector,buffer);
        return 0;
    }else{
        return usb_scsi_ctx->UsbDvdReadCD_Data(0,startsector,numblocks,buffer);
    }
    return -1;
}


int CUSBDVD_DATADISC::isofile_filesectorread(uint32_t sector,uint8_t *buffer){
    fseek(isofp,sector*DATA_SECOTR_SIZE,SEEK_SET);
    fread(buffer, sizeof(uint8_t), DATA_SECOTR_SIZE,isofp);
    return 0;
}


int CUSBDVD_DATADISC::ReadData(disc_dirlist_struct * _filedesc,uint32_t pos,uint32_t size,uint8_t * buf){
    
    if(_filedesc->cached){
        auto lk = std::scoped_lock(read_mutex);
        uint8_t * _fileref = get_ifo_data(_filedesc->idx,NULL);
        memcpy(buf,&_fileref[pos],size);
        return 0;
    }
    
    size_t firstsector =  _filedesc->lba + (pos/DATA_SECOTR_SIZE);
    size_t offset_firstsector = pos%DATA_SECOTR_SIZE;
    size_t lastsector = firstsector + (size/DATA_SECOTR_SIZE);
    
    size_t remread = size;
    size_t buffosff = 0;
    
    for(size_t numblock = firstsector;numblock<=lastsector && remread > 0;numblock++){
        size_t toread;
        size_t offsetinblock = (numblock == firstsector) ? offset_firstsector : 0;
        if(numblock == firstsector){
            toread = std::min(remread,(size_t)DATA_SECOTR_SIZE-offset_firstsector);
        }else{
            toread = std::min(remread,(size_t)DATA_SECOTR_SIZE);
        }
        
        if(numblock != drive_readbuffer.read_sector){
            ReadSector(numblock,drive_readbuffer.read_buffer);
            drive_readbuffer.read_sector = numblock;
        }
        
        if(DVD_CSS && currenttitlekey_idx >= 0){
            dvdcss_unscramble(titlekeys[currenttitlekey_idx].key,drive_readbuffer.read_buffer);
            //printf("cssret %d\r\n",cssret);
        }
        memcpy(buf+buffosff,drive_readbuffer.read_buffer+offsetinblock,toread);
        
        buffosff+=toread;
        remread-=toread;
        
        
    }
    //usb_scsi_ctx->UsbDvdReadAhead(0,lastsector,(size-1/DATA_SECOTR_SIZE));
    return 0;
    

}

int CUSBDVD_DATADISC::ReadSectorsLen(uint32_t startsector,uint32_t _len,uint8_t * buffer){
    
    
    //size_t lastsector = startsector + (_len/DATA_SECOTR_SIZE)+1;
    //size_t interbufferlen = 1+(lastsector-startsector)*DATA_SECOTR_SIZE;
    //uint8_t interbuf[interbufferlen];
    
    //uint32_t numreq = ((lastsector-startsector)/MAX_READ_SECTORS)+1;
    //uint32_t currsecnums = lastsector-startsector;
    uint32_t numsectors = (_len/DATA_SECOTR_SIZE)+1;
    uint32_t buffosff = 0;
    size_t remread = _len;
    for(uint32_t i=0;i<numsectors;i++){
        size_t toread  = std::min(remread,(size_t)DATA_SECOTR_SIZE);
        ReadSector(startsector+i,drive_readbuffer.read_buffer);
        drive_readbuffer.read_sector = startsector+i;
        
        memcpy(buffer+buffosff,drive_readbuffer.read_buffer,toread);
        buffosff+=toread;
        remread-=toread;
    }

    
    return 0;
}

void CUSBDVD_DATADISC::DVDGetAllCSSKeys(){
    char filename[0xff];
    std::vector<disc_dirlist_struct *> checkfiles;
    for(int title=0;title<10;title++){
        if( title == 0 ) {
            strcpy( filename, "/VIDEO_TS/VIDEO_TS.VOB" );
        } else {
            sprintf( filename, "/VIDEO_TS/VTS_%02d_%d.VOB", title, 0 );
        }
        int present = -1;
        present = FindFile(filename);
        if(present>=0){
            disc_dirlist_struct *testele = GetFileDescFromIDX(present); 
            uint8_t testtitlekey[5];
            int keyret = usb_scsi_ctx->CrackTitleKey(testele->lba,testele->size,testtitlekey);
            if(keyret >= 0){
                css_titlekey_struct tmp;
                //tmp.fullpathvob = testele->fullpath.c_str();
                memcpy(tmp.key,testtitlekey,5);
                tmp.titlenum = (uint8_t)title;
                titlekeys.push_back(tmp);
                continue;
            }
                                
        }
        if(present==-1 && title == 0){
            css_titlekey_struct tmp;
            memset(tmp.key,0,5);
            titlekeys.push_back(tmp);
        }
        if( title == 0 ) continue;
        sprintf( filename, "/VIDEO_TS/VTS_%02d_%d.VOB", title, 1 );
                        
        present = FindFile(filename);
        if(present>=0){
            uint8_t testtitlekey[5];
            disc_dirlist_struct *testele = GetFileDescFromIDX(present); 
            int keyret = usb_scsi_ctx->CrackTitleKey(testele->lba,testele->size,testtitlekey);
            if(keyret >= 0){
                css_titlekey_struct tmp;
                //tmp.fullpathvob = testele->fullpath.c_str();
                memcpy(tmp.key,testtitlekey,5);
                titlekeys.push_back(tmp);
            }
            
        }
                        
    }
}


void CUSBDVD_DATADISC::cleanup_ifo_cache() {
    if (g_ifo_cache.entries) {
        for (int i = 0; i < g_ifo_cache.num_entries; i++) {
            if (g_ifo_cache.entries[i].is_loaded) {
                free(g_ifo_cache.entries[i].data);
            }
        }
        free(g_ifo_cache.entries);
        g_ifo_cache.entries = NULL;
        g_ifo_cache.num_entries = 0;
        g_ifo_cache.capacity = 0;
    }
}


uint8_t* CUSBDVD_DATADISC::get_ifo_data(int file_idx, size_t *size_out) {
    for (int i = 0; i < g_ifo_cache.num_entries; i++) {
        if (g_ifo_cache.entries[i].file_idx == file_idx && 
            g_ifo_cache.entries[i].is_loaded) {
            
            if (size_out) {
                *size_out = g_ifo_cache.entries[i].size;
            }
            return g_ifo_cache.entries[i].data;
        }
    }
    return NULL;
}

int CUSBDVD_DATADISC::load_ifo_by_index(int file_idx) {
    
    
    // Espandi array se necessario
    if (g_ifo_cache.num_entries >= g_ifo_cache.capacity) {
        g_ifo_cache.capacity *= 2;
        g_ifo_cache.entries = (ifo_cache_entry_t*)realloc(g_ifo_cache.entries, 
                                     g_ifo_cache.capacity * sizeof(ifo_cache_entry_t));
        if (!g_ifo_cache.entries) return -1;
    }
    
    uint8_t *buffer = (uint8_t *)malloc(disc_dirlist[file_idx].size);
    if (!buffer) {
        return -1;
    }
    
    
    ReadData(&disc_dirlist[file_idx],0,disc_dirlist[file_idx].size,(uint8_t *)buffer);
    
    // Aggiungi alla cache
    ifo_cache_entry_t *entry = &g_ifo_cache.entries[g_ifo_cache.num_entries++];
    entry->file_idx = file_idx;
    entry->data = buffer;
    entry->size = disc_dirlist[file_idx].size;
    entry->is_loaded = true;
    
    return 0;
}

void CUSBDVD_DATADISC::Cache_IFO_Files(){
    int _entrycount = 0;
    cleanup_ifo_cache();
    for(unsigned int i=0;i<disc_dirlist.size();i++){
        if(endsWithIfo(disc_dirlist[i].name)){
            //printf("Caching : %s %lu\r\n",disc_dirlist[i].name.c_str(),disc_dirlist[i].size);
            _entrycount++;
        }
        
    }
    
    g_ifo_cache.capacity = _entrycount;
    g_ifo_cache.entries = (ifo_cache_entry_t*)malloc(_entrycount * sizeof(ifo_cache_entry_t));
    g_ifo_cache.num_entries = 0;
    
    for(unsigned int i=0;i<disc_dirlist.size();i++){
        if(endsWithIfo(disc_dirlist[i].name)){
            usbdvd_log("Caching: %s\r\n",disc_dirlist[i].name.c_str());
            if(DVD_CSS){
                currenttitlekey_idx = FindTitleKey_IDX(disc_dirlist[i].fullpath);
            }
            load_ifo_by_index(i);
            disc_dirlist[i].cached = true;
            disc_dirlist[i].idx = i;
        }
        
    }
    
    
}


bool CUSBDVD_DATADISC::isAACSProtected(const std::string& directory_path) {
    
    std::string bdmv_path = usbdvdutils_joinPath(directory_path, "BDMV");
    if (!usbdvdutils_pathExists(bdmv_path) || !usbdvdutils_isDirectory(bdmv_path)) {
        return false; 
    }
    
    std::string aacs_path = usbdvdutils_joinPath(directory_path, "AACS");
    if (usbdvdutils_pathExists(aacs_path) && usbdvdutils_isDirectory(aacs_path)) {
        std::string unit_key_path = usbdvdutils_joinPath(aacs_path, "Unit_Key_RO.inf");
        if (usbdvdutils_pathExists(unit_key_path)) {
            return true;
        }
        
        std::string content_cert_path = usbdvdutils_joinPath(aacs_path, "Content000.cer");
        if (usbdvdutils_pathExists(content_cert_path)) {
            return true;
        }
        
        std::string mkb_path = usbdvdutils_joinPath(aacs_path, "MKB_RO.inf");
        if (usbdvdutils_pathExists(mkb_path)) {
            return true;
        }
    }
    
    std::string backup_path = usbdvdutils_joinPath(bdmv_path, "BACKUP");
    if (usbdvdutils_pathExists(backup_path) && usbdvdutils_isDirectory(backup_path)) {
        
        std::string backup_clipinf = usbdvdutils_joinPath(backup_path, "CLIPINF");
        std::string backup_playlist = usbdvdutils_joinPath(backup_path, "PLAYLIST");
        
        if (usbdvdutils_pathExists(backup_clipinf) && usbdvdutils_pathExists(backup_playlist)) {
            return true; 
        }
    }
    
    return false;
}