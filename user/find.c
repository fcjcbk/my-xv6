#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"


void find(char* path, char* target){

    struct stat st;
    struct dirent de;
    char buf[128];
    char* p;

    int fd = open(path, 0);
    if(fd < 0){
        fprintf(2, "find: cannot open %s\n", path);
        return;
    }

    if(fstat(fd, &st) < 0){
        close(fd);
        fprintf(2, "find: cannot stat %s\n", path);
        return;
    }

    if(st.type != T_DIR){
        close(fd);
        fprintf(2, "find: first argment should be directory, but not %s\n", path);
        return;
    }
    
    while(read(fd, &de, sizeof(de)) == sizeof(de)){
        if(de.inum == 0){
            continue;
        }
        //将获得的文件名与path拼成一个完整的字符串
        //如果该字符串是文件名，则对其进行匹配查看是否是目标，否则递归调用find
        if(strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0){
            continue;
        }
        memmove(buf, path, strlen(path));
        p = buf + strlen(path);
        *p++ = '/';
        memmove(p, de.name, strlen(de.name));
        p += strlen(de.name);
        *p = 0;

        if(stat(buf, &st) < 0){
            close(fd);
            fprintf(2, "find: cannot stat %s\n", buf);
            return;
        }
        
        if(st.type == T_DIR){
            find(buf, target);
        }else{
            if(strcmp(target, de.name) == 0){
                printf("%s\n", buf);
            }
        }
    }
    close(fd);
}

int main(int argc, char* argv[]){
    if(argc != 3){
        fprintf(2, "Usage: find dir filename");
        exit(0);
    }
    find(argv[1], argv[2]);

    exit(0);
}