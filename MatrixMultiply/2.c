#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>


// Multiply NxM matrixA with MxQ matrixB to get NxQ matrix C.
void mulMatrix(int *A, int *B, int *C, int N, int M, int Q)
{
	int i, j, k;

	for(i = 0; i < N; i++)
	{
		for(j = 0; j < Q; j++)
		{
			C[(Q*i+j)] = 0;
			for(k = 0; k < M; k++)
			{
				C[(Q*i+j)] += A[(M*i+k)] * B[(Q*k+j)];
			}
		}
	}

	return;
}


void initVector(int *A, int N)
{
	int i;

	for(i = 0; i < N; i++)
	{
		A[i] = rand()%3;
	}
	return;
}

void printVector(int *A, int N, int M, char *str, int gpid)
{
	int i, j;

	printf("%s (%02d)\n", str, gpid);
	for(i = 0; i < N; i++)
	{
		for(j = 0; j < M; j++) printf("%d ", A[(M*i+j)]);
	   printf("\n");
	}

	return;
}


int main(int argc, char* argv[])
{
	int i, j, k;
	int gpid, p, N, M;

	int *A, *B, *x, *y, *A_t, *B_t, *C_t;
	double time1, time2;
	int Q;


	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &gpid);
	MPI_Comm_size(MPI_COMM_WORLD, &p);

	MPI_Datatype sblock, sblocktype;
	int disp[p];
	int scount[p];
	int rcount[p];
	
	srand(gpid+1);

	if(argc == 2)
	{
		if(gpid == 0) printf("\n\nnumP=%d, N=%s\n\n", p, argv[1]);
		N = atoi(argv[1]);
	}
	else
	{
		if(gpid == 0)
		{
			printf("Command Line : %s", argv[0]);
			for(i = 1; i < argc; i++)
			{
				printf("%s ", argv[i]);
			}
		}
		MPI_Finalize();
		return 0;
	}

	M = N*p;

	A = (int*) malloc(N*M*sizeof(int));
	B = (int*) malloc(N*M*sizeof(int));

	A_t = (int*) malloc(M*M*sizeof(int));
	B_t = (int*) malloc(M*M*sizeof(int));
	C_t = (int*) malloc(M*M*sizeof(int));

	initVector(A, N*M);
	initVector(B, N*M);



	Q = N*p;
	for(i = 0; i < p; i++)
	{
		disp[i] = i*(N*Q);
		scount[i] = 1;
		rcount[i] = (N*Q);
	}

	MPI_Type_vector(N, Q, M, MPI_INT, &sblock);
	MPI_Type_commit(&sblock);
	MPI_Type_create_resized(sblock, 0, (Q*sizeof(int)), &sblocktype);
	MPI_Type_commit(&sblocktype);

	MPI_Gatherv(A, 1, sblocktype, A_t, rcount, disp, MPI_INT, 0, MPI_COMM_WORLD);
	MPI_Gatherv(B, 1, sblocktype, B_t, rcount, disp, MPI_INT, 0, MPI_COMM_WORLD);


	//for(i = 0; i < 1; i++)
	//{
	//	if(gpid == i) printVector(A_t, M, M, "A_t", gpid);
	//	fflush(stdout);
	//   MPI_Barrier(MPI_COMM_WORLD);
	//	if(gpid == i) printVector(B_t, M, M, "B_t", gpid);
	//	fflush(stdout);
	//   MPI_Barrier(MPI_COMM_WORLD);
	//	
	//	//if(gpid == i) printVector(A, N, M, "A", gpid);
	//	//fflush(stdout);
	//	//MPI_Barrier(MPI_COMM_WORLD);
	//	//if(gpid == i) printVector(B, N, M, "B", gpid);
	//	//fflush(stdout);
	//	//MPI_Barrier(MPI_COMM_WORLD);
	//	printf("\n\n");
	//	fflush(stdout);
	//   MPI_Barrier(MPI_COMM_WORLD);
	//}

	if(gpid == 0)
	{
	   time1 = MPI_Wtime();
		mulMatrix(A_t, B_t, C_t, M, M, Q);
	   time1 = MPI_Wtime() - time1;
		//printVector(C_t, M, M, "C_t", gpid);
		//fflush(stdout);
		free(A_t);
		free(B_t);
		free(C_t);
	}
	MPI_Barrier(MPI_COMM_WORLD);


	Q = N*p;
	x = (int*) malloc(N*Q*p*sizeof(int));
	y = (int*) malloc(N*Q*sizeof(int));
	
	MPI_Barrier(MPI_COMM_WORLD);
	time2 = MPI_Wtime();

	for(i = 0; i < p; i++)
	{
		disp[i] = i*(N*Q);
		scount[i] = 1;
		rcount[i] = (N*Q);
	}

	MPI_Type_vector(N, Q, M, MPI_INT, &sblock);
	MPI_Type_commit(&sblock);
	MPI_Type_create_resized(sblock, 0, (Q*sizeof(int)), &sblocktype);
	MPI_Type_commit(&sblocktype);

	MPI_Allgatherv(&B[0], 1, sblocktype, x, rcount, disp, MPI_INT, MPI_COMM_WORLD);

	mulMatrix(A, x, y, N, M, Q);
	
	MPI_Barrier(MPI_COMM_WORLD);
	time2 = MPI_Wtime() - time2;


	//printf("\n");
	//fflush(stdout);
	//MPI_Barrier(MPI_COMM_WORLD);
	//for(i = 0; i < p; i++)
	//{
	//	//if(gpid == i) printVector(A, N, M, "A", gpid);
	//   //fflush(stdout);
	//	//MPI_Barrier(MPI_COMM_WORLD);
	//	//if(gpid == i) printVector(B, N, M, "B", gpid);
	//   //fflush(stdout);
	//   //MPI_Barrier(MPI_COMM_WORLD);
	//	//if(gpid == i) printVector(x, M, Q, "x", gpid);
	//   //fflush(stdout);
	//   //MPI_Barrier(MPI_COMM_WORLD);
	//   if(gpid == i) printVector(y, N, Q, "y", gpid);
	//   fflush(stdout);
	//	if(gpid == i) printf("\n\n");
	//   fflush(stdout);
	//   MPI_Barrier(MPI_COMM_WORLD);
	//}

	
	if(gpid == 0) printf("%02d : Elapsed Time (SERIAL) is %f\n", gpid, time1);
	if(gpid == 0) printf("%02d : Elapsed Time (PARALLEL) is %f\n", gpid, time2);
	if(gpid == 0) printf("%02d : SPEEDUP (Serial vs Parallel) is %f\n", gpid, time1/time2);

	MPI_Barrier(MPI_COMM_WORLD);
	MPI_Finalize();
	return 0;
}
