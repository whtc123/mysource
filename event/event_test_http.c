#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
 #include <time.h>
#include "buff.h"
#include "event.h"
#include "http.h"
#include "rtsp.h"
#include <signal.h>  
int g_exit_flag=0;

#if 0
void cbk(struct event *ev,int what, void *d )
{
	char utctimestr[128];
	time_t timep;
	struct tm *p;
	time(&timep);
	p = gmtime(&timep);
	sprintf(utctimestr,"%d-%02d-%02dT%02d:%02d:%02dZ",
	  	(1900+p->tm_year), 
	  	(1+p->tm_mon), 
	  	p->tm_mday,
	  	p->tm_hour,
	  	p->tm_min, 
	  	p->tm_sec);

	printf ( "[alarm] callback  :%s\n" ,utctimestr );
	//ev=event_alarm_new(5000,cbk,"current");
	//testSocketClient();
}
void send_bk(struct event *ev,int what, void *d )
{
	char *buff =(char*)d;
	int n=0;
	char *html= "HTTP//1.1 200 OK\r\n"\
				"Content-Type: text/html\r\n"\
				"\r\n\r\n"\
				"<html>\r\n"\
				"<head><title>Test Page</title></head>\n"\
				"<body>\n"\
					"<p>Test OK</p>\n"\
					"<img src='mypic.jpg'>\n"\
				"</body>\n"\
				"</html>";
	if((n=write(ev->ev.fd,html,strlen(html)+1))>0 )
	{
		printf("send %d\n",n);
	}
	printf("send %d byte\n",n);
	//close(ev->ev.fd);
	//event_fd_del(ev);
}

void recv_bk(struct event *ev,int what, void *d )
{
	char buff[512]="";
	char ip[256]="";
	int n=0;
	int len=0;
	int i=0;
	struct sockaddr_in peeraddr;
	
	getpeername(ev->ev.fd, (struct sockaddr *)&peeraddr, &len);
	 inet_ntop(AF_INET, &peeraddr.sin_addr, ip, sizeof(ip));
	printf("recving \n");
	if((n=read(ev->ev.fd,buff,512 ) )>0 )
	{
		printf("[%s] recv %d\n",ip,n);
		buff[n]=0;
		//event_fd_new(ev->ev.fd,EPOLLOUT,send_bk,"--------------\n");
		event_fd_setflags( ev,EPOLLOUT);
	}
	else
	{
		printf("[%s] closed\n",ip );
		close(ev->ev.fd);
		event_fd_del(ev);
		
	}
	
	printf("%s\n",buff);
}




void connect_bk(struct event *ev,int what, void *d)
{
	struct sockaddr_in s_add;
	int slavefd=-1;
	printf("[client]connect ok !\r\n");
	//event_fd_del(slavefd,EPOLLIN,recv_bk,"ok");
	event_fd_setflags(ev,EPOLLOUT);
	event_fd_setcallback(ev,send_bk);
}

void accept_bk(struct event *ev,int what, void *d)
{
	int nfp;
	struct sockaddr_in  c_add;
	size_t sin_size = sizeof(struct sockaddr_in);
	nfp = accept(ev->ev.fd, (struct sockaddr *)(&c_add), &sin_size);
	if(-1 == nfp)
	{
		printf("accept fail !\r\n");
		return  ;
	}
	fcntl( nfp, F_SETFL, O_NONBLOCK); 
	printf("[server]accept ok!\r\nServer start get connect from %#x : %#x\r\n",ntohl(c_add.sin_addr.s_addr),ntohs(c_add.sin_port));
	event_fd_new(nfp,EPOLLIN,recv_bk,"none");
}

