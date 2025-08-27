#include "usbdvd_utils.h"
#include <mutex>
#include <cstdio>
#include <cstdarg>
#include <switch.h>
#include <string.h>
#include <vector>

static std::mutex log_mutex;

void usbdvd_log(const char *fmt, ...){

#ifdef DEBUG
	auto lk = std::scoped_lock(log_mutex);
	char outbuff[1024];
	va_list arglist;
	va_start( arglist, fmt );
	vprintf( fmt, arglist );
	std::vsnprintf(outbuff, sizeof outbuff, fmt, arglist);
	va_end( arglist );
	fflush(stdout);
#else


#endif
}

uint32_t byte2u32_le(uint8_t * ptr) {

  return (static_cast<uint32_t>(ptr[0])) |
         (static_cast<uint32_t>(ptr[1]) << 8) |
         (static_cast<uint32_t>(ptr[2]) << 16) |
         (static_cast<uint32_t>(ptr[3]) << 24);
}

uint32_t byte2u32_be(uint8_t * ptr) {

  return (static_cast<uint32_t>(ptr[3])) |
         (static_cast<uint32_t>(ptr[2]) << 8) |
         (static_cast<uint32_t>(ptr[1]) << 16) |
         (static_cast<uint32_t>(ptr[0]) << 24);
}


uint16_t byte2u16_le(uint8_t * ptr) {

  return (static_cast<uint32_t>(ptr[0])) |
         (static_cast<uint32_t>(ptr[1]) << 8);
}

uint16_t byte2u16_be(uint8_t * ptr) {

  return (static_cast<uint32_t>(ptr[1])) |
         (static_cast<uint32_t>(ptr[0]) << 8);

}

struct CueTrack {
    int number;
	std::string title;
    std::string performer;
    uint8_t minutes;
    uint8_t seconds;
    uint8_t frames;
    CueTrack() : number(0), minutes(0), seconds(0), frames(0) {}
};

void lbaToMsf(uint32_t lba, uint8_t* minutes, uint8_t* seconds, uint8_t* frames) {

	long totalFrames = lba;
    *minutes = totalFrames / (75 * 60);
	totalFrames %= (75 * 60);
    *seconds = totalFrames / 75;
	*frames = totalFrames % 75;
}



bool cuebin_to_TOC(std::string _cuepath,std::string _binpath,CDDVD_TOC * _toc){

	std::vector<CueTrack> tracks;
	FILE *binfp = fopen(_binpath.c_str(), "rb");
	if(!binfp)return false;
	fseek(binfp, 0L, SEEK_END);
	size_t sz = ftell(binfp);
	fseek(binfp, 0L, SEEK_SET);
	fclose(binfp);

	FILE* file = fopen(_cuepath.c_str(), "r");
        if (!file) {
            printf("Impossibile aprire il file %s\r\n",_cuepath.c_str());
			return false;
        }


        char line[1024];
        CueTrack currentTrack;
        bool inTrack = false;

        while (fgets(line, sizeof(line), file)) {
            // Rimuovi il carattere di newline alla fine
            size_t len = strlen(line);
            if (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r')) {
                line[len-1] = '\0';
                if (len > 1 && line[len-2] == '\r') {
                    line[len-2] = '\0';
                }
            }

            // Rimuovi spazi all'inizio
            char* start = line;
            while (*start == ' ' || *start == '\t') {
                start++;
            }

            // Salta righe vuote
            if (*start == '\0') continue;

            // Parse del comando
            char command[32];
            if (sscanf(start, "%31s", command) != 1) continue;

            if (strcmp(command, "TRACK") == 0) {
                if (inTrack) {
                    tracks.push_back(currentTrack);
                }

                int trackNum;
                char trackType[32];
                if (sscanf(start, "TRACK %d %31s", &trackNum, trackType) == 2) {
                    currentTrack = CueTrack();
                    currentTrack.number = trackNum;
                    inTrack = true;
                }
            }
            else if (strcmp(command, "TITLE") == 0 && inTrack) {
                char* titleStart = strstr(start, "TITLE");
                if (titleStart) {
                    titleStart += 5; // Salta "TITLE"
                    while (*titleStart == ' ' || *titleStart == '\t') titleStart++; // Salta spazi

                    // Rimuovi virgolette se presenti
                    if (*titleStart == '"') {
                        titleStart++;
                        char* titleEnd = strrchr(titleStart, '"');
                        if (titleEnd) *titleEnd = '\0';
                    }
                    currentTrack.title = std::string(titleStart);
                }
            }
            else if (strcmp(command, "PERFORMER") == 0 && inTrack) {
                char* performerStart = strstr(start, "PERFORMER");
                if (performerStart) {
                    performerStart += 9; // Salta "PERFORMER"
                    while (*performerStart == ' ' || *performerStart == '\t') performerStart++; // Salta spazi

                    // Rimuovi virgolette se presenti
                    if (*performerStart == '"') {
                        performerStart++;
                        char* performerEnd = strrchr(performerStart, '"');
                        if (performerEnd) *performerEnd = '\0';
                    }
                    currentTrack.performer = std::string(performerStart);
                }
            }
            else if (strcmp(command, "INDEX") == 0) {
                int indexNum;
                int minutes, seconds, frames;
                if (sscanf(start, "INDEX %d %d:%d:%d", &indexNum, &minutes, &seconds, &frames) == 4) {
                    // Solitamente INDEX 01 è l'inizio della traccia
                    if (indexNum == 1 && inTrack) {
                        currentTrack.minutes = minutes;
                        currentTrack.seconds = seconds;
                        currentTrack.frames = frames;
                    }
                }
            }
        }

        // Aggiungi l'ultima traccia
        if (inTrack) {
            tracks.push_back(currentTrack);
        }
		fclose(file);
		CueTrack leadoutTrack;
		leadoutTrack.number = 0xaa;
		lbaToMsf(sz/2352,&leadoutTrack.minutes,&leadoutTrack.seconds,&leadoutTrack.frames);
        tracks.push_back(leadoutTrack);


		_toc->hdr.first_track = 1;
		_toc->hdr.last_track = tracks.size()-1;
		for(unsigned int i=0;i<tracks.size();i++){
			_toc->tracks[i].tracktype = 0;
			_toc->tracks[i].TNO = tracks[i].number;
			_toc->tracks[i].MIN = tracks[i].minutes;
			_toc->tracks[i].SEC = tracks[i].seconds;
			_toc->tracks[i].FRAME = tracks[i].frames + 150;

		}


        return true;

}

std::string trim_left(const std::string& str)
{
  const std::string pattern = " \f\n\r\t\v";
  return str.substr(str.find_first_not_of(pattern));
}

//
//Right trim
//
std::string trim_right(const std::string& str)
{
  const std::string pattern = " \f\n\r\t\v";
  return str.substr(0,str.find_last_not_of(pattern) + 1);
}

//
//Left and Right trim
//
std::string trim(const std::string& str)
{
  return trim_left(trim_right(str));
}
