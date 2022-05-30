//
// Created by summage on 2022/5/30.
//
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
int main(int argc, char * argv[]){
    if(argc < 3){
        fprintf(2, "usage: trace syscallMask cmd\n");
        exit(-1);
    }
    if(trace((int)atoi(argv[1])) < 0){
        printf("trace failed!\n");
        exit(-1);
    }
    exec(argv[2], argv+2);
    wait(0);
    exit(0);
}
