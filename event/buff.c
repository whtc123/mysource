#include <stdio.h>
#include <stdlib.h>

#define _GNU_SOURCE
#include <string.h>
#include <stdarg.h>
 #include <unistd.h>
#include <assert.h>
#include "buff.h"
#include "debug.h"
#define CHECK_COPY_WRITE(buff) do{\
									if((buff)->reference)assert(0);\
								}while(0)


Buff *buff_new(size_t len,size_t obligate)
{
	Buff * buff=NULL;
	if(obligate > len )return NULL;
	buff=malloc(sizeof(Buff));
	if(buff == NULL ) return NULL;
	bzero(buff,sizeof(Buff) );
	buff->ptr=malloc(len);
	if(buff->ptr==NULL )
	{
		free(buff);
		return NULL;
	}
	buff->start=buff->ptr+obligate;
	buff->total_len=len;
	buff->ref_cnt=1;
	return buff;
}

void buff_set_startpos(Buff *buff,size_t pos)
{
	buff->start=buff->ptr+pos;
	buff->pad_len=0;
}

int buff_free(Buff *buff)
{
	if(--buff->ref_cnt  > 0 )
		return buff->ref_cnt;
	if( buff->reference != NULL )
	{
		buff_free(buff->reference);
		buff->reference=NULL;
	}
	else
	{
		free(buff->ptr);
		buff->ptr=NULL;
		buff->start=NULL;
		
	}
	free(buff);
	return 0;
}
									
Buff *buff_mkcopy(Buff *buff)
{
	Buff * nbuff=NULL;
	//assert(buff);
	nbuff=malloc(sizeof(Buff));
	memcpy(nbuff,buff,sizeof(Buff) );
	nbuff->reference=buff;
	buff->ref_cnt++;
	nbuff->ref_cnt=1;
	return buff;
}

Buff *buff_copy(Buff *buff)
{
	Buff * nbuff=NULL;
	//assert(buff);
	nbuff=malloc(sizeof(Buff));
	memcpy(nbuff,buff,sizeof(Buff) );
	nbuff->ptr=malloc(nbuff->total_len);
	memcpy(nbuff->start,buff->start,buff->pad_len);
	return buff;
}

int buff_add_tail(Buff *buff,void *data,size_t len)
{
	int need_len=0;
	need_len =buff->start- buff->ptr + buff->pad_len + len ;
	if( need_len > buff->total_len )
	{
		return need_len;
	}
	memcpy(buff->start + buff->pad_len, data , len);
	buff->pad_len+=len;
	//buff->start+=len;
	return 0;
}
int buff_add_head(Buff *buff,void *data,size_t len)
{
	int left_len=0;
	left_len= buff->start - buff->ptr;
	if( left_len < len )
	{
		return len-left_len+buff->total_len;
	}
	memcpy(buff->start -len , data , len);
	buff->pad_len+=len;
	buff->start-=len;
	return 0;
}
int buff_printf(Buff *buff,const char *fmt, ...)
{
	int n=0;
	size_t leftlen=0;
	va_list ap;

	leftlen=buff->total_len-(buff->start-buff->ptr+buff->pad_len);
	va_start(ap, fmt);
	 n = vsnprintf (buff->start+buff->pad_len, leftlen, fmt, ap);
	va_end(ap);
	//printf("vsp ret:%d\n",n);
	if(n > -1 && n < leftlen){
		buff->pad_len+=n;
		return 0;
	}
	
	return n;
}

int buff_read(int fd,Buff *buff ,size_t len)
{
	size_t cnt=0;
	size_t left_len=0;
	void *tail=NULL;
	tail=buff_tail_ptr(buff);
	left_len=buff_tail_left(buff);
	if (left_len < len ){
		TraceErr("buff_read: len not enough!!\n");
		return -1;
	}
	cnt=read(fd,tail,len);
	if(cnt>0)
		buff->pad_len+=cnt;
	//printf("buff_read:%d\n",cnt);
	return cnt;
}

int buff_write(int fd,Buff *buff ,size_t len)
{
	size_t cnt=0;

	if( buff->pad_len == 0 )return 0;
	if(len > buff->pad_len)len=buff->pad_len;
	
	cnt=write(fd,buff->start,len);
	if(cnt>0)
	{
		buff->start+=cnt;
		buff->pad_len-=cnt;
	}
	//printf("buff_read:%s\n",buff->start);
	return cnt;
}

char* buff_get_line(Buff *buff ,enum eol_style style)
{
	char *p=NULL;
	char *str=NULL;
	//printf("getline start\n");
	p=buff->start;
	CHECK_COPY_WRITE(buff);
	switch(style)
	{
		case EOL_CRLF:
			while(p++!=buff->start+buff->pad_len)
			{
				if( *p=='\n' &&p!= buff->start && *(p-1)=='\r')
				{
					str=strndup((const char*)buff->start,(size_t)(p-(char *)(buff->start)-1));
					
					buff->pad_len-=( p-(char *)(buff->start)-2 );
					buff->start =(void *)(p+1);
					return str;
				}
			}
		case EOL_CRLF_STRICT:
		case EOL_ANY:
		case EOL_LF:
		break;
	}
	//printf("getline end\n");
	return NULL;
}

int buff_get_head(Buff *buff,void *data,size_t len)
{
	//if(len > buff->pad_len )return -1;
	CHECK_COPY_WRITE(buff);
	if(len >= buff->pad_len)
	{
		memcpy(data,buff->start,buff->pad_len);
		buff->start+=buff->pad_len;
		return buff->pad_len;
	}
	memcpy(data,buff->start,len);
	buff->start+= len;
	return len;
}

int buff_get_tail(Buff *buff,void *data,size_t len)
{
	//if(len > buff->pad_len )return -1;
	size_t left_len=0;
	CHECK_COPY_WRITE(buff);
	if(len >= buff->pad_len)
	{
		memcpy(data,buff->start,buff->pad_len);
		left_len=buff->pad_len;
		buff->pad_len=0;
		return buff->pad_len;
	}
	memcpy(data,buff->start,len);
	buff->start+=buff->pad_len;
	return len;
	
}

void buff_print(Buff *buff,char *str)
{
	char *p;
	printf("[buff bump]:%s\n",str);
	printf("start:%d padlen:%d ref:%d\n",buff->start-buff->ptr,
								buff->pad_len,buff->ref_cnt);
	for(p=(char*)(buff->start);p<(char*)(buff->start+buff->pad_len);p++)
		printf("%c",*p);
	printf("\n\n");
}

size_t buff_len(Buff *buff )
{
	return buff->pad_len;
}
