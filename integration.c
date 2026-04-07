#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>
#include <stddef.h>

typedef struct
{
    double a;     // Left bound
    double b;     // Right bound
    double tol;   // tolerance for this interval
    double whole; // Simpson estimate for the whole interval [a, b]
} Task;

#define TAG_WORK 1         // Master to Worker: send task
#define TAG_RESULT 2       // Worker to Master: send result
#define TAG_NEW_TASK 3     // Worker to Master: send split sub-interval
#define TAG_STOP 4         // Master to Worker: signal termination
#define TAG_WORK_REQUEST 5 // Worker to Master: request work

#define MAX_TASKS 10000

typedef double (*func_ptr)(double);

Task task_stack[MAX_TASKS];
int stack_top = -1;
MPI_Datatype task_type;

void push_task(Task t)
{
    if (stack_top < MAX_TASKS - 1)
    {
        task_stack[++stack_top] = t;
    }
    else
    {
        fprintf(stderr, "Error: Task queue overflow!\n");
    }
}

Task pop_task()
{
    return task_stack[stack_top--];
}

/* Functions to integrate*/
double fx0(const double x) { return sin(x) + 0.5 * cos(3 * x); }
double fx1(const double x)
{
    return 1.0 / (1.0 + 100.0 * pow((x - 0.3), 2));
}

double fx2(const double x)
{
    return sin(200.0 * x) * exp(-x);
}

/* Adaptive Simpson's Rule*/
double adaptive_simpson(func_ptr f, double a, double b, double tol, double whole)
{
    double m = (a + b) / 2.0;
    double h_half = (m - a) / 6.0;
    double left_simpson = h_half * (f(a) + 4.0 * f((a + m) / 2.0) + f(m));
    double right_simpson = h_half * (f(m) + 4.0 * f((m + b) / 2.0) + f(b));

    if (fabs(left_simpson + right_simpson - whole) <= 15.0 * tol)
    {
        return left_simpson + right_simpson + (left_simpson + right_simpson - whole) / 15.0;
    }

    return adaptive_simpson(f, a, m, tol / 2.0, left_simpson) +
           adaptive_simpson(f, m, b, tol / 2.0, right_simpson);
}

void process_task(Task t, func_ptr f)
{
    double m = (t.a + t.b) / 2.0;
    double h_half = (m - t.a) / 6.0;
    double left_s = h_half * (f(t.a) + 4.0 * f((t.a + m) / 2.0) + f(m));
    double right_s = h_half * (f(m) + 4.0 * f((m + t.b) / 2.0) + f(t.b));
    double refined = left_s + right_s;

    if (fabs(refined - t.whole) <= 15.0 * t.tol)
    {
        double result = refined + (refined - t.whole) / 15.0;
        MPI_Send(&result, 1, MPI_DOUBLE, 0, TAG_RESULT, MPI_COMM_WORLD); // [cite: 11]
    }
    else
    {
        Task right_task = {m, t.b, t.tol / 2.0, right_s};
        MPI_Send(&right_task, 1, task_type, 0, TAG_NEW_TASK, MPI_COMM_WORLD);

        Task left_task = {t.a, m, t.tol / 2.0, left_s};
        process_task(left_task, f);
    }
}

double adaptive_simpson_hybrid(func_ptr f, double a, double b, double tol, double whole)
{
    double m = (a + b) / 2.0;
    double h_half = (m - a) / 6.0;
    double left_s = h_half * (f(a) + 4.0 * f((a + m) / 2.0) + f(m));
    double right_s = h_half * (f(m) + 4.0 * f((m + b) / 2.0) + f(b));

    if (fabs(left_s + right_s - whole) <= 15.0 * tol)
    {
        return left_s + right_s + (left_s + right_s - whole) / 15.0;
    }

    double left_res, right_res;

#pragma omp task shared(left_res)
    left_res = adaptive_simpson_hybrid(f, a, m, tol / 2.0, left_s);

#pragma omp task shared(right_res)
    right_res = adaptive_simpson_hybrid(f, m, b, tol / 2.0, right_s);

#pragma omp taskwait
    return left_res + right_res;
}

