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
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "../include/buffer.h" 
unsigned _next_power(unsigned x)
{
	--x;
	x |= x >> 0x01;
	x |= x >> 0x02;
	x |= x >> 0x04;
	x |= x >> 0x08;
	x |= x >> 0x10;
	return ++x;
}

void _buffer_extendby(struct buffer *b, int len)
{
	char* buffer;

	len += b->NUL;
	if (len <= b->buflen)
		return;

	if (!b->buflen)
		b->buflen = 32;

	b->buflen = _next_power(len);
	buffer = realloc(b->buf, b->buflen);
	b->buf = buffer;
}

void _string_vprintf(struct buffer *b, const char *fmt, va_list ap)
{
	int num_required;
	
	while ((num_required = vsnprintf(b->buf + b->NUL, b->buflen - b->NUL, fmt, ap)) >= b->buflen - b->NUL)
		_buffer_extendby(b, num_required + 1);

	b->NUL += num_required;
}

struct buffer *buffer_new(size_t reserve)
{
	struct buffer *b = malloc(sizeof(struct buffer));

	b->buf = NULL;
	b->NUL = 0;
	b->buflen = 0;

	if (reserve)
		_buffer_extendby(b, reserve + 1);

	return b;
}

void buffer_free(struct buffer *b)
{
	if (b->buf)
		free(b->buf);
	free(b);
}

void buffer_clear(struct buffer *b)
{
	b->NUL = 0;

	if (b->buf)
		b->buf[b->NUL] = '\0';
}

void buffer_putstr(struct buffer *b, const char *str)
{
	size_t len = strlen(str);
	_buffer_extendby(b, len + 1);
	memcpy(&b->buf[b->NUL], str, len);
	b->NUL += len;
	b->buf[b->NUL] = '\0';
}

void buffer_putnstr(struct buffer *b, const char *str,size_t n)
{
	_buffer_extendby(b, n + 1);
	memcpy(&b->buf[b->NUL], str, n);
	b->NUL += n;
	b->buf[b->NUL] = '\0';
}

void buffer_putc(struct buffer *b, const char c)
{
	_buffer_extendby(b, 2);
	b->buf[b->NUL++] = c;
	b->buf[b->NUL] = '\0';
}

void buffer_putint(struct buffer *b, int val)
{
	_buffer_extendby(b, sizeof(int));
	b->buf[b->NUL++] = (val >> 24) & 0xff;
	b->buf[b->NUL++] = (val >> 16) & 0xff;
	b->buf[b->NUL++] = (val >> 8) & 0xff;
	b->buf[b->NUL++] = val & 0xff;
}

void buffer_putshort(struct buffer *b, short val)
{
	_buffer_extendby(b, sizeof(short));
	b->buf[b->NUL++] = (val >> 8) & 0xff;
	b->buf[b->NUL++] = val & 0xff;
}

uint16_t u16_from_big(unsigned char *buf) {
	uint16_t val = 0;

	val |= buf[2] << 8;
	val |= buf[3];
	return val;
}

uint32_t u32_from_big(unsigned char *buf) {
	uint32_t val = 0;

	val |= buf[0] << 24;
	val |= buf[1] << 16;
	val |= buf[2] << 8;
	val |= buf[3];
	return val;
}

void buffer_putlong(struct buffer *b, uint64_t val)
{
	_buffer_extendby(b, sizeof(uint64_t));
	b->buf[b->NUL++] = (val >> 56) & 0xff;
	b->buf[b->NUL++] = (val >> 48) & 0xff;
	b->buf[b->NUL++] = (val >> 40) & 0xff;
	b->buf[b->NUL++] = (val >> 32) & 0xff;
	b->buf[b->NUL++] = (val >> 24) & 0xff;
	b->buf[b->NUL++] = (val >> 16) & 0xff;
	b->buf[b->NUL++] = (val >> 8) & 0xff;
	b->buf[b->NUL++] = val & 0xff;
}

uint64_t u64_from_big(unsigned char *buf) {
	uint64_t val = 0;

	val |= (uint64_t) buf[0] << 56;
	val |= (uint64_t) buf[1] << 48;
	val |= (uint64_t) buf[2] << 40;
	val |= (uint64_t) buf[3] << 32;
	val |= buf[4] << 24;
	val |= buf[5] << 16;
	val |= buf[6] << 8;
	val |= buf[7];
	return val;
}

void buffer_scatf(struct buffer *b, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	_string_vprintf(b, fmt, ap);
	va_end(ap);
}

char * buffer_detach(struct buffer *b)
{
	char *buffer = b->buf;

	b->NUL = 0;
	return buffer;
}

void buffer_dump(struct buffer *b)
{
	int i;

	printf("--buffer dump : buffer length <%d>, pos <%d>\n", b->buflen, b->NUL);

	for (i = 0; i < b->NUL; i++)
		printf("\t[%d] %c\n", i, b->buf[i]);
}