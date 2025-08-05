#ifndef USBDVD_COMMON_H
#define USBDVD_COMMON_H

typedef struct{
	uint8_t toc_len_msb;
	uint8_t toc_len_lsb;
	uint8_t first_track;
	uint8_t last_track;

    }TOC_Header;

    typedef struct {
        uint8_t adr_control;
        struct {
            uint8_t bit0       : 1;
			uint8_t bit1       : 1;
            uint8_t tracktype  : 1;  
            uint8_t trackcopy  : 1;
            uint8_t trackpre   : 1;
            uint8_t unused     : 3;  
        };   
        uint8_t TNO;            
        uint8_t zero;           
        uint8_t POINT;          
        uint8_t MIN;            
        uint8_t SEC;            
        uint8_t FRAME;          
    } CD_TOC_Entry;


    typedef struct{
            TOC_Header hdr;
            CD_TOC_Entry tracks[0xff];
    }CDDVD_TOC;
	
	
#endif