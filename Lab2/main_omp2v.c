#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>
#include <math.h>

const char *omp_file = NULL;

typedef double time_run_t;

int main (int argc, char **argv)
{
    if (argc != 5 && argc != 4) {
        printf ("usage: %s <num_threads> <result file> <i size> <j_size>", argv[0]);
        return -1;
    }
    int i_size = -1;
    int j_size = -1;
    int num_threads = 0;
    if (argc == 5) {
        i_size = atoi (argv[3]);
        j_size = atoi (argv[4]);
        omp_file = argv[2];
        num_threads = atoi (argv[1]);
    }
    else {
        omp_file = NULL;
        i_size = atoi (argv[2]);
        j_size = atoi (argv[3]);
        num_threads = atoi (argv[1]);
    }
    double *a = (double *) calloc (i_size * j_size, sizeof (double));
    int i, j;
    //подготовительная часть – заполнение некими данными
    for (i = 0; i < i_size; i++) {
        for (j = 0; j < j_size; j++) {
            a[i * j_size + j] = 10 * i + j;
        }
    }
    // task
    /*
     * Lets analyze dependencies:
     * Linear system:
     * i = k - 3; j = l + 2
     * Distance vector is (3, -2)
     * We have true dependecy by i and false dependency by j.
     * We can use up to 3 threads for i cycle and potentially infinity for j cycle (with string duplicate)
     * But why so complex? COLLAPSE! that converts our nested loop into a one loop with 3*j_size - 2 true dependency lenght.
     * in this 3*j_size+2 we can run all our threads. And in fact it is the very same with running external and internal groups.
     * I was too dumb to think of it in MPI implementation.
    */
    int chunk_size = (3 * j_size - 2) / num_threads; 
    time_run_t start = omp_get_wtime();
    #pragma omp parallel for shared (a) collapse (2) num_threads (num_threads) schedule (static, chunk_size) ordered (2)
    for (int i = 3; i < i_size; i++)
    {
        for (int j = 0; j < j_size - 2; j++)
        {
            #pragma omp ordered depend (sink:i-3,j+2)
            a[i * j_size + j] = sin (3 * a[(i - 3) * j_size + j + 2]);
            #pragma omp ordered depend (source)
        }
    }
    time_run_t time = omp_get_wtime() - start;

    if (omp_file != NULL) {
        FILE *ff = fopen(omp_file,"w");
        for(int i = 0; i < i_size; i++){
            for (int j = 0; j < j_size; j++) {
                fprintf(ff,"%f ",a[i * j_size + j]);
            }
            fprintf(ff,"\n");
        }
        fclose(ff);
    }
    free (a);

    printf ("%f\n", time);
    return 0;
}