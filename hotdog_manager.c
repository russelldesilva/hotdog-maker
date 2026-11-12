#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <mqueue.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#define QUEUE_NAME "/hotdog_queue"

// --- Globals ---
// for storing messages
typedef enum { HOTDOG_MSG, TERMINATE_MSG } message_type;
typedef struct { 
    message_type type; 
    int hotdog_id; 
    int maker_id; 
} hotdog_message;

// constants
const int HOTDOG_MAKING_DURATION = 4;
const int SENDING_MSG_DURATION = 1;

// inputs
int N; // number of hotdogs to be made
int S; // number of slots in the hotdog pool
int M; // number of making machines
int P; // number of packing machines

// message queue
mqd_t mq;

// mutexes 
pthread_mutex_t id_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t log_lock = PTHREAD_MUTEX_INITIALIZER;

// id of next hotdog
int next_id = 1;

// log file
FILE *log_fp;


void do_work(int n) {
    for (int i = 0; i < n; i++) {
        long m = 300000000L;
        while (m > 0) m--;
    }
}

void *make_hotdog(void *arg) {
    int maker_id = *(int *)arg;

    while (1) {
        pthread_mutex_lock(&id_lock);
        int hotdog_id = next_id++;
        pthread_mutex_unlock(&id_lock);
        
        // stop if we have made enough hotdogs
        if (hotdog_id > N) {
            break;
        }

        do_work(HOTDOG_MAKING_DURATION); // simulate making a hotdog
        do_work(SENDING_MSG_DURATION); // simulate sending the message

        hotdog_message msg = {.type = HOTDOG_MSG, .hotdog_id = hotdog_id, .maker_id = maker_id};
        mq_send(mq, (char *)&msg, sizeof(msg), 0);

        pthread_mutex_lock(&log_lock);
        fprintf(log_fp, "m%d puts %d\n", maker_id, hotdog_id);
        fflush(log_fp);
        pthread_mutex_unlock(&log_lock);
    }

    return NULL;
}

void pack_hotdog() {}

// Parses s as a base-10 integer in [min, max] and stores it in *out.
// Prints an error to stderr and returns -1 on failure, 0 on success.
static int parse_int(const char *s, const char *name, long min, long max, int *out) {
    char *end;
    errno = 0;
    long v = strtol(s, &end, 10);

    if (end == s || *end != '\0') {
        fprintf(stderr, "Invalid %s: '%s' is not an integer\n", name, s);
        return -1;
    }

    if (errno == ERANGE) {
        fprintf(stderr, "Invalid %s: '%s' is out of range\n", name, s);
        return -1;
    }

    if (v < min || v > max) {
        fprintf(stderr, "%s must be between %ld and %ld (got %ld)\n", name, min, max, v);
        return -1;
    }

    *out = (int)v;
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 5) {
        fprintf(stderr, "Usage: %s <number of hotdogs> <size of pool> <number of makers> <number of packers>\n", argv[0]);
        return 1;
    }

    // N must be parsed first: S's upper bound depends on it.
    // || short-circuits, so parsing stops at the first invalid argument.
    if (parse_int(argv[1], "N (number of hotdogs)", 1, INT_MAX, &N) ||
        parse_int(argv[2], "S (pool size)", 1, (long)N - 1, &S) ||
        parse_int(argv[3], "M (number of makers)", 1, 30, &M) ||
        parse_int(argv[4], "P (number of packers)", 1, 30, &P)) {
        return 1;
    }

    // --- Step 3: open the message queue ---
    mq_unlink(QUEUE_NAME); // clear out a leftover queue from a crashed run (error ignored)

    struct mq_attr attr;
    memset(&attr, 0, sizeof(attr));
    attr.mq_flags = 0;                         // blocking mode
    attr.mq_maxmsg = S;                        // pool capacity
    attr.mq_msgsize = sizeof(hotdog_message);  // one message = one hot dog

    mq = mq_open(QUEUE_NAME, O_CREAT | O_RDWR, 0644, &attr);
    if (mq == (mqd_t)-1) {
        perror("mq_open");
        if (errno == EINVAL) {
            fprintf(stderr, "Note: S may exceed /proc/sys/fs/mqueue/msg_max\n");
        }
        return 1;
    }

    // --- Step 4: write the log header ---
    log_fp = fopen("log.txt", "w"); // "w" wipes the previous run's log
    if (log_fp == NULL) {
        perror("fopen log.txt");
        mq_close(mq);
        mq_unlink(QUEUE_NAME);
        return 1;
    }

    fprintf(log_fp, "order:%d\n", N);
    fprintf(log_fp, "capacity:%d\n", S);
    fprintf(log_fp, "making machines:%d\n", M);
    fprintf(log_fp, "packing machines:%d\n", P);
    fprintf(log_fp, "-----\n");
    fflush(log_fp);

    // TODO Steps 5-8: start threads, shut down, write summary

    fclose(log_fp);
    mq_close(mq);
    mq_unlink(QUEUE_NAME);

    return 0;
}
