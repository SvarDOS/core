#include "../../wmincrt.h"

typedef unsigned size_t;

size_t strlen( const char *text )
{
   size_t len = 0;
   while ( text[len] ) len++;
   return len;
}

char hextochar( unsigned x )
{
   x &= 0xf;
   return x + '0' + ((x > 9) ? 7 : 0);
}

void hextostr( unsigned short x, char *s )
{
   s[4] = 0;   
   s[3] = hextochar(x); x >>= 4;
   s[2] = hextochar(x); x >>= 4;
   s[1] = hextochar(x); x >>= 4;
   s[0] = hextochar(x);
   *(unsigned short *)0 = 0;
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

int main( void )
{
   unsigned short env_seg = *(unsigned short __far *)PSP_PTR(0x2c);
   char psp_str[5], env_str[5];

   hextostr( crt_psp_seg, psp_str );
   hextostr( env_seg, env_str );

   puts( "Hello, my PSP is " );
   puts( psp_str );
   puts( " and my environment is at ");
   puts( env_str );
   puts( "\r\n" );

   return 0;
}
