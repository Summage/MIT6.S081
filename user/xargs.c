//
// Created by summage on 2022/5/30.
//

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char * argv[]){
    int index, flag;
    uint32 buf_size = 512;
    char buf[buf_size];
    for(int i = 1; i < argc; i++)
        argv[i-1] = argv[i];
    argv[argc-1] = buf;

    while(1){
        for(index = -1; (flag = read(0, buf+1+index, sizeof(char))) > 0 && index+1<buf_size; ){
            if(buf[++index] == '\n') break;
        }
        if(flag == 0){
            if(index > -1)
                buf[index+1] = 0;
            else
                break;
        }else {
            buf[index] = 0;
        }
        if(fork() == 0){
            exec(argv[0], argv);
            exit(0);
        }else{
            wait(0);
        }
    }
    exit(0);
}
