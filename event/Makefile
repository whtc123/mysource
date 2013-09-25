.PHONY: all clean 
CFLAGS = -g  -Wall  -DPC_TEST_BIN  -fPIC  -fno-omit-frame-pointer
LDFLAGS = -Lcommon -lhmcommon -lpthread -levent_core -Bdynamic -ltcmalloc_and_profiler
CC=gcc
all:  event_test rtspclient event_test_http

rtspclient:rtspclient.o ringbuffer.o
event_test: base64.o queue.o event.o buff.o  parse_rtsp.o rtp_new.o mediasource.o  rtsp.o event_test.o
event_test_http: queue.o event.o buff.o http_parser.o http.o event_test_http.o
mediasource.o:mediasource.c
ringbuffer.o:ringbuffer.c
rtspclient.o:rtspclient.c
base64.o:base64.c
rtp_new.o:rtp_new.c
queue.o:queue.c
event.o:event.c
buff.o:buff.c
event_test.o:event_test.c
#http.o:http.c
#cJSON.o:cJSON.c
parse_rtsp.o:parse_rtsp.c
rtsp.o:rtsp.c
clean:
	rm -rf *.o event_test