int main(int argc, char **argv)
{
    MPI_Init(&argc, &argv);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (argc < 4)
    {
        if (rank == 0)
            printf("Usage: mpirun -np P integration func_id mode tol\n");
        MPI_Finalize();
        return 1;
    }

    const int func_id = atoi(argv[1]);
    const int mode = atoi(argv[2]);
    const double tol = atof(argv[3]);

    func_ptr f = (func_id == 0) ? fx0 : (func_id == 1 ? fx1 : fx2);

    int blocklengths[4] = {1, 1, 1, 1};
    MPI_Datatype types[4] = {MPI_DOUBLE, MPI_DOUBLE, MPI_DOUBLE, MPI_DOUBLE};
    MPI_Aint displacements[4];

    displacements[0] = offsetof(Task, a);
    displacements[1] = offsetof(Task, b);
    displacements[2] = offsetof(Task, tol);
    displacements[3] = offsetof(Task, whole);

    MPI_Type_create_struct(4, blocklengths, displacements, types, &task_type);
    MPI_Type_commit(&task_type);

    double total_time = 0.0;
    int num_runs = 10;
    double results[10];

    for (int run = 0; run < num_runs; run++)
    {
        MPI_Barrier(MPI_COMM_WORLD);
        double start = MPI_Wtime();

        if (mode == 0 && rank == 0)
        {

            double init_est = (1.0 / 6.0) * (f(0.0) + 4.0 * f(0.5) + f(1.0));
            double result = adaptive_simpson(f, 0.0, 1.0, tol, init_est);
        }
        else if (mode == 1)
        {
            if (rank == 0)
            {
                double total_integral = 0.0;
                int active_workers = 0;

                Task first = {0.0, 1.0, tol, (1.0 / 6.0) * (f(0.0) + 4.0 * f(0.5) + f(1.0))};
                push_task(first);

                while (active_workers > 0 || stack_top >= 0)
                {
                    MPI_Status status;
                    MPI_Recv(NULL, 0, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
                    int worker_id = status.MPI_SOURCE;

                    if (status.MPI_TAG == TAG_WORK_REQUEST)
                    {
                        if (stack_top >= 0)
                        {
                            Task t = pop_task();
                            MPI_Send(&t, 1, task_type, worker_id, TAG_WORK, MPI_COMM_WORLD);
                            active_workers++;
                        }
                        else
                        {

                            MPI_Send(NULL, 0, MPI_INT, worker_id, TAG_STOP, MPI_COMM_WORLD);
                        }
                    }
                    else if (status.MPI_TAG == TAG_RESULT)
                    {
                        double res;
                        MPI_Recv(&res, 1, MPI_DOUBLE, worker_id, TAG_RESULT, MPI_COMM_WORLD, &status);
                        total_integral += res;
                        active_workers--;
                    }
                    else if (status.MPI_TAG == TAG_NEW_TASK)
                    {
                        Task new_t;
                        MPI_Recv(&new_t, 1, task_type, worker_id, TAG_NEW_TASK, MPI_COMM_WORLD, &status);
                        push_task(new_t);
                    }
                }

                for (int i = 1; i < size; i++)
                {
                    MPI_Send(NULL, 0, MPI_INT, i, TAG_STOP, MPI_COMM_WORLD);
                }
            }
            else
            {
                while (1)
                {
                    MPI_Send(NULL, 0, MPI_INT, 0, TAG_WORK_REQUEST, MPI_COMM_WORLD);
                    MPI_Status status;
                    MPI_Probe(0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
                    if (status.MPI_TAG == TAG_STOP)
                    {
                        MPI_Recv(NULL, 0, MPI_INT, 0, TAG_STOP, MPI_COMM_WORLD, &status);
                        break;
                    }
                    Task t;
                    MPI_Recv(&t, 1, task_type, 0, TAG_WORK, MPI_COMM_WORLD, &status);
                    process_task(t, f);
                }
            }
        }
        else if (mode == 2)
        {
            double local_sum = 0.0, global_sum = 0.0;

            int K = size;
            double h = 1.0 / K;
            double a = rank * h;
            double b = (rank + 1) * h;

            double init_est = (h / 6.0) * (f(a) + 4.0 * f((a + b) / 2.0) + f(b));

            double start_parallel = MPI_Wtime();

#pragma omp parallel
            {
#pragma omp single
                {
                    local_sum = adaptive_simpson_hybrid(f, a, b, tol / K, init_est);
                }
            }

            MPI_Reduce(&local_sum, &global_sum, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
        }

        double end = MPI_Wtime();
        if (run > 0)
        {
            total_time += (end - start);
        }
        results[run] = (end - start);
    }

    if (rank == 0)
    {
        printf("Average Time (excluding first): %f s\n", total_time / (num_runs - 1));
    }

    MPI_Type_free(&task_type);
    MPI_Finalize();
    return 0;
}