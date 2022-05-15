#include "transfer.h"
#include "cmath"

typedef double num_t;

num_t a = 0.1;

num_t f (num_t x, num_t t)
{
    return t * cos (x);
}

num_t u_x_0 (num_t x)
{
    return 3 * cos (x * x);
}

num_t u_0_t (num_t t)
{
    return 3 * sin (t * t / 4);
}

int main (int argc, char *argv[])
{
    num_t x_max = 0;
    num_t t_max = 0;
    size_t x_points = 0;
    size_t t_points = 0;
    size_t layer_start = 0;
    size_t layer_end = 0;

    if (!(argc == 5 || argc == 7 || argc == 8 || argc == 6)) {
        std::cout << "usage: mpirun " << argv[0] << " <x_points> <t_points> <x_max> <t_max> [layer+start] [layer_end] [n not to print ans]" << std::endl;
        return 1;
    }
    else {
        x_points = atoll (argv[1]);
        t_points = atoll (argv[2]);
        x_max = atof (argv[3]);
        t_max = atof (argv[4]);
        layer_start = 0;
        layer_end = t_points;
    }
    if (argc == 7 || argc == 8) {
        layer_start = atoll (argv[5]);
        layer_end = atoll (argv[6]);
    }
    bool is_print_ans = 1;
    if (argc == 8) {
        if (!strcmp (argv[7], "n"))
            is_print_ans = 0;
    }
    if (argc == 6) {
        if (!strcmp (argv[5], "n"))
            is_print_ans = 0;
    }
    transfer_equation<num_t> treq (x_points, t_points, x_max, t_max, a, f, u_x_0, u_0_t);
    treq.set_verboseness (0);
    treq.solve_explicit_quad_dot ();
    treq.gather_result_to_matrix (layer_start, layer_end);
    if (is_print_ans)
        treq.print_ans ();
    treq.print_perf ();
    return 0;
}