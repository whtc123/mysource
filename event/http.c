#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stddef.h>
#include <libgen.h>
#include <errno.h>
#include <assert.h>
#include "debug.h"
#include "http.h"

static void httpserver_session_free(HttpSession *session);

static const struct {
	const char *dot_extension;
	const char *mimetype;
} mimetypes[] = { 
	{ ".html", "text/html" },
	{ ".htm",  "text/html" },
	{ ".css",  "text/css" },
	{ ".js",   "text/javascript" },
	{ ".txt",  "text/plain" },
	{ ".jpg",  "image/jpeg" },
	{ ".jpeg", "image/jpeg" },
	{ ".png",  "image/png"},
	{ ".gif",  "image/gif" },
	{ ".ico",  "image/x-icon" },
	{ ".swf",  "application/x-shockwave-flash" },
	{ ".cab",  "application/x-shockwave-flash" },
	{ ".jar",  "application/java-archive" },
	{ ".cgi",  "application/x_www_form_urlencodede"}
};

static int http_parse_http_version(const char *version, HttpRequest*req)
{
	int major, minor;
	char ch;
	//printf("http_parse_http_version:%s\n",version);
	int n = sscanf(version, "HTTP/%d.%d%c", &major, &minor, &ch);
	if (n != 2 || major > 1) {
		TraceErr("http_parse_http_version: bad version \n");
		return (-1);
	}
	//printf("http_parse_http_version end\n");
	req->major = major;
	req->minor = minor;
	return (0);
}

static int http_add_head(HttpRequest *req,char *key,char *val)
{
	HttpKeyVal *kv=NULL;
	kv=malloc(sizeof(HttpKeyVal));
	kv->key=key;
	kv->val=val;
	queue_insert_tail(&req->headers,&kv->node);
	return 0;

}


static int http_parse_head(HttpRequest *req, char *line)
{
	char *key=NULL;
	key = strsep(&line, ":");

	if (line == NULL)
		return (-1);
	if(strcmp(key,"Content_Length")==0 )
	{
		req->content_len=atoi(line);
	}
	if(strcmp(key,"Content-Type")==0 )
	{
		req->content_type=line;
		return 0;
	}
	http_add_head(req,key,line);
	return 0;

}

static int http_parse_first_line( HttpRequest *req, char *line)
{
	char *method;
	char *uri;
	char *version;
	char *query;
	char *fragment;
	const char *hostname;
	const char *scheme;
	//printf("http_parse_first_line start\n");
	//printf("line:%s\n",line);
	/* Parse the request line */
	method = strsep(&line, " ");
	if (line == NULL)
		return (-1);
	uri = strsep(&line, " ");
	if (line == NULL)
		return (-1);
	version = strsep(&line, " ");
	if (line != NULL)
		return (-1);
	//printf("http_parse_first_line:1\n");
	query=strchr(uri,'?');
	if(query!=NULL )

	{
		*query='\0';
		query++;
		fragment=strrchr(query,'#');
	}
	else
	{
		fragment=strrchr(uri,'#');
	}
	
	if(fragment!=NULL )
	{
		*fragment='\0';
		fragment++;
	}

	//printf("http_parse_first_line 1\n");
	/* First line */
	if (strcmp(method, "GET") == 0) {
		req->type = HTTP_GET;
	} else if (strcmp(method, "POST") == 0) {
		req->type = HTTP_POST;
	} else if (strcmp(method, "HEAD") == 0) {
		req->type = HTTP_HEAD;
	} else if (strcmp(method, "PUT") == 0) {
		req->type = HTTP_PUT;
	} else if (strcmp(method, "DELETE") == 0) {
		req->type = HTTP_DELETE;
	} else if (strcmp(method, "OPTIONS") == 0) {
		req->type = HTTP_OPTIONS;
	} else if (strcmp(method, "TRACE") == 0) {
		req->type = HTTP_TRACE;
	} else if (strcmp(method, "PATCH") == 0) {
		req->type = HTTP_PATCH;
	} else {
		req->type = HTTP_UNKNOWN;
		TraceErr("bad request method:%s\n",method);
		/* No error yet; we'll give a better error later when
		 * we see that req->type is unsupported. */
	}
	//printf("http_parse_first_line 2\n");
	req->path=uri;
	req->query=query;
	req->fragment=fragment;
	if (http_parse_http_version(version, req) < 0)
		return (-1);
	//printf("http_parse_first_line 3\n");
	return (0);
}


