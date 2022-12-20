#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char* argv[]){
    int res;
    if(argc != 2){
        fprintf(2, "Usage: sleep times\n");
    }
    int time = atoi(argv[1]);
    res = sleep(time);
    if(res < 0){
        fprintf(2, "sleep: sleep %d fail", time);
    }
    exit(0);
}