#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "funcs.h"
#include <pthread.h>

int main(int argc, char** argv){
    char* port = get_args(argc,argv);
    pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
    Scan(&mtx);
    listen_to_connections(port,&mtx);
}
