#ifndef AUDIOCDFS_H
#define AUDIOCDFS_H

#include <cstdlib>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string>
#include <pthread.h>
#include "usbdvd_scsi.h"
#include "usbdvd_common.h"

#define CD_SECTOR_SIZE_AUDIO 2352

typedef struct WAV_HEADER {
    uint8_t RIFF[4] = {'R', 'I', 'F', 'F'};
    uint32_t ChunkSize;                     
    uint8_t WAVE[4] = {'W', 'A', 'V', 'E'};
    uint8_t fmt[4] = {'f', 'm', 't', ' '};
    uint32_t Subchunk1Size = 16;
    uint16_t AudioFormat = 1;
    uint16_t AudioChannels = 2;
    uint32_t SamplesPerSec = 44100;
    uint32_t bytesPerSec = 44100 * 4;
    uint16_t blockAlign = 4;
    uint16_t bitsPerSample = 16;
    uint8_t Subchunk2ID[4] = {'d', 'a', 't', 'a'};
    uint32_t Subchunk2Size;
} wav_hdr;

    
class CAUDIOCD_PSEUDOFS{
public:
    CAUDIOCD_PSEUDOFS(CDDVD_TOC mytoc,CUSBSCSI * _usb_scsi_ctx);
    CAUDIOCD_PSEUDOFS(uint8_t * tocbuffer,uint32_t bufsize,CUSBSCSI * _usb_scsi_ctx);
    CAUDIOCD_PSEUDOFS(CDDVD_TOC mytoc,std::string _binfile);
    ~CAUDIOCD_PSEUDOFS();
    int audiocdfs_gettrackfilesize(int tracknum);
    int audiocdfs_gettracknums();
    int audiocdfs_File2Track(const char * filename);
    int audiocdfs_readdata(uint32_t tracknum,uint32_t pos,uint32_t size,uint8_t * buf);
    int audiocdfs_gettracklba(int tracknum);
    std::string audiocdfs_Track2Name(int tracknum);
    int audiocdfs_gettracksize(int tracknum);
        
    int audiocdfs_gettocfromfile();
        
    void createWavHeader(wav_hdr * _hdr,int tracknum);
        
    bool CDAudioFound(){
        return iscdaudio;
    }
        
private:

    CUSBSCSI *usb_scsi_ctx;
    int ReadCD_Audio_Frame(uint32_t _lba,uint8_t *buffer);    
    int ReadCD_Num_Audio_Frames(uint32_t _lba,uint16_t _num,uint8_t *buffer);   
    uint8_t lastbuffer[CD_SECTOR_SIZE_AUDIO];
    uint32_t sectornum;
    bool iscdaudio = false;
    bool isfile = false;
    
    std::string _cuefilename;
    std::string _binfilename;
    FILE * binfile_fp;
    pthread_mutex_t read_lock;      
    CDDVD_TOC toc;

};



#endif /* AUDIOCDFS_H */

