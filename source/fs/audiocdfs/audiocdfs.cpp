#include "audiocdfs.h"
#include "usbdvd_utils.h"

CAUDIOCD_PSEUDOFS::CAUDIOCD_PSEUDOFS(std::string _binfilename){
    if (pthread_mutex_init(&this->read_lock, NULL) != 0) {
        usbdvd_log("\n mutex init has failed\n");
        return;
    }
    this->binfilename = _binfilename;
    this->binfile_fp = fopen(this->binfilename.c_str(),"rb");
    this->isfile = true;
    
    audiocdfs_gettocfromfile();
    
}


CAUDIOCD_PSEUDOFS::CAUDIOCD_PSEUDOFS(CDDVD_TOC mytoc,CUSBSCSI * _usb_scsi_ctx){
	usb_scsi_ctx = _usb_scsi_ctx;
	if (pthread_mutex_init(&this->read_lock, NULL) != 0) {
        usbdvd_log("\n mutex init has failed\n");
        return;
    }
	
	this->sectornum = 0;
    this->toc = mytoc;
	this->iscdaudio = true;
	
}

CAUDIOCD_PSEUDOFS::~CAUDIOCD_PSEUDOFS(){
	pthread_mutex_destroy(&this->read_lock);
}

int CAUDIOCD_PSEUDOFS::audiocdfs_gettocfromfile(){
    fseek(this->binfile_fp,2352*55,SEEK_SET);
    for(int i=0;i<2352;i++){
        char buffer[1];
        fread(buffer, sizeof(char), sizeof(buffer), this->binfile_fp);
        usbdvd_log("%02hhx ",buffer[0]);
        
    }
    return -1;
}

int CAUDIOCD_PSEUDOFS::audiocdfs_gettrackfilesize(int tracknum){
    if(tracknum>this->toc.hdr.last_track)return -1;
    return audiocdfs_gettracksize(tracknum)+44;
}
int CAUDIOCD_PSEUDOFS::audiocdfs_gettracknums(){
    return this->toc.hdr.last_track-1;
}

int CAUDIOCD_PSEUDOFS::audiocdfs_File2Track(const char * filename){
    int testnum = -1;
    sscanf(filename,"Track%02d.wav",&testnum);
    return testnum-1;
}

int CAUDIOCD_PSEUDOFS::audiocdfs_readdata(uint32_t tracknum,uint32_t pos,uint32_t size,uint8_t * buf){
  
  pthread_mutex_lock(&this->read_lock);
  if(tracknum>this->toc.hdr.last_track)return -1;
  int filelba = audiocdfs_gettracklba(tracknum)-150;
  
  uint32_t secpos = pos-44;
  uint32_t secorreadsize = size;
  bool include_header = false;
  if(pos<44)include_header = true;
  if(include_header){
	  secpos = 0;
	  secorreadsize = size-44-pos;
  }
  
  size_t firstsector =  filelba + (secpos/CD_SECTOR_SIZE_AUDIO);
  size_t offset_firstsector = secpos%CD_SECTOR_SIZE_AUDIO;
  size_t lastsector = firstsector + (secorreadsize-1/CD_SECTOR_SIZE_AUDIO);
  
  size_t remread = secorreadsize;
  size_t buffosff = 0;
   
  if(include_header){
	buffosff=44-pos;
	wav_hdr test;
	createWavHeader(&test,tracknum);
    uint8_t *testpointer = (uint8_t *)&test;
    memcpy(buf,testpointer+pos,44-pos);
	remread-=(44-pos);
	
  }
  
  for(size_t numblock = firstsector;numblock<=lastsector && remread > 0;numblock++){
		size_t toread;
		size_t offsetinblock = (numblock == firstsector) ? offset_firstsector : 0;
		if(numblock == firstsector){
			toread = std::min(remread,(size_t)CD_SECTOR_SIZE_AUDIO-offset_firstsector);
		}else{
			toread = std::min(remread,(size_t)CD_SECTOR_SIZE_AUDIO);
		}
		
		if(numblock != this->sectornum){
			usb_scsi_ctx->UsbDvdReadCD_Audio(0,numblock,1,this->lastbuffer);
			this->sectornum = numblock;
		}
		
		
		memcpy(buf+buffosff,this->lastbuffer+offsetinblock,toread);
		
		buffosff+=toread;
		remread-=toread;
  }
  
  pthread_mutex_unlock(&this->read_lock);
  return 0;
  
}

int CAUDIOCD_PSEUDOFS::audiocdfs_gettracklba(int tracknum){
    if(tracknum>this->toc.hdr.last_track)return -1;
    return ((this->toc.tracks[tracknum].MIN*60)+this->toc.tracks[tracknum].SEC)*75+this->toc.tracks[tracknum].FRAME;
    
}


std::string my_to_string2(int n, int width = 2)
{
    auto s = std::to_string(n);
    if (int(s.size()) < width) s.insert(0, width - s.size(), '0');
    return s;
}


std::string CAUDIOCD_PSEUDOFS::audiocdfs_Track2Name(int tracknum){
    
    std::string mystring = std::string("Track") + my_to_string2(tracknum+1) + std::string(".wav");
    return mystring;
    
}

int CAUDIOCD_PSEUDOFS::audiocdfs_gettracksize(int tracknum){
    
    if(tracknum>this->toc.hdr.last_track)return -1;
    int mylba = ((this->toc.tracks[tracknum].MIN*60)+this->toc.tracks[tracknum].SEC)*75+this->toc.tracks[tracknum].FRAME;
    int nextlba = ((this->toc.tracks[tracknum+1].MIN*60)+this->toc.tracks[tracknum+1].SEC)*75+this->toc.tracks[tracknum+1].FRAME;
    return (nextlba-mylba)*2352;
    
}

void CAUDIOCD_PSEUDOFS::createWavHeader(wav_hdr * _hdr,int tracknum){
    
    _hdr->ChunkSize = audiocdfs_gettracksize(tracknum) + sizeof(wav_hdr) - 8;
    _hdr->Subchunk2Size = audiocdfs_gettracksize(tracknum) + sizeof(wav_hdr) - 44;
	
	_hdr->RIFF[0] = 'R';
	_hdr->RIFF[1] = 'I';
	_hdr->RIFF[2] = 'F';
	_hdr->RIFF[3] = 'F';
	
	_hdr->WAVE[0] = 'W';
	_hdr->WAVE[1] = 'A';
	_hdr->WAVE[2] = 'V';
	_hdr->WAVE[3] = 'E';
	
	_hdr->fmt[0] = 'f';
	_hdr->fmt[1] = 'm';
	_hdr->fmt[2] = 't';
	_hdr->fmt[3] = ' ';
	
	_hdr->Subchunk1Size = 16;
    _hdr->AudioFormat = 1;
    _hdr->SamplesPerSec = 44100;
    _hdr->bytesPerSec = 44100 * 4;
    _hdr->blockAlign = 4;
    _hdr->bitsPerSample = 16;
	_hdr->AudioChannels = 2;
	
	_hdr->Subchunk2ID[0] = 'd';
	_hdr->Subchunk2ID[1] = 'a';
	_hdr->Subchunk2ID[2] = 't';
	_hdr->Subchunk2ID[3] = 'a';
	
	
}


