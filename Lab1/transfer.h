/* ##########################################

    du/dt (x, t) + a * du/dx (x, t) = f (x, t),   0 < x < x_max; 0 < t < t_max. 
    u (0, t) = u_0_t
    u (x, 0) = u_x_0

        0        h        2h    ...
   --------------------------------> x
  0 |_u[0][0] _u[0][1] _u[0][2] ...
  t |_u[1][0] _u[1][1] _u[1][2] ...
 2t |_u[2][0] _u[2][1] _u[2][2] ...
  . |..............................
  . |..............................
  . |..............................
    |..............................
    |..............................
    |..............................
    v

    NOTES:
    1) We will precalculate h and t only for optimization to use as standalone values. To avoid summary errors: h + h + h + h .... != n * h for too low h.
       the point will be calculated as i * axis_limit / (N - 1) to make sure final point is real final point.
    2) Matrix from ComputeMath repo is not parallel and quite slow, it is needed for human-readable operator [][] and for printing option.
       Matrix is based on raw num_t* array. Printing is O(N^2) and [][] is O(1).
    3) _u[0][0] is taken from _u_x_0
    4) WE WILL USE SHARED MEMORY, as gathering the answere is nonsence! Effort for calculating a value in the field is O(1), ~ 1000 tacts (due to floats and division),
       much less than MPI's queue will take. So calculation on a single process will be faster than multi-process calculation.
       Due to that problem this task is not sutable for NUMA with separate memory, due to inter-processor bus will be the bottleneck both for shared memory and gathering.

       Well, we can do without gathering and print out accordingly using messages, but commonly, we need a matrix for further analysis, that will e passed to other modules.

########################################## */

#include <mpi.h>
#include <unistd.h>

#define MPI_TAG_EDGE_TRANSFER 1
#define MPI_TAG_EDGE_TRANSFER_LEFT 3
#define MPI_TAG_EDGE_TRANSFER_RIGHT 4
#define MPI_TAG_GATHER 2

template <typename num_t>
class transfer_equation {
    private:
        num_t *_u = nullptr; // the net - solution we are constructing
        size_t _n_for_proc = 0;
        int _n_rest = 0;
        size_t _x_points = 0, _t_points = 0; //number of points by t and x axes
        size_t _res_layer_start = 0;
        size_t _res_layer_end = 0; 
        num_t _x_max, _t_max; // maximum X and T
        num_t (*_u_0_t) (num_t); // edge condition function for (0, t)
        num_t (*_u_x_0) (num_t); // edge condition function for (x, 0)
        num_t _a; // a from the equation
        num_t (*_f) (num_t, num_t); // f (x, t) from the equation
        int _verboseness = 1; // how verbose printing is. 0 - result only for frontends. 1(default) - human-readable. 2 - debug info. -1 - no output
        bool _u_arr_is_mpi_shmem = 0;
        int _rank;
        int _comm_size;
        double _rank_time_start = 0;
        double _rank_time_calc = 0;
        double _rank_time_gather = 0;

        num_t _h;
        num_t _t;
    public:
        num_t *result = nullptr;
        transfer_equation () = delete;
        transfer_equation (size_t x_points, size_t t_points, num_t x_max, num_t t_max, num_t a, num_t (*f) (num_t, num_t), num_t (*u_x_0) (num_t), num_t (*u_0_t) (num_t));
        ~transfer_equation ();

        void set_verboseness (int verboseness);
        void solve_explicit_level_triange ();
        void solve_explicit_quad_dot ();
        void gather_result_to_matrix (size_t t_start, size_t t_end);
        void print_ans ();
        void print_perf ();
};

