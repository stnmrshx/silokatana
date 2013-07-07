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
#ifndef _UTIL_H
#define _UTIL_H

#include <stdint.h>
#ifdef _WIN32
#include <winsock.h>
#else

#include <arpa/inet.h> 
	#ifndef O_BINARY 
		#define O_BINARY 0
	#endif
#endif

#define FILE_ERR(a) (a == -1)

#ifdef __CHECKER__
#define FORCE __attribute__((force))
#else
#define FORCE
#endif

#ifdef __CHECKER__
#define BITWISE __attribute__((bitwise))
#else
#define BITWISE
#endif

typedef uint16_t BITWISE __be16; 
typedef uint32_t BITWISE __be32; 
typedef uint64_t BITWISE __be64; 

#if CHAR_BIT != 8
#define CHAR_BIT (8)
#endif
#define SETBIT_1(bitset,i) (bitset[i / CHAR_BIT] |=  (1<<(i % CHAR_BIT)))
#define SETBIT_0(bitset,i) (bitset[i / CHAR_BIT] &=  (~(1<<(i % CHAR_BIT))))
#define GETBIT(bitset,i) (bitset[i / CHAR_BIT] &   (1<<(i % CHAR_BIT)))

static inline __be32 to_be32(uint32_t x)
{
	return (FORCE __be32) htonl(x);
}

static inline __be16 to_be16(uint16_t x)
{
	return (FORCE __be16) htons(x);
}

static inline __be64 to_be64(uint64_t x)
{
#if (BYTE_ORDER == LITTLE_ENDIAN)
	return (FORCE __be64) (((uint64_t) htonl((uint32_t) x) << 32) | htonl((uint32_t) (x >> 32)));
#else
	return (FORCE __be64) x;
#endif
}

static inline uint32_t from_be32(__be32 x)
{
	return ntohl((FORCE uint32_t) x);
}

static inline uint16_t from_be16(__be16 x)
{
	return ntohs((FORCE uint16_t) x);
}

static inline uint64_t from_be64(__be64 x)
{
#if (BYTE_ORDER == LITTLE_ENDIAN)
	return ((uint64_t) ntohl((uint32_t) (FORCE uint64_t) x) << 32) | ntohl((uint32_t) ((FORCE uint64_t) x >> 32));
#else
	return (FORCE uint64_t) x;
#endif
}

static inline int GET64_H(uint64_t x)
{
	if(((x>>63)&0x01)!=0x01)
		return 0;
	else
		return 1;
}

static inline uint64_t SET64_H_0(uint64_t x)
{
	return	x&=0x3FFFFFFFFFFFFFFF;
}

static inline uint64_t SET64_H_1(uint64_t x)
{
	return  x|=0x8000000000000000;	
}

struct slice{
	char *data;
	int len;
};

void ensure_dir_exists(const char *path);

static inline unsigned int sax_hash(const char *key)
{
	unsigned int h = 0;

	while (*key) {
		h ^= (h << 5) + (h >> 2) + (unsigned char) *key;
		++key;
	}
	return h;
}


static inline unsigned int sdbm_hash(const char *key)
{
	unsigned int h = 0;

	while (*key) {
		h = (unsigned char) *key + (h << 6) + (h << 16) - h;
		++key;
	}
	return h;
}

static inline unsigned int djb_hash(const char *key)
{
	unsigned int h = 5381;

	while (*key) {
		h = ((h<< 5) + h) + (unsigned int) *key;
		++key;
	}
	return h;
}

long long get_ustime_sec(void);
#endif