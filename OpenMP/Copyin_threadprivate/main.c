#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

int main ()
{
    static int Nthreadprivate = -666; // threadprivate variable should be global (at least for namespace), but local works too?
    #pragma omp threadprivate (Nthreadprivate)
    #pragma omp parallel copyin (Nthreadprivate)
    {
        Nthreadprivate += omp_get_thread_num () + 1;
        printf ("Changed Nthreadprivate = %d; Threadnum = %d\n", Nthreadprivate, omp_get_thread_num ());
    }
    #pragma omp parallel copyin (Nthreadprivate)
    {
        printf ("Nthreadprivate = %d; Threadnum = %d\n", Nthreadprivate, omp_get_thread_num ());
    }
    return 0;
}