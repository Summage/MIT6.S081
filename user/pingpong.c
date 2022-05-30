//
// Created by summage on 2022/5/30.
//
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define FMT_PING "<%d>:received ping\n"
#define FMT_PONG "<%d>:received pong\n"
#define R 0
#define W 1
int main(int argc, int * argv[]){
    int p[2][2];
    pipe(p[0]); // p -> c
    pipe(p[1]); // c -> p
    char buf[2] = "x\0";
    if(fork() == 0){
        // redirection (not necessary)
        // close(0);
        // dup(p[0][R]);
        // close(1);
        // dup(p[1][W]);
        close(p[0][W]);
        close(p[1][R]);
        if(read(p[0][R], buf, 1)){
            printf(FMT_PING, getpid());
            write(p[1][W], buf, 1);
        }else{
            exit(-1);
        }
    }else{
        close(p[0][R]);
        close(p[1][W]);
        write(p[0][W], buf, 1);
        if(read(p[1][R], buf, 1)) {
            printf(FMT_PONG, getpid());
        }else{
            exit(-1);
        }
    }
    exit(0);
}