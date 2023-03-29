#include <unistd.h>
#include <fcntl.h>

#define UTIL_IMPLEMENTATION
#include "utils.h"

int main(char* args){
    int fd = open(get_argv(args,1),O_RDONLY);
    char* arg = get_argv(args,2);
    if(*arg=='R') keyset(0,1,0);
    crypt(fd,0);
    if(*arg=='R') keyset(0,1,0);
    close(fd);
    _exit(0);
}
