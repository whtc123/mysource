/* Copyright(C) HUAMAITELAll right reserved
* 
* @file connect.c
* @brief 
* @author Wang Hong, hong.wang@huamaitel.com
* @version 1.0
* @date 2012-12-17
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/statfs.h>
#include <string.h>
#include <libgen.h>
#include <assert.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <signal.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/time.h>

#include <pthread.h>
#include <stddef.h>
//#include "common/xml_parse.h"
//#include "common/debug.h"
//#include "port_device_manager.h"
#include "event.h"
#ifdef MEMWATCH
#include "memwatch.h"
#endif
#define EVENT_MAX 20
static queue_t g_fd_list  ;
static queue_t g_tv_list  ;
int epfd;
extern int g_exit_flag ;

void event_init()
{
	queue_init(&g_fd_list);
	queue_init(&g_tv_list);
	epfd=epoll_create(100);
}

void time_add( struct timeval  *tr, int msec )
{
	tr->tv_sec += msec / 1000;
	tr->tv_usec += ( msec % 1000 ) * 1000;
	if( tr->tv_usec >= 1000000 )
	{
		tr->tv_usec -= 1000000;
		++tr->tv_sec;
	}
}


int time_diff( struct timeval *tr_start, struct timeval  *tr_end )
{
	return ( ( tr_end->tv_sec - tr_start->tv_sec ) * 1000000
		+ tr_end->tv_usec - tr_start->tv_usec + 500 ) / 1000;
}

int time_ago( struct timeval *tr )
{
	struct timeval now;

	gettimeofday( &now, NULL );
	return time_diff(  &now,tr );
}


void time_now( struct timeval *tr )
{
	gettimeofday(  tr, NULL );
}

 int  time_cmp(const queue_t *q1, const queue_t *q2)
{
	struct timeval *t1=NULL;
	struct timeval *t2=NULL;
	event *ev1=NULL;
	event *ev2=NULL;
	assert(q1);
	assert(q2);
	ev1=queue_data(q1, event, i_list);
	ev2=queue_data(q2, event, i_list);
	t1=&ev1->ev.time;
	t2=&ev2->ev.time;
	
	if(t1->tv_sec > t2->tv_sec )
		return 1;
	else if(t1->tv_sec == t2->tv_sec)
	{
		if(t1->tv_usec == t2->tv_usec)return 0;
		if(t1->tv_usec > t2->tv_usec) return 1;
		return -1;
	}
	return -1;
}

void* event_loop(void *arg)
{
	struct epoll_event  events[EVENT_MAX];
	event *ev=NULL;
	queue_t *p=NULL;

	int wait_time=0;
	int nfds;
	int i;
	
	if(epfd == -1 )
	{
		//fprintf(stderr,"epoll_create fail!\n");
		return NULL;
	}
	printf("event_loop  START!\n");
	while(g_exit_flag!=1)
	{
		wait_time=-1;
		do{
			if(queue_empty(&g_tv_list) )break;
			p=queue_head(&g_tv_list);
			ev=queue_data(p,event , i_list);
			/*检查事件是否超时*/
			if( ( wait_time=time_ago(&ev->ev.time) )> 0  ) break;
			/*定时事件*/
			if(ev->type==EVENT_TYPE_ONTIME)
			{
				/*重新设置超时时间*/
				time_now(&ev->ev.time);
				time_add(&ev->ev.time,ev->delay);
				/*重新排序*/
				queue_sort(&g_tv_list ,time_cmp );
				ev->func(ev,0,ev->data);
			}
			else
			{
				queue_remove(p);
				ev->func(ev,0,ev->data);
				free(ev);
			}
		}while(1);
		
		if(wait_time==-1)wait_time=60000;
		
		nfds=epoll_wait(epfd,events,EVENT_MAX,wait_time);
		for( i = 0; i < nfds; i++)
		{  
			ev =  events[i].data.ptr;  
			if( events[i].events&EPOLLIN  )  /*READ*/
			{  
				ev->func(  ev, EPOLLIN , ev->data );
			}  
			else if(  events[i].events&EPOLLOUT )/*WRITE*/
			{
				ev->func(  ev, EPOLLOUT , ev->data );
			}  
			else if(  events[i].events&0x2000 )  /*EPOLLRDHUP is not set for this kernel version,but we can recive this event*/
			{  
				ev->func(  ev, 0x2000 , ev->data );
			}	
		}  
	}
	return NULL;
}









