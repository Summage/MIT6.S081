//
// Created by summage on 2022/5/30.
//
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int sleep(int);

int main(int argc, char *argv[]){
    if(argc != 2 || argv[1][0] < '0' || argv[1][0] > '9'){
        fprintf(2, "usage: sleep n ticks.\n");
        exit(1);
    }
    int time = atoi(argv[1]);
    sleep(time);
    exit(0);
}