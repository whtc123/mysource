#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/*
 * base64_len
 *   /brief
 *       Get the Base64 output length
 */
//#define base64_len(n) ((n+2)/3*4)
//#define base64_len(n) ((n+2)/3*4)

/**
 * characters used for Base64 encoding
 */
const char *BASE64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/**
 * encode three bytes using base64 (RFC 3548)
 *
 * @param triple three bytes that should be encoded
 * @param result buffer of four characters where the result is stored
 */
void base64_encode_triple(unsigned char triple[3], char result[4])
{
	int tripleValue, i;

	tripleValue = triple[0];
	tripleValue *= 256;
	tripleValue += triple[1];
	tripleValue *= 256;
	tripleValue += triple[2];

	for (i=0; i<4; i++)
	{
		result[3-i] = BASE64[tripleValue%64];
		tripleValue /= 64;
	}
}
/**
 * encode an array of bytes using Base64 (RFC 3548)
 *
 * @param source the source buffer
 * @param sourcelen the length of the source buffer
 * @param target the target buffer
 * @param targetlen the length of the target buffer
 * @return 1 on success, 0 otherwise
 */
int Base64_encode(unsigned char *source, int  sourcelen, char *target, int  targetlen)
{

	/* check if the result will fit in the target buffer */
	if ((sourcelen+2)/3*4 > targetlen-1)
		return 0;

	/* encode all full triples */
	while (sourcelen >= 3)
	{
		base64_encode_triple(source, target);
		sourcelen -= 3;
		source += 3;
		target += 4;
	}

	/* encode the last one or two characters */
	if (sourcelen > 0)
	{
		unsigned char temp[3];
		memset(temp, 0, sizeof(temp));
		memcpy(temp, source, sourcelen);
		base64_encode_triple(temp, target);
		target[3] = '=';
		if (sourcelen == 1)
			target[2] = '=';

		target += 4;
	}

	/* terminate the string */
	target[0] = 0;

	return 1;
}


/* Base-64 decoding.  This represents binary data as printable ASCII
 ** characters.  Three 8-bit binary bytes are turned into four 6-bit
 ** values, like so:
 **
 **   [11111111]  [22222222]  [33333333]
 **
 **   [111111] [112222] [222233] [333333]
 **
 ** Then the 6-bit values are represented using the characters "A-Za-z0-9+/".
 */

static int b64_decode_table[256] = {
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* 00-0F */
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* 10-1F */
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,62,-1,-1,-1,63,  /* 20-2F */
	52,53,54,55,56,57,58,59,60,61,-1,-1,-1,-1,-1,-1,  /* 30-3F */
	-1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,  /* 40-4F */
	15,16,17,18,19,20,21,22,23,24,25,-1,-1,-1,-1,-1,  /* 50-5F */
	-1,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,  /* 60-6F */
	41,42,43,44,45,46,47,48,49,50,51,-1,-1,-1,-1,-1,  /* 70-7F */
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* 80-8F */
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* 90-9F */
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* A0-AF */
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* B0-BF */
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* C0-CF */
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* D0-DF */
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* E0-EF */
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1   /* F0-FF */
};

/* Do base-64 decoding on a string.  Ignore any non-base64 bytes.
 ** Return the actual number of bytes generated.  The decoded size will
 ** be at most 3/4 the size of the encoded, and may be smaller if there
 ** are padding characters (blanks, newlines).
 */
int Base64_decode( const char* str, unsigned char* space, int size )
{
	const char* cp;
	int space_idx, phase;
	int d, prev_d = 0;
	unsigned char c;

	space_idx = 0;
	phase = 0;
	for ( cp = str; *cp != '\0'; ++cp )
	{
		d = b64_decode_table[(int) *cp];
		if ( d != -1 )
		{
			switch ( phase )
			{
				case 0:
					++phase;
					break;
				case 1:
					c = ( ( prev_d << 2 ) | ( ( d & 0x30 ) >> 4 ) );
					if ( space_idx < size )
						space[space_idx++] = c;
					++phase;
					break;
				case 2:
					c = ( ( ( prev_d & 0xf ) << 4 ) | ( ( d & 0x3c ) >> 2 ) );
					if ( space_idx < size )
						space[space_idx++] = c;
					++phase;
					break;
				case 3:
					c = ( ( ( prev_d & 0x03 ) << 6 ) | d );
					if ( space_idx < size )
						space[space_idx++] = c;
					phase = 0;
					break;
			}
			prev_d = d;
		}
	}
	return space_idx;
}

