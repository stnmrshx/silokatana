/*
** Silokatana - Fast key-value store and storage engine
**
** Copyright (c) 2013, stnmrshx (stnmrshx@gmail.com)
** All rights reserved.
** Redistribution and use in source and binary forms, with or without modification, 
** are permitted provided that the following conditions are met: 
** 
** 1. Redistributions of source code must retain the above copyright notice, this
**    list of conditions and the following disclaimer. 
** 2. Redistributions in binary form must reproduce the above copyright notice,
**    this list of conditions and the following disclaimer in the documentation
**    and/or other materials provided with the distribution. 
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
** ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
** WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
** DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
** ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
** (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
** LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
** ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
** SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**
**/
#include <stdio.h>
#include <time.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include "../include/debug.h"

#define EVENT_NAME "silokatana.event"

void __debug_raw(int level, const char *msg, char *file, int line) 
{
	const char *c = ".-*#";
	time_t now = time(NULL);
	FILE *fp;
	char buf[64];

	strftime(buf, sizeof(buf),"%d %b %I:%M:%S", localtime(&now));

	if (level == LEVEL_ERROR) {
		
		fp = fopen(EVENT_NAME, "a");
		if (fp) { 
			fprintf(stderr,"[%d] %s %c %s, error:%s %s:%d\n", (int)getpid(), buf, c[level], msg, strerror(errno), file, line);
			fprintf(fp,"[%d] %s %c %s, error:%s %s:%d\n", (int)getpid(), buf, c[level], msg, strerror(errno), file, line);
			fflush(fp);
			fclose(fp);
		}
	} else
		fprintf(stderr, "[%d] %s %c %s \n", (int)getpid(), buf, c[level], msg);
}

void __debug(char *file, int line, DEBUG_LEVEL level, const char *fmt, ...)
{
	va_list ap;
	char msg[1024];
	va_start(ap, fmt);
	vsnprintf(msg, sizeof(msg), fmt, ap);
	va_end(ap);
	__debug_raw((int)level,msg, file, line);
}