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
#ifndef _REQUEST_H
#define _REQUEST_H

#define REQ_MAX_BUFFSIZE (10240)

typedef enum {CMD_PING, 
	CMD_GET, 
	CMD_MGET,
	CMD_SET, 
	CMD_MSET, 
	CMD_DEL, 
	CMD_INFO, 
	CMD_EXISTS, 
	CMD_SHUTDOWN, 
	CMD_UNKNOW} CMD;

struct cmds {
	char method[256];
	CMD cmd;
};

struct term {
	int len;
	char *data;
};

struct request {
	char querybuf[REQ_MAX_BUFFSIZE];
	int argc;
	struct term **argv;
	int pos;
	int multilen;
	int lastlen;
	int len;
	int idx;
	CMD cmd;
};

struct request *request_new();
int  request_parse(struct request *request);
int  request_append(struct request *request, const char *buf, int n);
void request_clean(struct request *request);
void request_free_value(struct request *request);
void request_free(struct request *request);
void request_dump(struct request *request);

#endif
