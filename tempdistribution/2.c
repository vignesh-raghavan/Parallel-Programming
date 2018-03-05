/*
 * To run in scholar machine (numP = 2, 4, 8 and 16 are run back-to-back).
 * qsub -q scholar -l nodes=1:ppn=16,walltime=00:05:00 2.sub
 *
 */

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <mpi.h>

double findAvg(int r, double* strip[r], int c)
{
	int i, j;
	double tot = 0.0;

	for(i = 1; i < (r-1); i++)
	{
		for(j = 1; j < (c-1); j++)
		{
			tot += strip[i][j];
		}
	}
	tot = tot/((r-2)*(c-2));

	return tot;
}


int main(int argc, char* argv[])
{
	int i, j, k, rows, cols, r, c, p, numP, count;
	double avg, lastavg, lerror, gerror, gavg;
	int prev, next;
	int tag1 = 1;
	int tag2 = 2;
	double time1;

	rows = 1002;
	cols = 30002;

	//rows = 102;
	//cols = 802;
	
	double* strip[rows];
	double* slbuf;
	double* rlbuf;
	double* srbuf;
	double* rrbuf;
	
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &p);
	MPI_Comm_size(MPI_COMM_WORLD, &numP);

	MPI_Request reqs[4];
	MPI_Status stats[4];

	if(p == 0) printf("\n\nnumP = %02d\n", numP);

	r = rows;
	c = (cols-2)/numP + 2; //Assume cols-2 is divisible by 16.

	gavg = 0;
	avg = 0;
	lastavg = -100;
	gerror = fabs(avg-lastavg);
	count = 0;
	
	prev = p-1;
	next = p+1;
	if(p == 0) prev = numP-1;
	if(p == (numP-1)) next = 0;
	
	//printf("%02d : prev %02d,  next %02d\n", p, prev, next);
	//printf("%02d : %02d \n", r, c);

	for(i = 0; i < r; i++)
	{
		strip[i] = (double*) malloc(c * sizeof(double));
	}
	slbuf = (double*) malloc(r * sizeof(double)); // Buffers for sending.
	srbuf = (double*) malloc(r * sizeof(double)); // Buffers for sending.
	rlbuf = (double*) malloc(r * sizeof(double)); // Buffers for receiving.
	rrbuf = (double*) malloc(r * sizeof(double)); // Buffers for receiving.

	for(i = 0; i < r; i++)
	{
		slbuf[i] = (double) p;
		srbuf[i] = (double) p;
	}

	for(i = 0; i < r; i++)
	{
		for(j = 0; j < c; j++)
		{
			strip[i][j] = 0.0;
		}
	}

	for(j = 1; j < (c-1); j++)
	{
		strip[0][j]     = 100.0;
		strip[(r-1)][j] = 100.0;
	}

	for(i = 0; i < r; i++)
	{
		if(p == 0)
		{
			strip[i][0] = -50.0;
		}
		else if(p == (numP-1))
		{
			strip[i][(c-1)] = -50.0;
		}
	}

	MPI_Barrier(MPI_COMM_WORLD);
	time1 = MPI_Wtime();

	//for(k = 0; k < 57; k++)
	while( gerror > 0.005 )
	{
	   // Computations start here...
	   for(i = 1; i < (r-1); i++)
	   {
	   	for(j = 1; j < (c-1); j++)
	   	{
	   		strip[i][j] = (strip[(i-1)][j] + strip[(i+1)][j] + strip[i][(j-1)] + strip[i][(j+1)])/4.0;
	   	}
	   }
	   
	   lastavg = gavg;
	   avg = findAvg(r, strip, c);
		//lerror = fabs(avg-lastavg);
	   count++;	
	   
	   for(i = 0; i < r; i++)
	   {
	   	slbuf[i] = strip[i][1];
	   	srbuf[i] = strip[i][(c-2)];
	   }

	   MPI_Irecv(rrbuf, r, MPI_DOUBLE, next, tag1, MPI_COMM_WORLD, &reqs[0]);
	   MPI_Irecv(rlbuf, r, MPI_DOUBLE, prev, tag2, MPI_COMM_WORLD, &reqs[1]);
	   
	   MPI_Isend(srbuf, r, MPI_DOUBLE, next, tag2, MPI_COMM_WORLD, &reqs[2]);
	   MPI_Isend(slbuf, r, MPI_DOUBLE, prev, tag1, MPI_COMM_WORLD, &reqs[3]);

	   MPI_Waitall(4, reqs, stats);
	   
	   for(i = 0; i < r; i++)
	   {
	   	if(p > 0)        strip[i][0]     = rlbuf[i];
	   	if(p < (numP-1)) strip[i][(c-1)] = rrbuf[i];
	   }
	
	   //MPI_Barrier(MPI_COMM_WORLD);
		//MPI_Allreduce(&lerror, &gerror, 1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);
		MPI_Allreduce(&avg, &gavg, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
		gavg = gavg/numP;
		gerror = fabs(gavg - lastavg);
		//if(count%100 == 0)
		//{
		//	if(p == 0) printf("%02d : %f, %f\n", count, gavg, gerror);
		//}
	}

	if(p == 0)
	{
		printf("Final Average %f, count %02d\n", gavg, count);
	}

	MPI_Barrier(MPI_COMM_WORLD);
	time1 = MPI_Wtime() - time1;

	if(p == 0) printf("Elapsed Time is %fs", time1);

	//MPI_Barrier(MPI_COMM_WORLD);
	//for(k = 0; k < numP; k++)
	//{
	//	if(k == p)
	//	{
	//		printf("\n\np%02d : \n", p);
	//		for(i = 0; i < r; i++)
	//		{
	//			//printf("%f  ", rlbuf[i]);
	//			for(j = 0; j < c; j++)
	//			{
	//				printf("%3.1f ", strip[i][j]);
	//			}
	//			printf("\n");
	//		}
	//		//printf("\np%02d : ", p);
	//		//for(i = 0; i < r; i++)
	//		//{
	//		//	printf("%f ", rrbuf[i]);
	//		//}
	//	}
	//   MPI_Barrier(MPI_COMM_WORLD);
	//}


	MPI_Barrier(MPI_COMM_WORLD);
	MPI_Finalize();

	return 0;
}
