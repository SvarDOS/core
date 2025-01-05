/* Test for NULL call INT 3 interception */

typedef int (*f_ptr)(void);

int main(void)
{
	f_ptr f = 0;
	f();
	return 0;
}