event *event_fd_new(int fd,int what,callback fun,void *clientdate)
{
	
	event* ev=	NULL;
	struct epoll_event epev;
	int err=0;
	//assert(con);
	ev=malloc(sizeof(event) );
	memset(&epev,0,sizeof(epev));
	epev.data.ptr=ev;
	epev.events=what;
	if((err=epoll_ctl(epfd,EPOLL_CTL_ADD, fd,&epev)) !=0)
	{
		if(err == EBADF)printf("epoll_ctl:EBADF.\n");
		if(err == EEXIST)printf("epoll_ctl:EEXIST.\n");
		if(err == EINVAL)printf("epoll_ctl:EINVAL.\n");
		if(err == ENOENT)printf("epoll_ctl:ENOENT.\n");
		if(err == ENOMEM)printf("epoll_ctl:ENOMEM.\n");
		if(err == EPERM)printf("epoll_ctl:EPERM.\n");
		free(ev);
		return NULL;
	}
	
	ev->data=clientdate;
	ev->func=fun;
	ev->type=EVENT_TYPE_FD;
	ev->flags=what;
	ev->ev.fd=fd;
	queue_insert_tail( &g_fd_list,&ev->i_list);
	return ev;
}

int event_fd_del(event *ev )
{
	//close(ev->ev.fd);
	queue_remove(&ev->i_list);
	free(ev);
	return 0;
}

int event_fd_setflags(event *ev ,unsigned int what)
{
	struct epoll_event epev;
	int err;
	ev->flags=what;	
	memset(&epev,0,sizeof(epev));
	epev.data.ptr=ev;
	epev.events=what;
	if((err=epoll_ctl(epfd,EPOLL_CTL_MOD,ev->ev.fd,&epev)) !=0)
	{
		if(err == EBADF)fprintf(stderr,"epoll_ctl:EBADF.\n");
		if(err == EEXIST)fprintf(stderr,"epoll_ctl:EEXIST.\n");
		if(err == EINVAL)fprintf(stderr,"epoll_ctl:EINVAL.\n");
		if(err == ENOENT)fprintf(stderr,"epoll_ctl:ENOENT.\n");
		if(err == ENOMEM)fprintf(stderr,"epoll_ctl:ENOMEM.\n");
		if(err == EPERM)fprintf(stderr,"epoll_ctl:EPERM.\n");
		//free(ev);
		return 1;
	}
	return 0;
}

int event_fd_setcallback(event *ev ,callback cbk)
{
	ev->func=cbk;
	return 0;
}


event *event_alarm_new(unsigned long ms,callback fun,void *clientdate)
{
	event* ev=	malloc(sizeof(event) );
	//event* tmp=NULL;
	//struct list_head *p=NULL;
	assert(ev);
	bzero(ev,sizeof(event));
	ev->data=clientdate;
	ev->func=fun;
	ev->type=EVENT_TYPE_TV;
	ev->flags=0;
	time_now(&ev->ev.time);
	time_add(&ev->ev.time,ms);
	queue_insert_tail(&g_tv_list, &ev->i_list);
	queue_sort(&g_tv_list ,time_cmp );
	return ev;
}

event *event_ontime_new(unsigned long ms,callback fun,void *clientdate)
{
	event* ev=	malloc(sizeof(event) );
	//event* tmp=NULL;
	//struct list_head *p=NULL;
	assert(ev);
	bzero(ev,sizeof(event));
	ev->data=clientdate;
	ev->func=fun;
	ev->type=EVENT_TYPE_ONTIME;
	ev->flags=0;
	ev->delay=ms;
	time_now(&ev->ev.time);
	time_add(&ev->ev.time,ms);
	queue_insert_tail(&g_tv_list, &ev->i_list);
	queue_sort(&g_tv_list ,time_cmp );
	return ev;
}

event *event_trige_new(callback fun,void *clientdate)
{
	return   event_alarm_new(0,  fun,  clientdate);
}


