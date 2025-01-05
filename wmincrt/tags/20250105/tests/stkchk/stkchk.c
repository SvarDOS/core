/* This should trigger a stack overflow error message */

int main( void )
{
	main();
	return 0;
}
