/*  Thread compatibility glue
 *  Copyright (C) 2009 Howard Chu
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with RTMPDump; see the file COPYING.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA  02110-1301, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#ifndef __THREAD_H__
#define __THREAD_H__

#ifdef WIN32
#include <windows.h>
#include <process.h>
#define TFTYPE	void
#define TFRET()
#define THANDLE	HANDLE
#define MUTEX CRITICAL_SECTION 
#define mutex_init(b) InitializeCriticalSection(&b)
#define mutex_destroy(b) DeleteCriticalSection(&b)
#define mutex_lock(b)  EnterCriticalSection(&b)
#define mutex_unlock(b)  LeaveCriticalSection(&b)
#define THREAD_JOIN(ID)  WaitForSingleObject(ID, INFINITE) 
#define msleep(n)	Sleep(n)

#else
#include <pthread.h>
#define TFTYPE	void *
#define TFRET()	return 0
#define THANDLE pthread_t
#define MUTEX pthread_mutex_t
//!linuxÏÂÐÞ¸ÄÎªµÝ¹éËø
#define mutex_init(b) {pthread_mutexattr_t attr;int ret; \
    if ((ret = pthread_mutexattr_init(&attr)) != 0) assert(0);\
    if ((ret = pthread_mutexattr_settype(&attr,PTHREAD_MUTEX_RECURSIVE_NP)) != 0) assert(0);\
    if ((ret = pthread_mutex_init(&b, &attr)) != 0) assert(0);}
#define mutex_destroy(b) pthread_mutex_destroy(&b)
#define mutex_lock(b)  pthread_mutex_lock(&b)
#define mutex_unlock(b)  pthread_mutex_unlock(&b)
#define THREAD_JOIN(ID)  pthread_join(ID, NULL)   

#endif
typedef TFTYPE (thrfunc)(void *arg);

THANDLE ThreadCreate_rtmp(thrfunc *routine, void *args);
#endif /* __THREAD_H__ */
