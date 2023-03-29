#define __LIBRARY__
#include <unistd.h>

_syscall2(int,keyset,char*,key,int,len)
