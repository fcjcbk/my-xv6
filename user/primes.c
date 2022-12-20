#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"


int main(int argc, char** argv){
    int* p1 = (int*)malloc(sizeof(int) * 2);
    pipe(p1);
    

    int pid = fork();
    if(pid != 0){
        close(p1[0]);
        printf("prime %d\n", 2);
        for(int i = 2; i <= 35; i++){
            if(i % 2 != 0){
                write(p1[1], &i, sizeof(i));
            }
        }
        close(p1[1]);
        free(p1);
        wait(0);
    }else{
        
        int* right = 0;
        int *left = p1;
        int pid1;
        do{
            //创建一个该进程右侧的管道，用于往里面写入数据
            right = (int*)malloc(sizeof(int) * 2);
            pipe(right);

            close(left[1]);
            
            //读入管道的第一个字符，直接输出
            int n;
            int res = read(left[0], &n, sizeof(n));
            if(res <= 0){      //如果管道是空的释放资源并退出
                free(left);
                free(right);
                exit(0);
            }
            printf("prime %d\n", n);  

            int t;
            //将管道中所有数字读入，如果某一个数字不能被n整除则将其写入right管道中
            while(read(left[0], &t, sizeof(t)) > 0){   
                if(t % n != 0){
                    write(right[1], &t, sizeof(t));
                }   
            }
            close(left[0]);
            free(left);
            
            //将left置为right，因为子进程的左侧管道是父进程的右侧管道
            left = right;
            pid1 = fork();
            //子进程继续循环，父进程退出循环
        }while(pid1 == 0);

        //释放资源，同时回收子进程
        close(right[0]);
        close(right[1]);
        free(right);
        wait(0);
    }
    exit(0);
}

