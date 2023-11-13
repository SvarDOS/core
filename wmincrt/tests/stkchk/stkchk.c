/* This should trigger a stack overflow */

int main( void )
{
	main();
	return 0;
}
