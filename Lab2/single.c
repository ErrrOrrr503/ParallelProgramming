#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>
#include <math.h>

const char *single_file = "single_res.txt";

typedef double time_run_t;

int main (int argc, char **argv)
{
    if (argc != 3) {
        printf ("usage: %s <i size> <j_size>", argv[0]);
        return -1;
    }
    int i_size = atoi (argv[1]);
    int j_size = atoi (argv[2]);
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
    */
    time_run_t start = omp_get_wtime();
    for (int i = 3; i < i_size; i++)
    {
        double *a_i = a + i * j_size;
        double *a_i_3 = a + (i - 3) * j_size;
        for (int j = 0; j < j_size - 2; j++)
        {
            a_i[j] = sin (3 * a_i_3[j + 2]);
        }
    }
    time_run_t time = omp_get_wtime() - start;

    FILE *ff = fopen(single_file,"w");
    for(int i = 0; i < i_size; i++){
        for (int j = 0; j < j_size; j++) {
            fprintf(ff,"%f ",a[i * j_size + j]);
        }
        fprintf(ff,"\n");
    }
    fclose(ff);
    free (a);

    printf ("%f\n", time);
    return 0;
}