#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "event.h"
#include <sys/epoll.h>

typedef struct 
{
	queue_t head;

}mem_poll_t;



int main()
{
	event *ev=NULL;
	ev=event_fd_new(stdin,EPOLLIN,NULL,NULL);
	event_fd_setflag(ev,EPOLLOUT);
}