void httpsession_write(struct event *ev,int what, void *d)
{
	HttpSession *session = (HttpSession *)d;
	Buff *buff=session->response;
	int len=0;
	if(buff_len(buff)>0 )
	{
		len=buff_write(session->fd,buff,buff_len(buff));
		if(len<0)
		{
			 if(errno == EAGAIN)
			 {
			 	return;
			 }
		}
		if(buff_len(buff)==0 )
			httpserver_session_free( session);
	}
}

void httpsession_response(HttpSession *session)
{
	event_fd_setflags(session->ev, EPOLLOUT);
	event_fd_setcallback(session->ev, httpsession_write);
}


void httpsession_file_write(struct event *ev,int what, void *d)
{
	HttpSession *session = (HttpSession *)d;
	Buff *buff=session->response;
	int file_fd=session->file_fd;
	int len=0;

	/*here is most send http head or fail with EAGAIN*/
	if(buff_len(buff)>0 )
	{
		len=buff_write(session->fd,buff,buff_len(buff));
		if(len<0)
		{
			 if(errno == EAGAIN)
			 {
			 	return;
			 }
		}
	}

	while(1)
	{
		buff_set_startpos( buff,0);
		/*read file*/
		do
		{
			len=buff_read(file_fd,buff,buff_tail_left(buff));
			if(len == 0 )
			{
				httpserver_session_free( session);
				return;
			}
			if(len == -1)
			{
				
				if(errno == EAGAIN)
				{
					continue;
				}
				TraceErr("read file fail:%s\n",strerror(errno));
				httpserver_session_free( session);
				return ;
			}
		}while(0);

		session->total_read+=len;
		/*send out*/
		len=buff_write(session->fd,buff,buff_len(buff));
		if(len<0)
		{
			/*socket buff full,try again*/
			
			if(errno == EAGAIN)
			{
				return;
			}
			/*error free session*/
			TraceErr("session write:FAIL\n");
			httpserver_session_free( session);
			break;
		}
		//printf("session write:%u\n",len);
	}
	
}


void httpsession_read(struct event *ev,int what, void *d)
{
	size_t len=0;
	size_t conten_len=0;
	char *line=NULL;
	char *ptr=NULL;
	//ptr=buff_tail_ptr()
	HttpSession *session = (HttpSession *)d;
	len=buff_read(session->fd, session->request.buff, buff_tail_left(session->request.buff));
	if(len<=0 )
		goto fail;
	while( (line=buff_get_line( session->request.buff,EOL_CRLF)) !=NULL )
	{
		if(session->request.prase_status==HPARSE_HEAD )
		{
			if( strlen(line)==0 )
			{
				session->request.prase_status=HPARSE_CONTENT;
				break;
			}
			if(http_parse_head(&session->request, line) == -1)
			{
				TraceErr("http_parse_head fail\n");
				goto fail;
			}
			continue;
		}

		if(session->request.prase_status==HPARSE_FIRST_LINE)
		{
			if(http_parse_first_line(&session->request,line)==-1)
				goto fail;
			session->request.prase_status=HPARSE_HEAD;
			continue;
		}

	}

	if(session->request.prase_status== HPARSE_CONTENT )
	{
		if(buff_len(session->request.buff) >= session->request.content_len)
		{
			session->request.prase_status = HPARSE_DONE;
			if(httpserver_route_request(session)!=200)goto fail;
			//if(session->server->request_callback(session,session->server->arg) != 0 )
			//	goto fail;
			//event_fd_setflags(session->ev,EPOLLIN|EPOLLOUT);
			//event_fd_setcallback(session->ev,httpsession_write);
			event_fd_setcallback(session->ev,httpsession_file_write);
			event_fd_setflags(session->ev,EPOLLOUT);
		}
	}
	return;	

fail:
	TraceErr("httpsession_read fail\n");
	httpserver_session_free(session);
	return;
}


