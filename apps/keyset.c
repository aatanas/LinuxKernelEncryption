#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#define UTIL_IMPLEMENTATION
#include "utils.h"

int main(char* args){
    char key[1024];

    keyset(1,1,1);
    int len = read(0, key, 1024);
    keyset(1,1,0);

    keyset(key,--len,0);
    _exit(0);
}
