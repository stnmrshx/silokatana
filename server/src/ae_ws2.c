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
#include <string.h>
#include "../include/win32fixes.h"

typedef struct aeApiState {
    fd_set rfds, wfds;
    fd_set _rfds, _wfds;
    HANDLE vm_pipe;
} aeApiState;

static int aeApiCreate(aeEventLoop *eventLoop) {
    aeApiState *state = zmalloc(sizeof(aeApiState));
	
    if (!state) return -1;
	
    FD_ZERO(&state->rfds);
    FD_ZERO(&state->wfds);
    state->vm_pipe = INVALID_HANDLE_VALUE;
	
    eventLoop->apidata = state;
    return 0;
}

static void aeApiFree(aeEventLoop *eventLoop) {
    zfree(eventLoop->apidata);
}

static int aeApiAddEvent(aeEventLoop *eventLoop, int fd, int mask) {
    aeApiState *state = eventLoop->apidata;
	
    if (mask & AE_PIPE) {
        state->vm_pipe = (HANDLE) _get_osfhandle(fd);
    } else {
        if (mask & AE_READABLE) FD_SET((SOCKET)fd,&state->rfds);
        if (mask & AE_WRITABLE) FD_SET((SOCKET)fd,&state->wfds);
    }
    return 0;
}

static void aeApiDelEvent(aeEventLoop *eventLoop, int fd, int mask) {
    aeApiState *state = eventLoop->apidata;
	
    if (mask & AE_PIPE) {
        state->vm_pipe = INVALID_HANDLE_VALUE;
    } else {
        if (mask & AE_READABLE) FD_CLR((SOCKET)fd,&state->rfds);
        if (mask & AE_WRITABLE) FD_CLR((SOCKET)fd,&state->wfds);
    }
}

static int aeApiPoll(aeEventLoop *eventLoop, struct timeval *tvp) {
    aeApiState *state = eventLoop->apidata;
    int retval, j, numevents = 0;
    DWORD pipe_is_on=0;
	
    memcpy(&state->_rfds,&state->rfds,sizeof(fd_set));
    memcpy(&state->_wfds,&state->wfds,sizeof(fd_set));
	
    retval = select(eventLoop->maxfd+1, &state->_rfds,&state->_wfds,NULL,tvp);
	
    if (state->vm_pipe != INVALID_HANDLE_VALUE) {
        if (PeekNamedPipe(state->vm_pipe, NULL, 0, NULL, &pipe_is_on, NULL)) {
            if (pipe_is_on) retval++;
		}
    }
	
    if (retval > 0) {
        for (j = 0; j <= eventLoop->maxfd; j++) {
            int mask = 0;
            aeFileEvent *fe = &eventLoop->events[j];
			
            if (fe->mask == AE_NONE) continue;
            if (fe->mask & AE_READABLE && FD_ISSET(j,&state->_rfds))
                mask |= AE_READABLE;
            if (fe->mask & AE_WRITABLE && FD_ISSET(j,&state->_wfds))
                mask |= AE_WRITABLE;
            if (fe->mask & AE_PIPE && pipe_is_on)
                mask |= AE_READABLE;
            eventLoop->fired[numevents].fd = j;
            eventLoop->fired[numevents].mask = mask;
            numevents++;
        }
    }
    return numevents;
}

static char *aeApiName(void) {
    return "winsock2";
}