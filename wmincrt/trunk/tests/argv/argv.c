#include "../../wmincrt.h"

typedef unsigned size_t;

size_t strlen( const char *text )
{
   size_t len = 0;
   while ( text[len] ) len++;
   return len;
}

/* pragma aux generates more efficient machine code than _asm {} blocks */
extern unsigned dos_write( unsigned handle, const char* data, size_t len );
#pragma aux dos_write = \
   "mov ah,40h" \
   "int 21h" \
   parm [bx] [dx] [cx] \
   modify [ax]


void puts( const char *text )
{
   dos_write( 1, text, strlen( text ) );
}

int main( int argc, char *argv[] )
{
	int i;
	puts( "I am " );
	puts( crt_argv[0] );
	puts( "\r\n" );
	for ( i = 1 ; i < argc ; i++ ) {
		puts( "ARG [" );
		puts( argv[i] );
		puts( "]\r\n" );
	}
	return 0;
}
