#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#define UTIL_IMPLEMENTATION
#include "utils.h"

int r;
void rand(){
    __asm__ __volatile__ (
        "rdtsc;"
        "imull $7621, %%eax;"
        "addl $1, %%eax;"
        "movl $32768, %%ebx;"
        "xorl %%edx, %%edx;"
        "idivl %%ebx;"
        : "=d" (r)
        :
        : "%eax","%ebx"
    );
}
int main(char* args){
    char mode = get_argv(args,1)[0];
    int len,i;
    switch (mode) {
        case '1': len=4; break;
        case '2': len=8; break;
        case '3': len=16; break;
    }
    char key[len];
    for(i=0;i<len;i++){
        rand();
        key[i] = 32+r%74;
    }
    keyset(key,len,0);
    _exit(0);
}
