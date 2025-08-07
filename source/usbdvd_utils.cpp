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
	
	//NX_IGNORE_ARG(fmt);
#endif
}


