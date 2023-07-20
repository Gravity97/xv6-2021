#include "kernel/param.h"
#include "kernel/types.h"
#include "user/user.h"

int main(int argc, char** argv)
{
    int count = 0;
    char* array[MAXARG];
    for (int i = 1; i < argc; i++){
        array[count++] = argv[i];
    }

    char buffer, arg[MAXARG][16];
    int n_a = 0, n_l = 0;  // n_a is the seq of arg, n_l is the seq of word in an arg
    while (read(0, &buffer, 1) == 1) {
        if (buffer == '\n'){
            arg[n_a][n_l] = 0;
            array[count++] = arg[n_a];
            array[count] = 0;
            n_a = 0;
            n_l = 0;
            count = argc - 1;

            if(fork() == 0){
                exec(argv[1], array);
            }
            wait(0);
        } 
        else if (buffer == ' ') {
            arg[n_a][n_l] = 0;
            array[count++] = arg[n_a];
            n_a++;
            n_l = 0;
        } 
        else {
            arg[n_a][n_l++] = buffer;
        }
    }

    exit(0);
}