#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <time.h>
#include <semaphore.h>

// using middle value at each step.

struct sum_part {
    double start;
    double end;
    double h; // integration step. In fact no more that h, [start, end] should not be aligned by h. last step will be corrected accordingly.
    size_t thread_num;
    double *S_total;
    sem_t semaphore; // semaphore to add part-result to the result.
};

double F (double x) {
    return x * x;
}

void *thread_routine (void *S_part) {
    struct timespec t_begin, t_end;
    clock_gettime (CLOCK_REALTIME, &t_begin);

    // read params
    double start = ((struct sum_part *) S_part)->start;
    double end = ((struct sum_part *) S_part)->end;
    double h = ((struct sum_part *) S_part)->h;
    size_t thread_num = ((struct sum_part *) S_part)->thread_num;
    double *S_total = ((struct sum_part *) S_part)->S_total;
    sem_t sem =((struct sum_part *) S_part)->semaphore;

    // calculate thread's part
    double S = 0;
    size_t h_steps = (size_t) ((end - start) / h); // honest steps.
    double last_h = (end - start) - h * h_steps; // last step that is no more than h.
    for (size_t i = 0; i < h_steps; i++)
        S += (F (start + i * h) + F (start + (i + 1) * h)) / 2 * h;
    S += (F (end - last_h) + F(end)) / 2 * last_h;

    // wait until nobody is accessing shared variable and add thread's part
    sem_wait (&sem);
        *S_total += S;
    sem_post (&sem);

    clock_gettime (CLOCK_REALTIME, &t_end);
    double elapsed = t_end.tv_sec - t_begin.tv_sec + (t_end.tv_nsec - t_begin.tv_nsec) / 1000000000.0;
    printf ("Thread %lu finished, %lgs elapsed\n", thread_num, elapsed);
    pthread_exit (0);
}

int main (int argc, char *argv[]) {
    if (argc != 5) {
        printf ("usage: %s <number of threads> <start> <end> <step>\n", argv[0]);
        return 1;
    }
    struct timespec t_begin, t_end;
    clock_gettime (CLOCK_REALTIME, &t_begin);

    sem_t sem;
    sem_init (&sem, 0, 1); // init sem that is visible to all the threads and allows a single thread in protected section.

    // determine constants
    double start = atof (argv[2]);
    double end = atof (argv[3]);
    double step = atof (argv[4]);
    size_t N_proc = (size_t) atoll (argv[1]);
    double len = (end - start) / N_proc;
    double S_total = 0;

    pthread_t *tids = (pthread_t *) calloc (N_proc, sizeof (pthread_t));
    pthread_attr_t *thr_attrs = (pthread_attr_t *) calloc (N_proc, sizeof (pthread_attr_t));
    struct sum_part *S_parts =  (struct sum_part *) calloc (N_proc, sizeof (struct sum_part));
    // just create necessary amount of
    for (size_t i = 0; i < N_proc; i++) {
        pthread_attr_init (&thr_attrs[i]);
        // determine amount of work and start numbers. thread_num is needed not to use non human-readable tids.
            S_parts[i].start = start + i * len;
            S_parts[i].end = start + (i + 1) * len;
            S_parts[i].h = step;
            S_parts[i].thread_num = i;
            S_parts[i].S_total = &S_total;
            S_parts[i].semaphore = sem;
        // start routines
        pthread_create (&tids[i], &thr_attrs[i], thread_routine, (void **) &S_parts[i]);
    }
    for (size_t i = 0; i < N_proc; i++) {
        pthread_join (tids[i], NULL);
    }
    clock_gettime (CLOCK_REALTIME, &t_end);
    double elapsed = t_end.tv_sec - t_begin.tv_sec + (t_end.tv_nsec - t_begin.tv_nsec) / 1000000000.0;
    printf ("S = %lg, elapsed = %lgs\n", S_total, elapsed);

    sem_destroy (&sem);
    free (tids);
    free (thr_attrs);
    free (S_parts);
    return 0;
}