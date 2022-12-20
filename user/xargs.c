#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#define MAX_SIZE 31
#define MAXARG 32
int main(int argc, char* argv[]){

    if(argc < 2){
        fprintf(2, "Usage: there have a operation after args\n");
        exit(0);
    }
    
    char* args[MAXARG];
    int i = 0;
    while(i < argc - 1){
        args[i] = argv[i + 1];
        i++;
    }
    char c;
    while(read(0, &c, 1) > 0 && i < MAXARG){
        int j = 0;
        args[i] = (char*)malloc(MAX_SIZE + 1);
        args[i][j++] = c;
        while(read(0, &c, 1) > 0 && j < MAX_SIZE - 1){
            if(c == '\n'){
                break;
            }
            args[i][j++] = c;
        }
        if(j > MAX_SIZE){
            fprintf(2, "xargs: arg is too long\n");
            exit(0);
        }
        args[i][j] = 0;
        i++;
    }

    int pid = fork();
    if(pid != 0){
        wait(0);
        for(int k = argc; k < i; k++){
            free(args[k]);
        }
    }else{
        exec(args[0], args);
        
    }
    exit(0);
}