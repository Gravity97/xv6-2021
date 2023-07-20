#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fs.h"
#include "user/user.h"

const char* fmtname(const char* path)
{
    const char* p = path + strlen(path);
    while (p >= path && *p != '/')
        p--;

    p++;
    return p;
}

void find(const char* path, const char* file)
{
    int fd;
    struct stat st;

    //open the dir
    if((fd = open(path, 0)) < 0){
        fprintf(2, "find: cannot open %s\n", path);
        return;
    }

    if(fstat(fd, &st) < 0){
        fprintf(2, "find: cannot stat %s\n", path);
        close(fd);
        return;
    }

    switch (st.type){
        case T_FILE:
            if(strcmp(fmtname(path), file) == 0)
                printf("%s\n", path);
            break;

        case T_DIR:
            char buffer[512];
            if (strlen(path) + 1 + DIRSIZ + 1 > sizeof(buffer)) {
                fprintf(2, "find: path too long\n");
                break;
            }

            strcpy(buffer, path);
            char* p = buffer + strlen(buffer);
            *p++ = '/';
            
            struct dirent dir;
            while(read(fd, &dir, sizeof(dir)) == sizeof(dir)){
                if(dir.inum == 0 || strcmp(dir.name, ".") == 0 || strcmp(dir.name, "..") == 0)
                    continue;
                
                memmove(p, dir.name, DIRSIZ);
                *(p + DIRSIZ) = 0;
                
                find(buffer, file);
            }

            break;
    }

    close(fd);
}

int main(int argc, char** argv)
{
    if(argc < 2){
        fprintf(2, "Usage: find path file\n");
        exit(1);
    }

    char* path = (argc == 2 ? "." : argv[1]);
    char* file = (argc == 2 ? argv[1] : argv[2]);
    find(path, file);
    exit(0);
}