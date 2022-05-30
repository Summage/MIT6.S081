//
// Created by summage on 2022/5/30.
//
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

void find(char * path, char * name){
    int fd;
    char buf[512];
    char *p;
    struct dirent de;
    struct stat st;

    if((fd = open(path, 0)) < 0){
        fprintf(2, "ls: cannot open %s\n", path);
        return;
    }

    if(fstat(fd, &st) < 0){
        fprintf(2, "ls: cannot stat %s\n", path);
        close(fd);
        return;
    }

    switch (st.type) {
        case T_FILE:
            for(p = path + strlen(path); p >= path && *p != '/'; p--);
            if(strcmp(++p, name) == 0) {
                printf(path);
                printf("\n");
            }
            break;
        case T_DIR:
            if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
                printf("ls: path too long\n");
                break;
            }
            strcpy(buf, path);
            p = buf + strlen(buf);
            *p++ = '/';
            while(read(fd, &de, sizeof(de)) == sizeof(de)){
                if(de.name[0] != 0 && de.name[0] != '.'){
                    memcpy(p, de.name, DIRSIZ);
                    p[strlen(de.name)] = 0;
                    find(buf, name);
                }
            }
            break;
    }
    close(fd);
}


int main(int argc, char * argv[]){
    if(argc != 3){
        fprintf(2, "usage: find path name\n");
        exit(-1);
    }
    find(argv[1], argv[2]);
    exit(0);
}