#include <unistd.h>
#include <fcntl.h>

#define UTIL_IMPLEMENTATION
#include "utils.h"

int main(char* args){
    int fd = open(get_argv(args,1),O_RDONLY);
    crypt(fd,1);
    close(fd);
    _exit(0);
}