template <typename num_t>
transfer_equation<num_t>::transfer_equation (size_t x_points, size_t t_points, num_t x_max, num_t t_max, num_t a, num_t (*f) (num_t, num_t), num_t (*u_x_0) (num_t), num_t (*u_0_t) (num_t)) :
    _x_points (x_points),
    _t_points (t_points),
    _x_max (x_max),
    _t_max (t_max),
    _u_0_t (u_0_t),
    _u_x_0 (u_x_0),
    _a (a),
    _f (f),

    _h (x_max / (x_points - 1)),
    _t (t_max / (t_points - 1))
{
    MPI_Init (NULL, NULL);
    MPI_Comm_size (MPI_COMM_WORLD, &_comm_size);
    MPI_Comm_rank (MPI_COMM_WORLD, &_rank);
    _rank_time_start = MPI_Wtime ();
}

template <typename num_t>
transfer_equation<num_t>::~transfer_equation ()
{
    if (_u != nullptr)
        delete[] _u;
    if (result != nullptr && _comm_size != 1)
        delete[] result;
    MPI_Finalize ();
}

template <typename num_t>
void transfer_equation<num_t>::set_verboseness (int verboseness)
{
    _verboseness = verboseness;
}

template <typename num_t>
void transfer_equation<num_t>::print_ans ()
{
    if (_rank)
        return;
    if (result == nullptr)
        throw std::runtime_error ("print_ans() is called before answere gathering");
    if (_verboseness < 0)
        return;
    if (_verboseness)
        printf ("U (x, t) =\n");
    for (size_t i = 0; i < _res_layer_end - _res_layer_start; i++) {
        if (_verboseness)
            printf ("t = %5.3lg | ", (i + _res_layer_start) * _t);
        for (size_t j = 0; j < _x_points; j++)
            printf ("%8.3lf ", result[i * _x_points + j]);
        printf ("\n");
    }
}

template <typename num_t>
void transfer_equation<num_t>::print_perf ()
{
    double *_rank_times_calc = new double[_comm_size];
    double *_rank_times_gather = new double[_comm_size];
    double *_rank_times_total = new double[_comm_size];
    MPI_Gather (&_rank_time_calc, sizeof (double), MPI_CHAR, _rank_times_calc, sizeof (double), MPI_CHAR, 0, MPI_COMM_WORLD);
    MPI_Gather (&_rank_time_gather, sizeof (double), MPI_CHAR, _rank_times_gather, sizeof (double), MPI_CHAR, 0, MPI_COMM_WORLD);
    double time_total = MPI_Wtime () - _rank_time_start; 
    MPI_Gather (&time_total, sizeof (double), MPI_CHAR, _rank_times_total, sizeof (double), MPI_CHAR, 0, MPI_COMM_WORLD);
    if (!_rank) {
        if (_verboseness)
            printf ("rank %d: calc: %lgs; gather %lgs; total: %lgs\n", _rank, _rank_time_calc, _rank_time_gather, MPI_Wtime () - _rank_time_start);
        else
            printf ("%d %lg %lg %lg\n", _rank, _rank_time_calc, _rank_time_gather, MPI_Wtime () - _rank_time_start);
        for (int i = 1; i < _comm_size; i++) {
            if (_verboseness)
                printf ("rank %d: calc: %lgs; gather %lgs; total: %lgs\n", i, _rank_times_calc[i], _rank_times_gather[i], _rank_times_total[i]);
            else
                printf ("%d %lg %lg %lg\n", i, _rank_times_calc[i], _rank_times_gather[i], _rank_times_total[i]);
        }
    }
    delete[] _rank_times_calc;
    delete[] _rank_times_gather;
    delete[] _rank_times_total;
}

