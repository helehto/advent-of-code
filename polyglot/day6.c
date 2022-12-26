#include <asm/unistd_64.h>

static unsigned char buffer[65536];

static long read(int fd, void *buf, unsigned long count)
{
	long result;
	asm volatile("syscall"
		     : "=a"(result)
		     : "a"(__NR_read), "D"(fd), "S"(buf), "d"(count)
		     : "memory");
	return result;
}

static long write(int fd, void *buf, unsigned long count)
{
	long result;
	asm volatile("syscall"
		     : "=a"(result)
		     : "a"(__NR_write), "D"(fd), "S"(buf), "d"(count)
		     : "memory");
	return result;
}

static _Noreturn void exit(int status)
{
	asm volatile("syscall" :: "a"(__NR_exit), "D"(status));
	__builtin_unreachable();
}

static void print(int n)
{
	unsigned char buf[32];
	unsigned char *p = buf + sizeof(buf) - 1;

	*p = '\n';

	do {
		p--;
		*p = n % 10 + '0';
		n /= 10;
	} while (n);

	write(1, p, buf + sizeof(buf) - p);
}

static void search(unsigned char *buf, int n)
{
	unsigned long mask = 0;
	int i = 0;

	for (; __builtin_popcount(mask) != n; i++) {
		if (i>=n)
			mask ^= 1UL << (buf[i - n] - 'a');
		mask ^= 1UL << (buf[i] - 'a');
	}

	print(i);
}

void _start(void)
{
	unsigned char *buf = buffer;
	read(0, buf, sizeof(buffer));
	search(buf, 4);
	search(buf, 14);
	exit(0);
}
