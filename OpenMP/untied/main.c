#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <omp.h>

int main ()
{
    static int destroyer = 1;
    unsigned long long S[12];
    #pragma omp threadprivate (destroyer)
    #pragma omp parallel shared (S) num_threads(12)
    {
        int thread_num = omp_get_thread_num ();
        S[thread_num] = 0;
        destroyer = omp_get_thread_num () % 4;
        // zeoros must be in 0, 4, 8 threads only!
        #pragma omp task untied shared (S, thread_num)
        {
            for (int i = 0; i < 10000; i++) {
                for (int i = 0; i < 1000; i++) {
                    S[thread_num] += omp_get_thread_num ();
                }
                #pragma omp taskyield
            }
        }
        #pragma omp taskwait
    }
    for (int i = 0; i < 12; i++)
        printf ("S = %llu, thread %d\n", S[i], i);

    return 0;
}