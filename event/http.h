#ifndef HTTP_H
#define HTTP_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "buff.h"
#include "event.h"
#include "queue.h"

typedef enum _HttpMethod
{
	HTTP_GET,
	HTTP_POST,
	HTTP_HEAD,
	HTTP_PUT,
	HTTP_DELETE,
	HTTP_OPTIONS,
	HTTP_TRACE,
	HTTP_CONNECT,
	HTTP_PATCH,
	HTTP_UNKNOWN
}HttpMethod;

typedef enum 
{
	HTTP_ROUTE_PATH,
	HTTP_ROUTE_FUNC
}HttpRouteType;



typedef enum HttpParseStatus
{
	HPARSE_FIRST_LINE,
	HPARSE_HEAD,
	HPARSE_CONTENT,
	HPARSE_DONE,
}HttpParseStatus;


typedef struct HttpKeyVal
{
	char *key;
	char *val;
	queue_t node;
}HttpKeyVal;


typedef struct _HttpRequest
{
	HttpMethod type;
	queue_t headers;
	queue_t querys;
	//evhttp_uri uri;
	HttpParseStatus prase_status;
	Buff *buff;
	size_t content_len;
	char *content_type;

	
	/*heads*/
	char major  ;
	char minor  ;
	char *remote_host;
	char *scheme; 
	char *username;
	char *password;
	char *host;
	char *path;
	char *fragment;
	char *query;
}HttpRequest;


	
typedef struct _HttpSession
{
	int fd;
	int file_fd;
	event *ev;
	queue_t node;
	HttpRequest request;
	Buff *response;
	size_t total_read;
	struct _HttpServer *server;
}HttpSession;
typedef int (*http_request_callback)(HttpSession *session,void *arg);

typedef struct _HttpServer
{
	short port;
	http_request_callback request_callback;
	void *arg;
	int fd;
	char *www_folder;
	unsigned long timeout;
	queue_t sessions;
	queue_t routes;
	event *ev;
}HttpServer;


typedef int (*HttpRouteCallback)(HttpSession *session,void *arg);

typedef struct _HttpRoute
{
	queue_t node;
	char *path;
	HttpRouteCallback func;
	void *arg;
} HttpRoute;
HttpServer *httpserver_new(short port,http_request_callback request,void *arg);
int httpserver_set_folder(HttpServer *server ,char *folder);
void httproute_callback_add(HttpServer *server,char *path,HttpRouteCallback func,void *arg);
#endif

