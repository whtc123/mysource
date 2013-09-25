/**
* @file connect.h
* @brief 网络连接部分
* @author Wang Hong, hong.wang@huamaitel.com
* @version 1.0
* @date 2012-12-19
*/
#ifndef CONNECT_H_
#define CONNECT_H_
//#include "common/common.h"
#include <sys/epoll.h>

#include "queue.h"
struct event;

typedef void (*callback)(struct event *ev,int what, void *d );

typedef struct event{
	queue_t i_list;
	callback func;
	void *data;
	char type;
	unsigned int flags;
	time_t delay;
	union {
		struct timeval time;
		int fd;
	} ev;
}event;


#define EVENT_TYPE_FD 1
#define EVENT_TYPE_TV 2
#define EVENT_TYPE_ONTIME 3


void* event_loop(void *arg);

event *event_fd_new(int fd,int what,callback fun,void *clientdate);

int event_fd_setflags(event *ev ,unsigned int what);

event *event_alarm_new(unsigned long ms,callback fun,void *clientdate);

event *event_ontime_new(unsigned long ms,callback fun,void *clientdate);

event *event_trige_new(callback fun,void *clientdate);

int event_fd_del(event *ev );

void event_init();

#endif /* CONNECT_H_ */
