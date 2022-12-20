#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char** argv){
    int p1[2];
    int p2[2];
    pipe(p1);
    pipe(p2);
    
    
    
    if(fork() != 0){
        close(p2[1]);
        close(p1[0]);

        write(p1[1], "!", 1); // 父进程向子进程写入一个字节
        close(p1[1]);

        char buf;
        read(p2[0], &buf, 1); //等待子进程回复
        printf("%d: received pong\n", getpid()); 
        close(p2[0]);
        wait(0);
    }else{
        
        close(p2[0]);
        close(p1[1]);

        char buf;
        read(p1[0], &buf, 1);  //从父进程中读一个字节
        close(p1[0]);

        printf("%d: received ping\n", getpid()); //打印收到

        write(p2[1], &buf, 1);  //向父进程写一个字节
        close(p2[1]);
    }
    exit(0);
}

