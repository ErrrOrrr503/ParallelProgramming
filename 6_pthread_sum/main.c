#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <time.h>

struct sum_part {
    size_t start;
    size_t N_for_proc;
};

void *thread_routing (void *S_part) {
    size_t start = ((struct sum_part *) S_part)->start;
    size_t N_for_proc = ((struct sum_part *) S_part)->N_for_proc;
    size_t *S = (size_t *) calloc (1, sizeof (size_t));
    for (size_t i = start; i < N_for_proc + start; i++)
        *S += i;
    pthread_exit (S);
}

int main (int argc, char *argv[]) {
    if (argc != 3) {
        printf ("usage: %s <number of threads> <N to count 1 to N>\n", argv[0]);
        return 1;
    }
    struct timespec begin, end;
    clock_gettime (CLOCK_REALTIME, &begin);

    size_t N = (size_t) atoll (argv[2]);
    size_t N_proc = (size_t) atoll (argv[1]);
    size_t N_for_proc = N / N_proc;
    size_t N_rest = N % N_proc;

    pthread_t *tids = (pthread_t *) calloc (N_proc, sizeof (pthread_t));
    pthread_attr_t *thr_attrs = (pthread_attr_t *) calloc (N_proc, sizeof (pthread_attr_t));
    struct sum_part *S_parts =  (struct sum_part *) calloc (N_proc, sizeof (struct sum_part));
    // just create necessary amount of
    for (size_t i = 0; i < N_proc; i++) {
        pthread_attr_init (&thr_attrs[i]);
        if (i < N_rest) {
            S_parts[i].start = (N_for_proc + 1) * i + 1;
            S_parts[i].N_for_proc = N_for_proc + 1;
        }
        else {
            S_parts[i].start = (N_for_proc + 1) * N_rest + N_for_proc * (i - N_rest) + 1;
            S_parts[i].N_for_proc = N_for_proc;
        }
        pthread_create (&tids[i], &thr_attrs[i], thread_routing, (void **) &S_parts[i]);
    }
    size_t *p_S_part;
    size_t S = 0;
    for (size_t i = 0; i < N_proc; i++) {
        pthread_join (tids[i], (void **) &p_S_part);
        S += *p_S_part;
        free (p_S_part);
    }
    clock_gettime (CLOCK_REALTIME, &end);
    double elapsed = end.tv_sec - begin.tv_sec + (end.tv_nsec - begin.tv_nsec) / 1000000000.0;
    printf ("S = %lu, elapsed = %lgs\n", S, elapsed);
    free (tids);
    free (thr_attrs);
    free (S_parts);
    return 0;
}