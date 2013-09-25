#ifndef RTSP_BASE_64_H
#define RTSP_BASE_64_H
#define base64_len(n) ((n+2)/3*4)
	int Base64_encode(unsigned char *source, int  sourcelen, char *target, int  targetlen);
	 int Base64_decode( const char* str, unsigned char* space, int size );

#endif

