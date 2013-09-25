#ifndef BUFF_H
#define BUFF_H
#include "queue.h"

enum eol_style {
    EOL_ANY,

    EOL_CRLF,
    /** An EOL is a CR followed by an LF. */
    EOL_CRLF_STRICT,
    /** An EOL is a LF. */
    EOL_LF
};

typedef struct _Buff
{
	queue_t node;
	void *ptr;
	struct _Buff *reference;
	void *start;
	size_t pad_len;
	size_t total_len;
	int ref_cnt;
}Buff;
#define buff_tail_left(buff) ((buff)->total_len - ((buff)->start-(buff)->ptr+(buff)->pad_len ))
#define buff_head_left(buff) ((buff)->start-(buff)->ptr)
#define buff_tail_ptr(buff)  ( (buff)->start + (buff)->pad_len)

size_t buff_len(Buff *buff );
 
Buff *buff_new(size_t len,size_t obligate);
Buff *buff_mkcopy(Buff *buff);
Buff *buff_copy(Buff *buff);
int buff_add_tail(Buff *buff,void *data,size_t len);
int buff_add_head(Buff *buff,void *data,size_t len);
int buff_printf(Buff *buff,const char *fmt, ...);
void buff_set_startpos(Buff *buff,size_t pos);
int buff_read(int fd,Buff *buff ,size_t len);
int buff_free(Buff *buff);
int buff_write(int fd,Buff *buff ,size_t len);
char* buff_get_line(Buff *buff ,enum eol_style style);
#endif
