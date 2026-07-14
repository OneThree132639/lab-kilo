#ifndef LOGGER_H_INCLUDED
#define LOGGER_H_INCLUDED

#include <stdio.h>
#include <stdarg.h>

#define DEBUG_LOG_FILE "kilo-debug.log"

void debug_log(const char *fmt, ...) {
	FILE *fp = fopen(DEBUG_LOG_FILE, "a"); 
	if (!fp) return; 

	va_list ap; 
	va_start(ap, fmt); 
	vfprintf(fp, fmt, ap); 
	va_end(ap); 

	fprintf(fp, "\n"); 
	fclose(fp); 
}

void debug_log_init(void) {
	FILE *fp = fopen(DEBUG_LOG_FILE, "w"); 
	if (fp) fclose(fp); 
}

#endif // LOGGER_H_INCLUDED