static HttpSession *httpserver_session_new(HttpServer *server,int fd)
{
	HttpSession *session=NULL;
	session=malloc(sizeof(HttpSession));
	bzero(session,sizeof(HttpSession) );
	queue_init(&session->request.headers);
	session->fd=fd;
	session->server=server;
	session->request.buff=buff_new(1024,0);
	session->ev=event_fd_new( fd, EPOLLIN, httpsession_read, session);
	session->request.prase_status=HPARSE_FIRST_LINE;
	queue_insert_head(&server->sessions,&session->node);
	return session;
}

static void httpserver_session_free(HttpSession *session)
{	
	queue_remove(&session->node);
	event_fd_del(session->ev);
	close(session->fd);
	close(session->file_fd);
	if(session->request.buff)
		buff_free( session->request.buff);
	if(session->response)
		buff_free( session->response);
}




static void httpserver_accept(struct event *ev,int what, void *d)
{
	int nfp;
	HttpServer *server=(HttpServer*)d;
	HttpSession *session=NULL;
	struct sockaddr_in  c_add;
	size_t sin_size = sizeof(struct sockaddr_in);
	nfp = accept(ev->ev.fd, (struct sockaddr *)(&c_add), &sin_size);
	if(-1 == nfp)
	{
		TraceErr("accept fail !\r\n");
		return  ;
	}
	fcntl( nfp, F_SETFL, O_NONBLOCK); 
	//printf("[server]accept ok!\r\nServer start get connect from %#x : %#x\r\n",ntohl(c_add.sin_addr.s_addr),ntohs(c_add.sin_port));
	session=httpserver_session_new(server,nfp);
	event_fd_new(nfp,EPOLLIN,httpsession_read,session);
	TraceImport("[server]accept ok! Server start get connect from %#x : %#x\r\n",ntohl(c_add.sin_addr.s_addr),ntohs(c_add.sin_port));
}

int httpserver_set_folder(HttpServer *server ,char *folder)
{
	assert(server);
	assert(folder);
	server->www_folder=strdup(folder);
	return 0;
}

HttpServer *httpserver_new(short port,http_request_callback request,void *arg)
{
	int sfp ;
	//event *ev=NULL;
	struct sockaddr_in s_add;
	HttpServer *server=NULL;
	int sin_size;
	sfp = socket(AF_INET, SOCK_STREAM, 0);
	if(-1 == sfp)
	{
		TraceErr("socket fail ! \r\n");
		return  NULL;
	}
	bzero(&s_add,sizeof(struct sockaddr_in));
	s_add.sin_family=AF_INET;
	s_add.sin_addr.s_addr=htonl(INADDR_ANY);
	s_add.sin_port=htons(port);

	if(-1 == bind(sfp,(struct sockaddr *)(&s_add), sizeof(struct sockaddr)))
	{
		TraceErr("[server]bind fail !\r\n");
		return  NULL;
	}

	TraceImport("[server]bind ok !\r\n");

	if(-1 == listen(sfp,5))
	{
		TraceErr("[server]listen fail !\r\n");
		return  NULL;
	}

	TraceImport("[server]listen ok !\r\n");
	fcntl( sfp, F_SETFL, O_NONBLOCK); 
	server= malloc(sizeof(HttpServer));
	server->port=port;
	server->request_callback=request;
	server->arg=arg;
	queue_init(&server->sessions);
	queue_init(&server->routes);
	server->ev=event_fd_new(sfp,EPOLLIN,httpserver_accept,server);
	assert(server->ev);
	return server;
}



void httproute_callback_add(HttpServer *server,char *path,HttpRouteCallback func,void *arg)
{
	HttpRoute *route=NULL;
	route = malloc(sizeof(HttpRoute));
	route->path=strdup(path);
	route->func=func;
	route->arg=arg;
	queue_insert_head(&server->routes, &route->node);
}

void httproute_free(HttpRoute *route)
{
	if(route && route->path )free(route->path);
	if(route)free(route);
}


static const char *get_mime(char *bname)
{
	int i=0;
	if(bname == NULL)return NULL;
	for(i=0;i<sizeof(mimetypes)/sizeof(char *);i++ )
	{
		if(strcmp(mimetypes[i].dot_extension,bname)==0 )
			return mimetypes[i].mimetype;
	}
	return NULL;
} 

