#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

void non_atomic (int *var)
{
    int temp = *var;
    *var = 0;
    *var += 1;
    *var *= temp;
}

int main ()
{
    int var = 0, executor = 0;
    omp_lock_t lock;
    omp_init_lock (&lock);
    #pragma omp parallel shared (var, lock, executor)
    {
        do {
            omp_unset_lock (&lock);
            omp_set_lock (&lock);
        } while (executor != omp_get_thread_num ());
        non_atomic (&var);
        var++;
        executor++;
        printf ("Thread %d in the routine\n", omp_get_thread_num ());
        omp_unset_lock (&lock);
    }
    printf ("Var = %d\n", var);
    return 0;
}