void testSocketServer()
{
	int sfp ;
	struct sockaddr_in s_add;
	int sin_size;
	sfp = socket(AF_INET, SOCK_STREAM, 0);
	if(-1 == sfp)
	{
	    printf("socket fail ! \r\n");
	    return  ;
	}
	//printf("socket ok !\r\n");

	bzero(&s_add,sizeof(struct sockaddr_in));
	s_add.sin_family=AF_INET;
	s_add.sin_addr.s_addr=htonl(INADDR_ANY);
	s_add.sin_port=htons(8088);

	if(-1 == bind(sfp,(struct sockaddr *)(&s_add), sizeof(struct sockaddr)))
	{
	    printf("[server]bind fail !\r\n");
	    return  ;
	}
	printf("[server]bind ok !\r\n");

	if(-1 == listen(sfp,5))
	{
	    printf("[server]listen fail !\r\n");
	    return  ;
	}
	printf("[server]listen ok !\r\n");
	fcntl( sfp, F_SETFL, O_NONBLOCK); 
	event_fd_new(sfp,EPOLLIN,accept_bk,"none");
}

void testSocketClient()
{
	int cfd;
	int recbytes;
	int sin_size;
	struct sockaddr_in s_add,c_add;

	cfd = socket(AF_INET, SOCK_STREAM, 0);
	if(-1 == cfd)
	{
	    printf("socket fail ! \r\n");
	    return  ;
	}
	printf("socket ok !\r\n");

	bzero(&s_add,sizeof(struct sockaddr_in));
	s_add.sin_family=AF_INET;
	s_add.sin_addr.s_addr= inet_addr("127.0.0.1");
	s_add.sin_port=htons(8088);
	fcntl( cfd, F_SETFL, O_NONBLOCK); 
	 connect(cfd,(struct sockaddr *)(&s_add), sizeof(struct sockaddr)) ;
	//{
	//    printf("connect fail !\r\n");
	    //return;
	//}
	
	event_fd_new(cfd,  EPOLLOUT,connect_bk,"wahaha");
}

static void readfile_callback (struct event *ev,int what, void *d)
{
	size_t len=0;
	char buff[1024]="abcdefg\n1234567890\n";
	printf("-----------------\n");
	if( (len=write(ev->ev.fd,buff,strlen(buff)))==-1)
	{
		printf("read file fail\n");
		event_fd_del(ev);
	}
	else if( (len=read(ev->ev.fd,buff,1024))==0)
	{
		printf("read file finish\n");
		event_fd_del(ev);
	}
	else printf("read %u\n",len);
	
}
#endif
int  request_callback (HttpSession *session,void *arg)
{
	static i=0;
	printf("session call back\n");
	if(i==0 )
	{
		i=1;
		return  request_file( session,"/opt/event/www/1.jpg");
	}
	else
	{
		i=0;
		return request_file( session,"/opt/event/www/2.jpg");
	}
	
	//return 200;
} 

int main()
{
	event_init();
	HttpServer *server=httpserver_new(8088,request_callback,NULL);
	if(server == NULL )exit(1);
	httpserver_set_folder( server,"/opt/nfshost/event/www");
	httproute_callback_add(server,"/servlet/MonitorChartServlet",request_callback,NULL);
	event_loop(NULL);
}
#if 0

	/*
	

	Buff *buff=NULL;
	Buff *buff1=NULL;
	buff=buff_new(256,100);
	buff1=buff_new(256,100);
	buff_free( buff);
	buff_free( buff1);
	buff_free( buff1);
	*/

/*
	event *ev;
	event_init();
	ev=event_ontime_new(5000,cbk,"task 5");
	testSocketServer();
	event_loop(NULL);
	*/
	/*
	char *p=NULL;
	Buff *buff=NULL;
	Buff *buff1=NULL;
	buff=buff_new(256,100);
	buff_printf( buff, "%s","hello ");
	buff_printf( buff, "%s","world\r\n");
	buff_printf( buff, "%s","hello");
	buff_print(buff,"after printf");

	p=buff_get_line( buff, EOL_CRLF);
	printf("line:%s\n",p);
	buff_print(buff,"after get line");

	p=buff_get_line( buff, EOL_CRLF);
	printf("line:%s\n",p? p:"(null)");
	buff_print(buff,"after get line");

	buff_add_head( buff, "head ", 5);
	buff_print(buff,"after add head");

	buff_add_tail( buff, " tail", 5);
	buff_print(buff,"after add tail");

	buff1=buff_copy(buff);
	buff_print(buff1,"buff1");
	*/
o
}
#endif
