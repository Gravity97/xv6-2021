#include "kernel/types.h"
#include "user/user.h"

int main(int argc, char** argv)
{
    // pd1是从parent到child，pd2反之
    int pd1[2], pd2[2];

    if(pipe(pd1) < 0 || pipe(pd2) < 0){
        fprintf(2, "pingpong: pipe failed\n");
        exit(1);
    }

    char buffer = 'd';

    //如果是子进程，会返回0
    if(fork() == 0){
        close(pd1[1]);
        close(pd2[0]);

        //child读取数据
        if(read(pd1[0], &buffer, 1) != 1){
            fprintf(2, "pingpong: child read failed\n");
            exit(1);
        }

        printf("%d: received ping\n", getpid());

        //child写入数据
        if(write(pd2[1], &buffer, 1) != 1){
            fprintf(2, "pingpong: child wrote failed\n");
            exit(1);
        }

        exit(0);
    } 
    else {
        close(pd1[0]);
        close(pd2[1]);

        //parent写入数据
        if(write(pd1[1], &buffer, 1) != 1){
            fprintf(2, "pingpong: parent wrote failed\n");
            exit(1);
        }

        //parent读取数据
        if(read(pd2[0], &buffer, 1) != 1){
            fprintf(2, "pingpong: parent read failed\n");
            exit(1);
        }

        //这里用read/write即可完成进程阻塞
        printf("%d: received pong\n", getpid());
        exit(0);
    }
}