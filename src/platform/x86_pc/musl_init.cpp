#include <elf.h>
#include <poll.h>
#include <fcntl.h>
#include <signal.h>
extern "C" void panic(const char*);

#ifndef LIBC_H
#define LIBC_H

#include <stdlib.h>
#include <stdio.h>
#include <limits.h>

struct __locale_map;

struct __locale_struct {
	const struct __locale_map *volatile cat[6];
};

struct tls_module {
	struct tls_module *next;
	void *image;
	size_t len, size, align, offset;
};

struct __libc {
	int can_do_threads;
	int threaded;
	int secure;
	volatile int threads_minus_1;
	size_t *auxv;
	struct tls_module *tls_head;
	size_t tls_size, tls_align, tls_cnt;
	size_t page_size;
	struct __locale_struct global_locale;
};

#ifndef PAGE_SIZE
#define PAGE_SIZE libc.page_size
#endif

#ifdef __PIC__
#define ATTR_LIBC_VISIBILITY __attribute__((visibility("hidden")))
#else
#define ATTR_LIBC_VISIBILITY
#endif

extern struct __libc __libc ATTR_LIBC_VISIBILITY;
#define libc __libc

extern size_t __hwcap ATTR_LIBC_VISIBILITY;
extern size_t __sysinfo ATTR_LIBC_VISIBILITY;
extern char *__progname, *__progname_full;

/* Designed to avoid any overhead in non-threaded processes */
void __lock(volatile int *) ATTR_LIBC_VISIBILITY;
void __unlock(volatile int *) ATTR_LIBC_VISIBILITY;
int __lockfile(FILE *) ATTR_LIBC_VISIBILITY;
void __unlockfile(FILE *) ATTR_LIBC_VISIBILITY;
#define LOCK(x) __lock(x)
#define UNLOCK(x) __unlock(x)

void __synccall(void (*)(void *), void *);
int __setxid(int, int, int, int);

extern char **__environ;

#undef weak_alias
#define weak_alias(old, new) \
	extern __typeof(old) new __attribute__((weak, alias(#old)))

#undef LFS64_2
#define LFS64_2(x, y) weak_alias(x, y)

#undef LFS64
#define LFS64(x) LFS64_2(x, x##64)

#endif


void __init_tls(size_t *);

extern "C" void _init();

__attribute__((__weak__, __visibility__("hidden")))
extern void (*const __init_array_start)(void), (*const __init_array_end)(void);

void __init_ssp(void*) {}

#define AUX_CNT 38

void hallo__init_libc(char **envp, char *pn)
{
  kprintf("__init_libc(%p, %p)\n", envp, pn);
	size_t i, *auxv, aux[AUX_CNT] = { 0 };
	__environ = envp;
	for (i=0; envp[i]; i++);
	libc.auxv = auxv = (size_t *)(envp+i+1);
  kprintf("aux: %p\n", auxv);

	for (i=0; auxv[i]; i+=2) if (auxv[i]<AUX_CNT) aux[auxv[i]] = auxv[i+1];

  kprintf("AT_PAGESZ: %lu\n", aux[AT_PAGESZ]);

	__hwcap = aux[AT_HWCAP];
	__sysinfo = aux[AT_SYSINFO];
	libc.page_size = aux[AT_PAGESZ];

	if (!pn) pn = (char*)aux[AT_EXECFN];
	if (!pn) pn = "";
	__progname = __progname_full = pn;
	for (i=0; pn[i]; i++) if (pn[i]=='/') __progname = pn+i+1;

	__init_tls(aux);
	__init_ssp((void *)aux[AT_RANDOM]);

  kprintf("AT_SECURE: %lu\n", aux[AT_SECURE]);
	if (aux[AT_UID]==aux[AT_EUID] && aux[AT_GID]==aux[AT_EGID]
		&& !aux[AT_SECURE]) return;
  kprintf("hest2\n");

	struct pollfd pfd[3] = { {.fd=0}, {.fd=1}, {.fd=2} };
	poll(pfd, 3, 0);
	for (i=0; i<3; i++) if (pfd[i].revents&POLLNVAL)
		if (open("/dev/null", O_RDWR)<0)
			panic("hmm");
	libc.secure = 1;
}

static void hallo_libc_start_init(void)
{
	_init();
	uintptr_t a = (uintptr_t)&__init_array_start;
	for (; a<(uintptr_t)&__init_array_end; a+=sizeof(void(*)()))
		(*(void (**)(void))a)();
}


int hallo__libc_start_main(int (*main)(int,char **,char **), int argc, char **argv)
{
  kprintf("__libc_start_main(%p, %d, %p, ...)\n", (void*) main, argc, argv);
	char **envp = argv+argc+1;
  kprintf("env[0] = %s\n", envp[0]);
  kprintf("envp = %p\n", argv+argc+1);

	hallo__init_libc(envp, argv[0]);

  kprintf("Calling global constructors\n");
  hallo_libc_start_init();
  kprintf("Done\n");

	/* Pass control to the application */
	exit(main(argc, argv, envp));
	return 0;
}
