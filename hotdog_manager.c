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

// --- Globals ---
// for storing messages
typedef enum { HOTDOG_MSG, TERMINATE_MSG } message_type;
typedef struct { 
    message_type type; 
    int hotdog_id; 
    int maker_id; 
} hotdog_message;

// inputs
int N; // number of hotdogs to be made
int S; // number of slots in the hotdog pool
int M; // number of making machines
int P; // number of packing machines

// message queue
mqd_t mq;

// log file
FILE *log_fp;

void make_hotdog() {}

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

    return 0;
}
