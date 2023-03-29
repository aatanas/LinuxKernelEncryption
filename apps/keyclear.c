#include <unistd.h>
#include <fcntl.h>

#define UTIL_IMPLEMENTATION
#include "utils.h"

int main(char* args){
    keyset(0,0,0);
    _exit(0);
}
