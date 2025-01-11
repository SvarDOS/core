/* This should trigger a stack overflow abort */

int main( void )
{
	main();
	return 0;
}
