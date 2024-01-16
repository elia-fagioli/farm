#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/socket.h>
#include<sys/un.h>
#include<unistd.h>
#include<errno.h>
#include<pthread.h>
#include<stdbool.h>
#include<sys/stat.h>
#include<dirent.h>
#include<assert.h>
#include<signal.h>
#include<sys/wait.h>

//definitions
#define SOCKET_PATH "socket.sck"
#define MAX_RETRIES 5

extern bool flag_fine_put;

extern int server_fd;
extern struct sockaddr_un server_addr;
extern pthread_mutex_t mutex_socket;

typedef struct{
    char file_path[256];
    long result;
} socket_args;

typedef struct node {
    char *msg;
    struct node *next;
} Node;

typedef struct queue {
    Node *head;
    Node *tail;
    pthread_mutex_t mutex;
    pthread_cond_t cond_full;
    pthread_cond_t cond_empty;
    bool empty;
    int queue_size;
    int max_size;
} Queue;

typedef struct {
    int n_workers;
    int q_size;
    int delay;
    int n_file;
    char* file_dir;
    char **file_list;
} Options;

