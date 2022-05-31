//
// Created by summage on 2022/5/30.
//
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define REDIR_2_IN(x) close(0), dup(x), close(x);

int main(int argc, char * argv[]){
    uint32 base = 2, cur, flag = 0;
    int p[2];
    pipe(p);
    printf("%d\n", base);
    if(fork() == 0){
        goto sub;
    }else {
        close(p[0]);
        for (uint32 i = 3; i < 35; i += base)
            write(p[1], &i, sizeof(uint32));
        close(p[1]); // close write end of the pipe
        goto end;
    }
sub:
    REDIR_2_IN(p[0]); // re-dir input to the prv pipe
    close(p[1]); // close the prv output
    flag = read(0, &base, sizeof(uint32)); // reset the base
    switch (flag) {
        case 0: goto end; break; // one beyond the last one
        case sizeof(uint32): printf("%d\n", base); break;
        default: flag = -1; goto end; break; // failure
    }
    pipe(p);
    if(fork() == 0) // this will cause one redundancy
        goto sub;
    while((flag = read(0, &cur, sizeof(uint32))) == sizeof(uint32)){
        if (cur % base != 0)
            write(p[1], &cur, sizeof(uint32));
    }
    close(p[1]); // close write end of the pipe
    if(flag != 0)
        flag = -1;
end:
    wait(0); // waiting for the children to exit
    exit(flag);
}