template <typename num_t>
void transfer_equation<num_t>::gather_result_to_matrix (size_t layer_start, size_t layer_end)
{
    if (!_n_for_proc) // nothing to do
        return;
    double gather_start = MPI_Wtime ();
    _res_layer_start = layer_start;
    _res_layer_end = layer_end;
    if (_comm_size == 1) {
        result = _u;
        _rank_time_gather = MPI_Wtime () - gather_start;
        return;
    }
    if (layer_start >= layer_end || layer_end > _t_points)
        throw std::runtime_error ("check limits for gather_result_to_matrix");
    if (!_rank) {
        result = new num_t[_x_points * (layer_end - layer_start)];
        for (size_t i = 0; i < layer_end - layer_start; i++) {
            for (size_t j = 0; j < _n_for_proc; j++)
                result[i * _x_points + j] = _u[(i + layer_start) * _n_for_proc + j];
        }
        for (int sender = 1; sender < _comm_size; sender++) {
            size_t n_for_sender = _n_for_proc;
            size_t start = _n_for_proc * sender;
            if (sender >= _n_rest && _n_rest) {
                n_for_sender = _n_for_proc - 1;
                start = (_n_for_proc) * _n_rest + (_n_for_proc - 1) * (sender - _n_rest);
            }
            if (!n_for_sender) // everything already gathered
                return;
            MPI_Recv (&_u[layer_start * n_for_sender], n_for_sender * (layer_end - layer_start) * sizeof (num_t), MPI_CHAR, sender, MPI_TAG_GATHER, MPI_COMM_WORLD, NULL);
            for (size_t i = 0; i < layer_end - layer_start; i++) {
                for (size_t j = 0; j < n_for_sender; j++)
                    result[i * _x_points + start + j] = _u[(i + layer_start) * n_for_sender + j];
            }
        }
    }
    else {
        MPI_Send (&_u[layer_start * _n_for_proc], _n_for_proc * (layer_end - layer_start) * sizeof (num_t), MPI_CHAR, 0, MPI_TAG_GATHER, MPI_COMM_WORLD);
    }
    _rank_time_gather = MPI_Wtime () - gather_start;
}

template <typename num_t>
void transfer_equation<num_t>::solve_explicit_level_triange ()
{
    // each process recieves work to calculate N = x_points / comm_size or N+1 points.
    // firstly each process except the last one will calculate
    double calc_start = MPI_Wtime ();
    MPI_Request unused_request;

    size_t n_for_proc = _x_points / _comm_size;
    _n_rest = _x_points % _comm_size;
    size_t start = (n_for_proc + 1) * _n_rest + n_for_proc * (_rank - _n_rest);
    if (_rank < _n_rest) {
        start = (n_for_proc + 1) * _rank;
        n_for_proc = n_for_proc + 1;
    }
    _n_for_proc = n_for_proc;
    if (!_n_for_proc) // nothing to do
        return;
    _u = new num_t [n_for_proc * _t_points];
    for (size_t layer = 0; layer < _t_points; layer++) {
        if (layer == 0) {
            for (size_t i = n_for_proc; i > 0; i--)
                _u[i - 1] = _u_x_0 ((start + i - 1) * _h);
        }
        else {
            // send edge point in order other process can calculate it's part of the layer
            if (_rank < _comm_size - 1)
                MPI_Isend (&_u[(layer - 1) * _n_for_proc + n_for_proc - 1], sizeof (num_t), MPI_CHAR, _rank + 1, MPI_TAG_EDGE_TRANSFER, MPI_COMM_WORLD, &unused_request);
            // calculate part until edge
            for (size_t i = n_for_proc - 1; i > 0; i--)
                _u[layer * _n_for_proc + i] = _t * (_f ((i + start) * _h, (layer - 1) * _t) - _a * (_u[(layer - 1) * _n_for_proc + i] - _u[(layer - 1) * _n_for_proc + i - 1]) / _h) + _u[(layer - 1) * _n_for_proc + i];
            // receive calculate value on edge,
            if (_rank) {
                num_t u_i_1_layer_1 = -1;
                MPI_Recv (&u_i_1_layer_1, sizeof (num_t), MPI_CHAR, _rank - 1, MPI_TAG_EDGE_TRANSFER, MPI_COMM_WORLD, NULL);
                _u[layer * _n_for_proc] = _t * (_f (start * _h, (layer - 1) * _t) - _a * (_u[(layer - 1) * _n_for_proc] - u_i_1_layer_1) / _h) + _u[(layer - 1) * _n_for_proc];
            }
            else { // u[t][0] is calculated from initial conditions
                _u[layer * _n_for_proc] = _u_0_t (layer * _t);
            }
        }
    }
    _rank_time_calc = MPI_Wtime () - calc_start;
}

