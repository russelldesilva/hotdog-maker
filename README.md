# Hot Dog Maker

CS205 Operating System Concepts with C (AY2026/27 Term 1), Concurrency assignment.

A multi-threaded C program that simulates a hot dog factory. **M** making machines (producers) and **P** packing machines (consumers) run as POSIX threads. They pass hot dogs through a single POSIX message queue (the "hot dog pool") that holds **S** messages. The main thread is the manager: it starts the machines, waits until all **N** hot dogs are made and packed, shuts the machines down, and appends a summary to `log.txt`.

## Build

```bash
gcc -Wall -Wextra -pthread hotdog_manager.c -o hotdog_manager -lrt
```

`-lrt` is needed for `mq_*` on older glibc. Newer glibc doesn't need it, but it does no harm.

## Run

```bash
./hotdog_manager N S M P
```

| Arg | Meaning | Constraint |
|-----|---------|------------|
| `N` | Number of hot dogs to make (order) | positive integer |
| `S` | Message queue capacity (slots) | `1 <= S < N` |
| `M` | Number of making machines | `1 <= M <= 30` |
| `P` | Number of packing machines | `1 <= P <= 30` |

Example:

```bash
./hotdog_manager 10 3 2 2
cat log.txt
```

## How it works

### Making machine (`m1` … `mM`)
1. Claims the next hot dog ID (`1..N`, shared counter, protected by a mutex).
2. Makes it: `do_work(4)`.
3. Sends it: `do_work(1)`, then `mq_send()` a `HOTDOG_MSG { hotdog_id, maker_id }`. Blocks if the queue is full.
4. Logs `m<i> puts <hotdog_id>`.
5. Stops once all N IDs have been claimed.

### Packing machine (`p1` … `pP`)
1. `mq_receive()` a message. Blocks if the queue is empty.
2. On `TERMINATE_MSG`, it exits.
3. Takes the hot dog: `do_work(1)`. Packs it: `do_work(2)`.
4. Logs `p<j> gets <hotdog_id> from m<i>`.

### Manager (main thread)
1. Parses and validates `N S M P`.
2. Opens the message queue (`mq_maxmsg = S`, `mq_msgsize = sizeof(hotdog_message)`) and writes the header to `log.txt`.
3. Creates M maker threads and P packer threads.
4. Joins all makers, then sends **P** `TERMINATE_MSG`s (one per packer). Terminations are only sent after production is finished.
5. Joins all packers, then closes and unlinks the queue.
6. Reads `log.txt` back and appends the per-machine summary.

### Message format

```c
typedef enum { HOTDOG_MSG, TERMINATE_MSG } message_type;
typedef struct {
    message_type type;
    int hotdog_id;
    int maker_id;
} hotdog_message;
```

Every message (normal and terminate) uses the same priority, so the queue is strictly FIFO. Because the terminate messages are sent after every hot dog, packers drain all real hot dogs before any of them sees a terminate.

### Synchronization

| Shared resource | Protection |
|-----------------|-----------|
| Hot dog pool (bounded buffer) | POSIX message queue: the kernel blocks on full/empty, so no busy-waiting |
| Next hot dog ID counter | `pthread_mutex_t` |
| `log.txt` writes | `pthread_mutex_t` (one record per locked `fprintf` + `fflush`) |
| Per-machine counts | Each thread owns its own slot, or counts are derived from the log at the end |

## Output (`log.txt`)

```
order:10
capacity:3
making machines:2
packing machines:2
-----
m1 puts 1
m2 puts 2
p1 gets 1 from m1
...
p2 gets 10 from m2
-----
summary:
m1 made 5
m2 made 5
p1 packed 5
p2 packed 5
```

Line order in the middle section depends on thread scheduling and will differ between runs. What should always hold:
- every ID `1..N` is `put` exactly once and `get` exactly once
- the `made` counts add up to N, and so do the `packed` counts

## Notes / gotchas

- **Queue size limit:** Linux caps `mq_maxmsg` for unprivileged users at `/proc/sys/fs/mqueue/msg_max` (default **10**). A bigger `S` makes `mq_open` fail with `EINVAL`. Check with `cat /proc/sys/fs/mqueue/msg_max`, or raise it with `sudo sysctl fs.mqueue.msg_max=<n>`.
- **Stale queues:** the queue name has to start with `/`. If the program crashes before `mq_unlink()`, the old queue may still exist. `mq_unlink()` it at startup (ignore `ENOENT`) or remove it from `/dev/mqueue/`.
- **WSL:** POSIX message queues work under WSL2. `/dev/mqueue` may need `sudo mkdir -p /dev/mqueue && sudo mount -t mqueue none /dev/mqueue` if you want to inspect queues by hand.
- `do_work(n)` is a CPU busy-loop, so the run time depends on the machine. Expect several seconds for `10 3 2 2`.

## Submission

Zip **only** `hotdog_manager.c` and the compiled `hotdog_manager`, with no subfolders. Leave this README out of the zip, because the brief says no additional files.
