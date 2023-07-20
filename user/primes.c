#include "kernel/types.h"
#include "user/user.h"

void cal_prime(uint len, uint32 array[])
{
    if(len == 0)
        return;

    uint32 prime = array[0];
    printf("prime %d\n", prime);

    int fd[2];  //file descriptor
    if(pipe(fd) < 0)
        return;

    //child proc will return 0
    if(fork() == 0){
        close(fd[1]);
        len = 0;
        char buffer[4];
        while (read(fd[0], buffer, 4) != 0){
            uint32 num = *(uint32*)buffer;
            if(num % prime != 0)
                array[len++] = num;
        }

        close(fd[0]);
        cal_prime(len, array);
    }
    else{
        close(fd[0]);
        for (uint i = 0; i < len; i++){
            write(fd[1], (char*)(array + i), 4);
        }
        close(fd[1]);
        wait(0);
    }
}

int main(int argc, char** argv)
{
    uint32 array[34];
    for (uint i = 0; i < 34; i++)
        array[i] = i + 2;

    cal_prime(34, array);
    exit(0);
}