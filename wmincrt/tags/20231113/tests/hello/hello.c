typedef unsigned size_t;

size_t strlen( const char *text )
{
	size_t len = 0;
	while (text[len]) len++;
	return len;
}

/* pragma aus generates more efficient machine code than _asm {} blocks */
extern unsigned dos_f40h(unsigned handle, const char* data, size_t len);
#pragma aux dos_f40h = \
	"mov ah,40h" \
	"int 21h" \
	parm [bx] [dx] [cx] \
	modify [ax]


void puts( const char *text )
{
	dos_f40h(1, text, strlen( text ));
}

int main( void )
{
	puts( "Hello, World\n" );
	return 0;
}
