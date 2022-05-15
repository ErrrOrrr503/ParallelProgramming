#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/syscall.h>

void *thread_routing (void *param) {
    // gettid is unsupported at cluster... linux 3.10, gcc 4.8.5, c99 default. 2022. Seriously!?
    pid_t tid = syscall(SYS_gettid);
    printf ("Hello World form tid = %d\n", tid);
    pthread_exit (0);
}

int main (int argc, char *argv[]) {
    if (argc != 2) {
        printf ("usage: %s <number of threads>\n", argv[0]);
        return 1;
    }
    int num_proc = atoi (argv[1]);
    pthread_t *tids = (pthread_t *) calloc (num_proc, sizeof (pthread_t));
    pthread_attr_t *thr_attrs = (pthread_attr_t *) calloc (num_proc, sizeof (pthread_attr_t));
    for (int i = 0; i < num_proc; i++) {
        pthread_attr_init (&thr_attrs[i]);
        pthread_create (&tids[i], &thr_attrs[i], thread_routing, NULL);
        pthread_join (tids[i], NULL);
    }
}