static unsigned long get_file_size(const char *filename)
{
	struct stat buf;
	if(stat(filename, &buf)<0)
	{
		return 0;
	}
	return (unsigned long)buf.st_size;
}

static int is_file_dir(const char *filename)
{
	struct stat buf;
	if(stat(filename, &buf)<0)
	{
		return -1;
	}
	 if(S_ISDIR(buf.st_mode)) 
	 	return 1;
	 
	return 0;
}

static int enum_index(char *path)
{
	struct stat buf;
	char *p_end=path+strlen(path);
	printf("enum_index in\n");
	strcat(path,"index.htm");
	if(stat(path, &buf)==0 && !S_ISDIR(buf.st_mode))
		return 0;
	*p_end='\0';

	strcat(path,"index.html");
	if(stat(path, &buf)==0 && !S_ISDIR(buf.st_mode))
		return 0;
	*p_end='\0';

	strcat(path,"index.cgi");
	if(stat(path, &buf)==0 && !S_ISDIR(buf.st_mode))
		return 0;
	*p_end='\0';
	printf("enum_index out\n");
	return -1;
	
}


int request_file(HttpSession *session,char *rpath)
{
	int fd;
	const char *mime=NULL;
	char *bname=NULL;
	char *full_fath=NULL;
	size_t path_len;
	unsigned long filesize=0;
	
	if(rpath==NULL)
	{
		path_len=strlen(session->request.path)+strlen(session->server->www_folder)+20;
		full_fath=malloc(path_len);
		sprintf(full_fath,"%s%s",session->server->www_folder,session->request.path);
	}
	else
	{
		full_fath=strdup(rpath);
	}
	//printf("mime:%s\n",mime);

	if(is_file_dir(full_fath) == 1 && enum_index(full_fath)==-1)
	{
		TraceErr("path %s has no inde.* ,return 404 !\n",full_fath);
		return 404;
	}
	TraceImport("request path:%s\n",full_fath);

	filesize=get_file_size((const char*)full_fath);
	if( filesize <= 0)
	{
		TraceErr("get_file_size %s fial,return 404 not found!\n",full_fath);
		return 404;
	}


	
	fd=open((const char *)full_fath,O_RDONLY|O_NONBLOCK);
	if(fd < 0 )
	{
		TraceErr("open %s fial,return 404 not found!\n",full_fath);
		return 404;
	}
	TraceImport("open %s OK,size:%u !\n",full_fath,filesize);
	//path_len=get_file_size(full_fath);
	bname=basename(full_fath);
	mime=get_mime(strrchr(bname,'.'));
	
	if(mime!=NULL )
	{
		session->response=buff_new(1500,0);
		buff_printf(session->response,"HTTP/1.0 200 OK\r\n");
		buff_printf(session->response,"Server:whtc123\r\n");
		buff_printf(session->response,"Content-type:%s\r\n",mime);
		buff_printf(session->response,"Content-length:%d; charset=GBK\r\n",filesize);
		buff_printf(session->response,"\r\n");
	}
	else
	{
	}
	session->file_fd=fd;
	event_fd_setcallback(session->ev,httpsession_file_write);
	free(full_fath);
	return 200;
}

int httproute_call(HttpSession *session)
{
	queue_t *route_queue=&session->server->routes;
	char *path=session->request.path;
	queue_t *q=NULL;
	HttpRoute *route;
	//printf("httproute_call  in !\n");
	if(queue_empty(route_queue))return 404;
	
	for(q = queue_prev(route_queue);
        q != queue_sentinel(route_queue);
        q = queue_last(q) ) {

        route = queue_data(q, HttpRoute, node);
		if(strcmp(path,route->path)==0 )
		{
			TraceInfo("route callback  found!\n");
			return route->func(session,route->arg);
		}
    }
	TraceInfo("route callback not found!\n");
	return 404;
}

int httpserver_route_request(HttpSession *session)
{
	if(httproute_call(session)!=200)
		return request_file(session,NULL);
	return 200;
}
