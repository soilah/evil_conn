



struct attack_args{
    char* ip;
    char* port;

};


//Command struct


typedef enum cmd_t{
    NORMAL,
    UNKNOWN,
    DISCONNECT
}cmd_t;


struct command{
    cmd_t type;
    char* cmd_name;
    size_t n_args;
    char *args[10];
};



