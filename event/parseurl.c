/*
1995-1996 William E. Weinman
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define false (0)
#define true  (1)

typedef struct evhttp_uri {
	unsigned flags;
	char scheme[32]; /* scheme; e.g http, ftp etc */
	char *username; /* userinfo (typically username:pass), or NULL */
	char *userpassword; /* userinfo (typically username:pass), or NULL */
	char *host; /* hostname, IP address, or NULL */
	int port; /* port, or zero */
	char *path; /* path, or "". */
	char *query; /* query, or NULL */
	char *fragment; /* fragment or NULL */
}evhttp_uri;

struct urlparts {
	char * name;
	char separator[4];
	char value[128];
} parts[] = {
	{ "scheme", ":" },
	{ "userid", "@" },
	{ "password", ":" },
	{ "host", "//" },
	{ "port", ":" },
	{ "path", "/" },
	{ "param", ";" },
	{ "query", "?" },
	{ "fragment", "#" }
};

/* for indexing the above array */
enum partnames { scheme = 0, userid, password, host, port, 
	path, param, query, fragment } ;

#define NUMPARTS (sizeof parts / sizeof (struct urlparts))

char parseError[128];

int parseURL(char *url);
char * strsplit(char * s, char * tok);
char firstpunc(char *s);
int strleft(char * s, int n);

main(int argc, char ** argv)
{
	register i;

	if(argc < 2)
	{
		printf("no command line.\n");
		exit(-1);
	}

	if(parseURL(argv[1]))
	{
		printf("parseURL: %s\n", parseError);
		exit(-1);
	}

	for(i = 0; i < NUMPARTS; i++)
		if (*parts[i].value) 
			printf("%s: %s\n", parts[i].name, parts[i].value); 

	exit(0);
}

int parseURL(char *url,evhttp_uri uri_info)
{
	register i;
	int seplen;
	char * remainder; 
	char * regall  = ":/;?#";
	char * regpath = ":;?#"; /* a path does NOT terminate with a '/' */
	char * regx;

	if(!*url)
	{
		strcpy(parseError, "nothing to do!\n");
		return -1;
	}

	if((remainder = malloc(strlen(url) + 1)) == NULL)
	{
		printf("cannot allocate memory\n");
		exit(-1);
	}

	/* don't destroy the url */
	strcpy(remainder, url);

	/* get a scheme, if any */
	if(firstpunc(remainder) == ':')
	{
		//uri_info->scheme=strsplit(remainder, parts[scheme].separator);
		strcpy(uri_info->scheme, strsplit(remainder, parts[scheme].separator));
		strleft(remainder, 1);
	}

	/* mailto hosts don't have leading "//" */
	/*
	if (!strcmp(parts[scheme].value, "mailto"))
		*parts[host].separator = 0;
	*/

	for(i = 0; i < NUMPARTS; i++)
	{
		if(!*remainder)
			break; /* nothing left to do. */
		/* skip scheme, userid, and password */
		if(i == scheme || i == userid || i == password)
			continue;

		if(i == host && strchr(remainder, '@')) /* has userid */
		{
			if(!strncmp(remainder, "//", 2))
				strleft(remainder, 2); /* lose the leading "//" */
			uri_info->username=strsplit(remainder, ":@");
			//strcpy(parts[userid].value, strsplit(remainder, ":@"));
			strleft(remainder, 1);
			if(strchr(remainder, '@'))  /* has a password too */
			{
				uri_info->password= strsplit(remainder, "@");
				//strcpy(parts[password].value, strsplit(remainder, "@"));
				strleft(remainder, 1);
			}
			*parts[host].separator = 0; /* the leading "//" is gone now */
		}

		/* don't lose the leading '/' for partial url
		   and default to scheme=http */
		if(i == path && (! *parts[scheme].value))
		{
			*parts[path].separator = 0;
			strcpy(uri_info->scheme, "http");
		}

		/* if it's the path part, use regpath */
		regx = (i == path) ? regpath : regall ;

		/* parse the part */
		seplen = strlen(parts[i].separator);
		if(strncmp(remainder, parts[i].separator, seplen))
			continue;
		else
			strleft(remainder, seplen);
		
		strcpy(parts[i].value, strsplit(remainder, regx));
	}

	if(*remainder) 
		sprintf(parseError, "I don't understand '%s'", remainder);
	free(remainder);
	return *parseError ? -1 : 0;
}

/* 
   char * strsplit(char * s; char * toks)

   returns a pointer to a string 
   representing the input string up 
   to and not including the first 
   occurrence of any character in tok.

NOTE: each character in s is tested 
against each character in tok until
one is found.

shifts the input string to begin 
with the first character of the 
matched tok string.

if no tok is found, the whole string 
is copied and s is terminated at zero 
length.

returned string is in static 
data space. 
 */ 

char * strsplit(char * s, char * tok)
{
#define OUTLEN (255)
	register i, j;
	static char out[OUTLEN + 1];

	for(i = 0; s[i] && i < OUTLEN; i++)
	{
		if(strchr(tok, s[i]))
			break;
		else
			out[i] = s[i];
	}

	out[i] = 0; /* terminate the out string */
	if(i && s[i])
	{
		for(j = 0; s[i]; i++, j++) s[j] = s[i];
		s[j] = 0;
	}
	else if (!s[i])
		*s = 0; /* we copied the whole string */

	return out;
}

/* 
   char firstpunc(char *s)

   return the first non-alphanumeric 
   in s.
 */

char firstpunc(char * s)
{
	while(*s++)
		if(!isalnum(*s)) return *s;

	return false;
}

/*
   int strleft(char * s, int n)

   shift s left n characters

   returns number of characters 
   shifted, zero for none or 
   -1 for error

   if n is less than the length
   of s, no   error is returned

   if n is equal to the length
   of s, s is truncated to zero 
   length and n is returned.
 */

int strleft(char * s, int n)
{
	int l;

	l = strlen(s);
	if(l < n)
		return -1;
	else if (l == n)
		*s = 0;

	memmove(s, s + n, l - n + 1);
	return n;
}