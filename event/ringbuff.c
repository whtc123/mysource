#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>

#include <sys/time.h>
#include "ringbuff.h"

#define START_POS_START(buff,n)  ( (buff)->ptr +( (( (buff)->head-(buff)->ptr)+(n))%(buff)->cnt) )
//#define RBUFF_POS_TAIL(buff,n)  ( (buff)->ptr +( (( (buff)->tail-(buff)->ptr)+(n))%(buff)->cnt) )
//#define RBUFF_COPY(buff,data,n)  
#define TAIL_LEFT(buff) ( (buff)->cnt -( (buff)->tail -(buff)->ptr ) )
#define HEAD_LEFT(buff) ( (buff)->cnt -( (buff)->head -(buff)->ptr ) )

#define rbuff_get_left(buff) ( (buff)->cnt - (buff)->len )
#define rbuff_get_len(buff) (buff)->len
#define  rbuff_empty(buff) ((buff)->head == buff->tail)
#define rbuff_full(buff) ((buff)->cnt == (buff)->len)


RingBuff *rbuff_new(size_t len)
{
	RingBuff *buff = NULL;
	if(len == 0 )return NULL;
	buff=malloc(sizeof(RingBuff));
	buff->ptr=malloc(len);
	buff->head=buff->ptr;
	buff->tail=buff->ptr;
	buff->cnt=len;
	buff->len=0;
	return buff;
	//printf("buff->cnt=%u\n",buff->cnt);
}

int rbuff_free(RingBuff *buff)
{
	if(buff && buff->ptr)free(buff->ptr);
	if(buff)free(buff);
}

int rbuff_append_tail(RingBuff *buff,void *data,size_t len)
{

	size_t tail_left=0;
	if(rbuff_get_left(buff) < len ) return -1;
	tail_left=TAIL_LEFT(buff);

	if( tail_left >= len )
	{
		memcpy(buff->tail,data,len);
		buff->tail=buff->tail + len;
	}
	else
	{
		memcpy(buff->tail,data,tail_left);
		memcpy(buff->ptr,data+tail_left,len - tail_left );
		buff->tail=buff->ptr + (len - tail_left);
	}
	buff->len+=len;
	return 0;
	
}

int rbuff_get_head(RingBuff *buff,void *data,size_t len)
{

	size_t head_left=HEAD_LEFT(buff);
	
	if( rbuff_get_left(buff) < len)len=rbuff_get_left(buff);
	if( head_left > len)
	{
		memcpy(data,buff->head,len);
		buff->head+=len;
		
	}
	else
	{
		memcpy(data,buff->head,head_left);
		memcpy(data+head_left,buff->ptr,len-head_left);
		buff->head=buff->ptr+len-head_left;
	}
	buff->len-=len;
	return len;
}

int rbuff_read(RingBuff *buff,int fd,size_t len)
{
	
}

int rbuff_write(RingBuff *buff,int fd,size_t len)
{

}

int rbuff_write_align(RingBuff *buff,int fd,size_t len)
{
	rbuff_align(buff);
	rbuff_write(buff,fd,len);
}

int rbuff_align(RingBuff *buff)
{
	void *new_buff=NULL;
	if(buff->tail > buff->head )return 0;
	new_buff=malloc(buff->cnt);
	memcpy(new_buff,buff->head,HEAD_LEFT(buff) );
	
	memcpy(new_buff+HEAD_LEFT(buff)  ,  buff->ptr , buff->len - HEAD_LEFT(buff) );
	
	free(buff->ptr);
	buff->ptr=new_buff;
	buff->head=buff->ptr;
	buff->tail=buff->ptr+buff->len;
	return 0;
}



int rbuff_printf(RingBuff *buff,const char *fmt, ...)
{
	va_list ap;
	int n=0;
	char **str;
	va_start(ap, fmt);
	n = vasprintf (str, rbuff_get_left(buff), fmt, ap);
	va_end(ap);
	if(n == -1 )
		return -1;
	if(  n <= buff->cnt-buff->len )
	{
		rbuff_append_tail( buff, *str, n);
		n=-1;
	}
	free(str);
	return n;
	
}
int rbuff_get_line(RingBuff *buff,char *line,size_t len,enum eol_style style)
{
	int i=0;
	
	for(i=0;i<buff->len&&i<len;i++)
	{
		fprintf(stderr,"-------------------------i=%d\n",i);
		if(  *(char *)START_POS_START(buff,i)=='\r' &&  *(char *)START_POS_START(buff,i+1)=='\n' )
		{
			fprintf(stderr,"-22------------------------i=%d\n",i);
			rbuff_get_head(buff,line,i);
			fprintf(stderr,"-22------------------------i=%d\n",i);
			line[i]=0;
			return i;
		}
	}
	return -1;
}




void rbuff_dump(RingBuff *buff)
{
	unsigned int i;
	char c;
	size_t index;
	size_t stop;

	fprintf(stderr,"---------------rbuff_dump len:%u----------------\n",buff->len);
	
	for(i=0;i<buff->len;i++)
	{
		c=*(char *)START_POS_START(buff,i);
		printf("%c",c);
	}
	printf("\n\n");
}
void set_time_start(struct timeval *tv )
{
    gettimeofday(tv,NULL);
}

void set_time_end(struct timeval *tv)
{
    struct timeval tv_now;
    struct timeval iv; 
    gettimeofday(&tv_now,NULL);
    iv.tv_sec=tv_now.tv_sec - tv->tv_sec;
    if(tv->tv_usec > tv_now.tv_usec)
    {   
        iv.tv_sec--;
        iv.tv_usec = tv->tv_usec + 1000000 - tv_now.tv_usec;
    }   
    else  iv.tv_usec = tv_now.tv_usec - tv->tv_usec;
    printf("time %u sec, %u usec\n",iv.tv_sec,iv.tv_usec);

}


int main()
{
	struct timeval tv;
	RingBuff *buff;
	char str[128];
	char data[2000];
	char data1[2000];
	buff=rbuff_new(20);
	printf("buff->cnt=%u\n",buff->cnt);
	set_time_start(&tv);
	assert(rbuff_append_tail(buff,"123456\r\n7890",10)== 0 );

	rbuff_dump(buff);

	rbuff_get_line( buff, str ,128, EOL_CRLF);
	fprintf(stderr,"GET LINE:%s\n",str);
	rbuff_dump(buff);
	
	rbuff_get_head(buff,data,8);
	fprintf(stderr,"2------------------------------------\n");
	rbuff_dump(buff);
	 fprintf(stderr,"3------------------------------------\n");
	assert(rbuff_append_tail(buff,"1234567890abcd",14)== 0 );
	rbuff_dump(buff);
	rbuff_align( buff);
	rbuff_dump(buff);
}
