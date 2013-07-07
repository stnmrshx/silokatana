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
#ifndef _DEBUG_H
#define _DEBUG_H

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

typedef enum {LEVEL_DEBUG = 0, LEVEL_INFO, LEVEL_WARNING, LEVEL_ERROR} DEBUG_LEVEL;

void __debug(char *file, int line, DEBUG_LEVEL level, const char *fmt, ...);

#define __DEBUG(format, args...)\
	do { __debug(__FILE__, __LINE__, LEVEL_DEBUG, format, ##args); } while (0)

#define __ERROR(format, args...)\
	do { __debug(__FILE__, __LINE__, LEVEL_ERROR, format, ##args); } while (0)

#define __WARN(format, args...)\
	do { __debug(__FILE__, __LINE__, LEVEL_WARNING, format, ##args); } while (0)

#define __INFO(format, args...)\
	do { __debug(__FILE__, __LINE__, LEVEL_INFO, format, ##args); } while (0)

#define __PANIC(format, args...)\
	do { __debug(__FILE__, __LINE__, LEVEL_ERROR, format, ##args); exit(1); } while (0)

#endif