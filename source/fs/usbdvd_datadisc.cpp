#include "usbdvd_datadisc.h"
#include "usbdvd_utils.h"

//#define MAX_READ_SECTORS 16

const uint32_t MAX_READ_SECTORS = 16;


CUSBDVD_DATADISC::~CUSBDVD_DATADISC(){
	if(isofile){
		if(isofp)fclose(isofp);
	}
	pthread_mutex_destroy(&this->read_lock);
}

CUSBDVD_DATADISC::CUSBDVD_DATADISC(CUSBSCSI * _usb_scsi_ctx,uint32_t _startlba,uint32_t _endlba){
	if (pthread_mutex_init(&this->read_lock, NULL) != 0) {
        usbdvd_log("\n mutex init has failed\n");
        return;
    }
	absstartlba = _startlba;
	absendlba = _endlba;
	usb_scsi_ctx = _usb_scsi_ctx;
	isofile = false;

}

CUSBDVD_DATADISC::CUSBDVD_DATADISC(std::string _filename){
    
    if (pthread_mutex_init(&this->read_lock, NULL) != 0) {
        usbdvd_log("\n mutex init has failed\n");
        return;
    }
	
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
        if(_filename == disc_dirlist[i].fullpath){
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
		if(_filename == disc_dirlist[i].fullpath){
           return i;
        }
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

/*
int CUSBDVD_DATADISC::ReadData(disc_dirlist_struct * _filedesc,uint32_t pos,uint32_t size,uint8_t * buf){
    
	
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
		
		
		memcpy(buf+buffosff,drive_readbuffer.read_buffer+offsetinblock,toread);
		
		buffosff+=toread;
		remread-=toread;
		
		
	}
	//usb_scsi_ctx->UsbDvdReadAhead(0,lastsector,(size-1/DATA_SECOTR_SIZE));
	return 0;
	

}
*/

int CUSBDVD_DATADISC::ReadData(disc_dirlist_struct * _filedesc,uint32_t pos,uint32_t size,uint8_t * buf){
    
	if(isofile){
		size_t firstsector =  _filedesc->lba;
		size_t offinbinary = (firstsector*DATA_SECOTR_SIZE)+pos;
		fseek(isofp,offinbinary,SEEK_SET);
		fread(buf, sizeof(uint8_t), size,isofp);
		return 0;
	}
	
	size_t firstsector =  _filedesc->lba + (pos/DATA_SECOTR_SIZE);
	size_t offset_firstsector = pos%DATA_SECOTR_SIZE;
	size_t lastsector = firstsector + (size/DATA_SECOTR_SIZE);
	
	size_t interbuffsize =  (lastsector-firstsector)*DATA_SECOTR_SIZE;
	
	uint8_t interbuf[interbuffsize];
	
	ReadSectorsLen(firstsector,interbuffsize,interbuf);
	memcpy(buf,interbuf+offset_firstsector,size);
	
	return 0;
	

}


int CUSBDVD_DATADISC::ReadSectorsLen(uint32_t startsector,uint32_t _len,uint8_t * buffer){
	
	
	size_t lastsector = startsector + (_len/DATA_SECOTR_SIZE)+1;
	size_t interbufferlen = 1+(lastsector-startsector)*DATA_SECOTR_SIZE;
	uint8_t interbuf[interbufferlen];
	
	uint32_t numreq = ((lastsector-startsector)/MAX_READ_SECTORS)+1;
	uint32_t currsecnums = lastsector-startsector;
	uint32_t buffosff = 0;
	for(uint32_t i=0;i<numreq;i++){
		uint32_t mymin =  std::min(currsecnums,MAX_READ_SECTORS);
		ReadNumSectors(startsector+(i*MAX_READ_SECTORS),mymin,&interbuf[buffosff]);
		buffosff+=mymin*DATA_SECOTR_SIZE;
		currsecnums-=mymin;
	}

	
	//ReadNumSectors(startsector,lastsector-startsector,interbuf);
	
	memcpy(buffer,interbuf,_len);
	return 0;
}

