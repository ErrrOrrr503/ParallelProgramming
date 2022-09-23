#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <time.h>

int N[65];

void rand_routine (int *x, int i)
{
    for (int j = 0; j < N[i]; j++) {
        *x++;
    }
}

int main (int argc, char *argv[])
{
    int x = 0;
    int out[65];
    for (int j = 0; j < 65; j++) {
        srand (clock ());
        N[j] = rand () % 3276700;
    }
    omp_set_num_threads (4);

    printf ("#### default scheduler ####\n");
    clock_t start = clock ();
    #pragma omp parallel for reduction (+:x)
    for (int i = 0; i < 65; i++) {
        rand_routine (&x, i);
        out[i] = omp_get_thread_num ();
    }
    printf ("consumed %lu clocks\n", clock () - start);
    for (int i = 0; i < 65; i++)
        printf ("iteration %d; thread %d\n", i, out[i]);

    printf ("\n#### static scheduler, chunk 1 ####\n");
    start = clock ();
    #pragma omp parallel for reduction (+:x) schedule (static,1)
    for (int i = 0; i < 65; i++) {
        rand_routine (&x, i);
        out[i] = omp_get_thread_num ();
    }
    printf ("consumed %lu clocks\n", clock () - start);
    for (int i = 0; i < 65; i++)
        printf ("iteration %d; thread %d\n", i, out[i]);

    printf ("\n#### dynamic scheduler, chunk 1 ####\n");
    start = clock ();
    #pragma omp parallel for reduction (+:x) schedule (dynamic,1)
    for (int i = 0; i < 65; i++) {
        rand_routine (&x, i);
        out[i] = omp_get_thread_num ();
    }
    printf ("consumed %lu clocks\n", clock () - start);
    for (int i = 0; i < 65; i++)
        printf ("iteration %d; thread %d\n", i, out[i]);

    printf ("\n#### guided scheduler, chunk 1 ####\n");
    start = clock ();
    #pragma omp parallel for reduction (+:x)  schedule (guided,1)
    for (int i = 0; i < 65; i++) {
        rand_routine (&x, i);
        out[i] = omp_get_thread_num ();
    }
    printf ("consumed %lu clocks\n", clock () - start);
    for (int i = 0; i < 65; i++)
        printf ("iteration %d; thread %d\n", i, out[i]);

    printf ("\n#### static scheduler, chunk 4 ####\n");
    start = clock ();
    #pragma omp parallel for reduction (+:x) schedule (static,4)
    for (int i = 0; i < 65; i++) {
        rand_routine (&x, i);
        out[i] = omp_get_thread_num ();
    }
    printf ("consumed %lu clocks\n", clock () - start);
    for (int i = 0; i < 65; i++)
        printf ("iteration %d; thread %d\n", i, out[i]);

    printf ("\n#### dynamic scheduler, chunk 4 ####\n");
    start = clock ();
    #pragma omp parallel for reduction (+:x) schedule (dynamic,4)
    for (int i = 0; i < 65; i++) {
        rand_routine (&x, i);
        out[i] = omp_get_thread_num ();
    }
    printf ("consumed %lu clocks\n", clock () - start);
    for (int i = 0; i < 65; i++)
        printf ("iteration %d; thread %d\n", i, out[i]);

    printf ("\n#### guided scheduler, chunk 4 ####\n");
    start = clock ();
    #pragma omp parallel for reduction (+:x)  schedule (guided,4)
    for (int i = 0; i < 65; i++) {
        rand_routine (&x, i);
        out[i] = omp_get_thread_num ();
    }
    printf ("consumed %lu clocks\n", clock () - start);
    for (int i = 0; i < 65; i++)
        printf ("iteration %d; thread %d\n", i, out[i]);
    return 0;
}