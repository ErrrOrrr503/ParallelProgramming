#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

int main()
{
    omp_set_nested (1);
    #pragma omp parallel num_threads(2)
    {
        printf ("level 1, thread %d from %d; ancestor %d\n", omp_get_thread_num (), omp_get_num_threads (), omp_get_ancestor_thread_num (omp_get_level()));
        #pragma omp barrier

        int thread_num = omp_get_thread_num ();
        #pragma omp parallel num_threads(3) firstprivate (thread_num)
        {
            printf ("level 2, thread %d from %d; ancestor %d\n", omp_get_thread_num (), omp_get_num_threads (), omp_get_ancestor_thread_num (omp_get_level()));
            #pragma omp barrier

            thread_num = omp_get_thread_num ();
            #pragma omp parallel num_threads(3) firstprivate (thread_num)
            {
                printf ("level 3, thread %d from %d; ancestor %d\n", omp_get_thread_num (), omp_get_num_threads (), omp_get_ancestor_thread_num (omp_get_level()));
                #pragma omp barrier
            }
        }
    }
    return 0;
}