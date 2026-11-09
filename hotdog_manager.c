#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <mqueue.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>

// --- Globals ---
// for storing messages
typedef enum { HOTDOG_MSG, TERMINATE_MSG } message_type;
typedef struct { 
    message_type type; 
    int hotdog_id; 
    int maker_id; 
} hotdog_message;

// inputs
int N;
int S;
int M;
int P;

// message queue
mqd_t mq;

// log file
FILE *log_fp;

void make_hotdog() {}

void pack_hotdog() {}
