
typedef struct
{
	size_t cnt;
	size_t len;
	void *ptr;
	void *head;
	void *tail;
}RingBuff;

enum eol_style {
    EOL_ANY,

    EOL_CRLF,
    /** An EOL is a CR followed by an LF. */
    EOL_CRLF_STRICT,
    /** An EOL is a LF. */
    EOL_LF
};
#define rbuff_get_left(buff) ( (buff)->cnt - (buff)->len )
#define rbuff_get_len(buff) (buff)->len
#define  rbuff_empty( buff) ((buff)->head == buff->tail)
#define rbuff_full( buff) ((buff)->cnt == (buff)->len)



RingBuff *rbuff_new(size_t len);
int rbuff_free(RingBuff *buff);
int rbuff_append_head(RingBuff *buff,void *data,size_t len);
int rbuff_append_tail(RingBuff *buff,void *data,size_t len);
int rbuff_get_head(RingBuff *buff,void *data,size_t len);
int rbuff_get_tail(RingBuff *buff,void *data,size_t len);
int rbuff_read(RingBuff *buff,int fd,size_t len);
int rbuff_write(RingBuff *buff,int fd,size_t len);
int rbuff_empty(RingBuff *buff);
int rbuff_full(RingBuff *buff);
int rbuff_printf(RingBuff *buff,const char *fmt, ...);
int rbuff_get_line(RingBuff *buff,char *line,size_t len,enum eol_style style);
