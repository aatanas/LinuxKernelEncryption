#define __LIBRARY__
#include <unistd.h>

_syscall2(int,crypt,int,fd,int,mode)