template <typename num_t>
void transfer_equation<num_t>::solve_explicit_quad_dot ()
{
    // each process recieves work to calculate N = x_points / comm_size or N+1 points.
    // firstly each process except the last one will calculate
    double calc_start = MPI_Wtime ();
    MPI_Request unused_request;

    size_t n_for_proc = _x_points / _comm_size;
    _n_rest = _x_points % _comm_size;
    size_t start = (n_for_proc + 1) * _n_rest + n_for_proc * (_rank - _n_rest);
    if (_rank < _n_rest) {
        start = (n_for_proc + 1) * _rank;
        n_for_proc = n_for_proc + 1;
    }
    _n_for_proc = n_for_proc;
    if (!_n_for_proc) // nothing to do
        return;
    _u = new num_t [n_for_proc * _t_points];
    for (size_t layer = 0; layer < _t_points; layer++) {
        if (layer == 0) {
            for (size_t i = n_for_proc; i > 0; i--)
                _u[i - 1] = _u_x_0 ((start + i - 1) * _h);
        }
        else {
            // send edge point in order other process can calculate it's part of the layer
            if (!_rank) {
                // rank0 should pass only end point 
                if (_rank < _comm_size - 1) {
                    MPI_Isend (&_u[(layer - 1) * _n_for_proc + n_for_proc - 1], sizeof (num_t), MPI_CHAR, _rank + 1, MPI_TAG_EDGE_TRANSFER_LEFT, MPI_COMM_WORLD, &unused_request);
                }
            }
            else {
                if (_rank < _comm_size - 1) {
                    // middle ranks should pass both points.
                    MPI_Isend (&_u[(layer - 1) * _n_for_proc + n_for_proc - 1], sizeof (num_t), MPI_CHAR, _rank + 1, MPI_TAG_EDGE_TRANSFER_LEFT, MPI_COMM_WORLD, &unused_request);
                    MPI_Isend (&_u[(layer - 1) * _n_for_proc], sizeof (num_t), MPI_CHAR, _rank - 1, MPI_TAG_EDGE_TRANSFER_RIGHT, MPI_COMM_WORLD, &unused_request);
                }
                else
                    MPI_Isend (&_u[(layer - 1) * _n_for_proc], sizeof (num_t), MPI_CHAR, _rank - 1, MPI_TAG_EDGE_TRANSFER_RIGHT, MPI_COMM_WORLD, &unused_request);
            }

            // calculate part until edge except the very left point (we should receive data for it)
            for (ssize_t i = (ssize_t) n_for_proc - 2; i > 0; i--) {
                _u[layer * _n_for_proc + i] = _t * (_f ((i + start) * _h, (layer - 1) * _t) - _a * (_u[(layer - 1) * _n_for_proc + i + 1] - _u[(layer - 1) * _n_for_proc + i - 1]) / 2 / _h + 0.5 * _a * _t * (_u[(layer - 1) * _n_for_proc + i + 1] - 2 * _u[(layer - 1) * _n_for_proc + i] + _u[(layer - 1) * _n_for_proc + i - 1]) / _h / _h) + _u[(layer - 1) * _n_for_proc + i];
            }
            if (n_for_proc > 1) {
                // receive calculate value on right edge
                if (_rank) {
                    num_t u_i_1_layer_1 = -1;
                    MPI_Recv (&u_i_1_layer_1, sizeof (num_t), MPI_CHAR, _rank - 1, MPI_TAG_EDGE_TRANSFER_LEFT, MPI_COMM_WORLD, NULL);
                    _u[layer * _n_for_proc] = _t * (_f (start * _h, (layer - 1) * _t) - _a * (_u[(layer - 1) * _n_for_proc + 1] - u_i_1_layer_1) / 2 / _h + 0.5 * _a * _t * (_u[(layer - 1) * _n_for_proc + 1] - 2 * _u[(layer - 1) * _n_for_proc] + u_i_1_layer_1) / _h / _h) + _u[(layer - 1) * _n_for_proc];
                }
                else { // u[t][0] is calculated from initial conditions
                    _u[layer * _n_for_proc] = _u_0_t (layer * _t);
                }
                // receive and calculate value on left edge
                if (_rank < _comm_size - 1) {
                    num_t u_i_1_layer1 = -1;
                    MPI_Recv (&u_i_1_layer1, sizeof (num_t), MPI_CHAR, _rank + 1, MPI_TAG_EDGE_TRANSFER_RIGHT, MPI_COMM_WORLD, NULL);
                    size_t i = _n_for_proc - 1;
                    _u[layer * _n_for_proc + i] = _t * (_f ((i + start) * _h, (layer - 1) * _t) - _a * (u_i_1_layer1 - _u[(layer - 1) * _n_for_proc + i - 1]) / 2 / _h + 0.5 * _a * _t * (u_i_1_layer1 - 2 * _u[(layer - 1) * _n_for_proc + i] + _u[(layer - 1) * _n_for_proc + i - 1]) / _h / _h) + _u[(layer - 1) * _n_for_proc + i];
                }
                else {
                    // we could have used characteristics to find out the value if f === 0, but in that case approximation is not necessary at all.
                    // so we will use left triangle by far, losing order due to these edge points.
                    size_t i = n_for_proc - 1;
                    _u[layer * _n_for_proc + i] = _t * (_f ((i + start) * _h, (layer - 1) * _t) - _a * (_u[(layer - 1) * _n_for_proc + i] - _u[(layer - 1) * _n_for_proc + i - 1]) / _h) + _u[(layer - 1) * _n_for_proc + i];
                }
            }
            else {
                if (_rank < _comm_size - 1) {
                    if (_rank) {
                        num_t u_i_1_layer_1 = -1;
                        MPI_Recv (&u_i_1_layer_1, sizeof (num_t), MPI_CHAR, _rank - 1, MPI_TAG_EDGE_TRANSFER_LEFT, MPI_COMM_WORLD, NULL);
                        num_t u_i_1_layer1 = -1;
                        MPI_Recv (&u_i_1_layer1, sizeof (num_t), MPI_CHAR, _rank + 1, MPI_TAG_EDGE_TRANSFER_RIGHT, MPI_COMM_WORLD, NULL);
                        _u[layer * _n_for_proc] = _t * (_f (start * _h, (layer - 1) * _t) - _a * (u_i_1_layer1 - u_i_1_layer_1) / 2 / _h + 0.5 * _a * _t * (u_i_1_layer1 - 2 * _u[(layer - 1) * _n_for_proc] + u_i_1_layer_1) / _h / _h) + _u[(layer - 1) * _n_for_proc];
                    }
                    else{
                        _u[layer * _n_for_proc] = _u_0_t (layer * _t);
                    }
                }
                else {
                    num_t u_i_1_layer_1 = -1;
                    MPI_Recv (&u_i_1_layer_1, sizeof (num_t), MPI_CHAR, _rank - 1, MPI_TAG_EDGE_TRANSFER_LEFT, MPI_COMM_WORLD, NULL);
                    _u[layer * _n_for_proc] = _t * (_f (start * _h, (layer - 1) * _t) - _a * (_u[(layer - 1) * _n_for_proc] - u_i_1_layer_1) / _h) + _u[(layer - 1) * _n_for_proc];
                }
            }
        }
    }
    _rank_time_calc = MPI_Wtime () - calc_start;
}