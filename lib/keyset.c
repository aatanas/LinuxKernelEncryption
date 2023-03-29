#define __LIBRARY__
#include <unistd.h>

_syscall3(int,keyset,char*,key,int,len,int,mode)
