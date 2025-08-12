#include "usbdvd_utils.h"
#include <mutex>
#include <cstdio>
#include <cstdarg>
#include <switch.h>

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


