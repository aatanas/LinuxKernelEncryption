#include <unistd.h>
#include <fcntl.h>

#define UTIL_IMPLEMENTATION
#include "utils.h"

int main(char* args){
    int fd = open(get_argv(args,1),O_RDWR);
    switch_case(fd);
    close(fd);
    _exit(0);
}
