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
#ifndef _BUF_H
#define _BUF_H

#include <stdint.h>

struct buffer {
    char *buf;
	int NUL;
    int buflen;
};

struct buffer *buffer_new(size_t reserve);
void buffer_free(struct buffer *b);
void buffer_clear(struct buffer *b);
char *buffer_detach(struct buffer *b);
void buffer_putc(struct buffer *b, const char c);
void buffer_putstr(struct buffer *b, const char *str);
void buffer_putnstr(struct buffer *b, const char *str, size_t n);
void buffer_putint(struct buffer *b, int val);
void buffer_scatf(struct buffer *b, const char *fmt, ...);
void buffer_putlong(struct buffer *b, uint64_t val);
void buffer_putshort(struct buffer *b, short val);
uint16_t u16_from_big(unsigned char *buf);
uint32_t u32_from_big(unsigned char *buf);
uint64_t u64_from_big(unsigned char *buf);
void buffer_dump(struct buffer *b);

#endif