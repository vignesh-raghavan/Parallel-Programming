#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <omp.h>


void mulMatrix(int rows, int *A[rows], int *B[rows], int *C[rows])
{
	int i, j, k, cols;

	cols = rows;

	for(i = 0; i < rows; i++)
	{
		for(j = 0; j < cols; j++)
		{
			C[i][j] = 0;
			for(k = 0; k < cols; k++)
			{
				C[i][j] += A[i][k] * B[k][j];
			}
		}
	}

	return;
}

void mulMatrixpll(int rows, int *A[rows], int *B[rows], int *C[rows])
{

	#pragma omp parallel
	{
	   int i, j, k, t, p, lb, ub;

		t = omp_get_thread_num();
		p = omp_get_num_threads();

		lb = t * (rows/p);
		ub = lb + (rows/p);

	   for(i = 0; i < rows; i++)
	   {
	   	for(j = lb; j < ub; j++)
	   	{
	   		C[i][j] = 0;
	   		for(k = 0; k < rows; k++)
	   		{
	   			C[i][j] += A[i][k] * B[k][j];
	   		}
	   	}
	   }
	}

	return;
}

void initVector(int rows, int *A[rows], int cols)
{
	int i, j;

	for(i = 0; i < rows; i++)
	{
		for(j = 0; j < cols; j++) A[i][j] = rand()%3;
	}
	return;
}

void printVector(int rows, int *A[rows], int cols, char *str)
{
	int i, j;

	printf("%s\n", str);
	for(i = 0; i < rows; i++)
	{
		for(j = 0; j < cols; j++) printf("%d ", A[i][j]);
	   printf("\n");
	}

	return;
}


int main(int argc, char* argv[])
{
	int i;
	int p, N, M;

	double time1, time2;


	if(argc >= 3)
	{
		printf("\n\nActive Threads=%s, N=%s\n\n", argv[1], argv[2]);
		p = atoi(argv[1]);
		N = atoi(argv[2]);
	}
	else
	{
   	printf("Command Line : %s", argv[0]);
   	for(i = 1; i < argc; i++)
   	{
   		printf("%s ", argv[i]);
   	}
		return 0;
	}

	omp_set_num_threads(p);
   #pragma omp parallel
	{
		p = omp_get_num_threads();
	}

	M = N*p;
	
	int *X[M];
	int *Y[M];
	int *Z[M];
	int *W[M];

	for(i = 0; i < M; i++)
	{
	   X[i] = (int*) malloc(M*sizeof(int));
	   Y[i] = (int*) malloc(M*sizeof(int));
	   Z[i] = (int*) malloc(M*sizeof(int));
	   W[i] = (int*) malloc(M*sizeof(int));
	}

	initVector(M, X, M);
	initVector(M, Y, M);

	time1 = omp_get_wtime();
   mulMatrix(M, X, Y, Z);
	time1 = omp_get_wtime() - time1;

	time2 = omp_get_wtime();
   mulMatrixpll(M, X, Y, W);
	time2 = omp_get_wtime() - time2;
	
	//printVector(M, X, M, "X");
	//printf("\n\n");
	//printVector(M, Y, M, "Y");
	//printf("\n\n");
	//printVector(M, Z, M, "Z");
	//printf("\n\n");
	//printVector(M, W, M, "W");


	printf("Elapsed Time (SERIAL) is %f\n", time1);
	printf("Elapsed Time (PARALLEL) is %f\n", time2);
	printf("SPEEDUP (Serial vs Parallel) is %f\n", time1/time2);

	return 0;
}
