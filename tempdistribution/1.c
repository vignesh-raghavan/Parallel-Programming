#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <math.h>

// For Block (Second method) distribution
#define BSIZE(p, numP, N)           (((p+1)*N/numP) - (p*N/numP))
#define BINIT(p, numP, N, i)        ((p*N/numP) + i)

int main(int argc, char* argv[])
{
   int lpid, lsize, N;
	int gpid, gsize;
   int i, j, alen, lsum, gsum, rc;

   int* A;

   double time1;

	int L_color;

   MPI_Comm L_comm;

   MPI_Init(&argc, &argv);
   MPI_Comm_rank(MPI_COMM_WORLD, &gpid);
   MPI_Comm_size(MPI_COMM_WORLD, &gsize);
 
	int iter = (int)sqrt(gsize);
	int exp;
	//printf("%02d", iter);
	
	lpid = gpid;
	lsize = gsize;

   if(argc == 2)
   {
      if(gpid == 0) printf("\n\nnumP=%d, N=%s\n\n", gsize, argv[1]);
      //Convert from String to Integer.
      N = atoi(argv[1]);
   }
   else
   {
      if(gpid == 0)
      {
         printf("Command Line : %s", argv[0]);
         for(i = 1; i < argc; i++) printf(" %s", argv[i]);
      }
      MPI_Finalize();
		return 0;
   }

	lsum = 0;
	gsum = 0;
	alen = BSIZE(gpid, gsize, N);
	
	A = (int*) malloc(alen*sizeof(int));
	for(i = 0; i < alen; i++)
	{
		A[i] = 1;
	}
	
	MPI_Barrier(MPI_COMM_WORLD);
	time1 = MPI_Wtime();

	for(i = 0; i < alen; i++)
	{
		lsum += A[i];
	}
	

	//for(i = 0; i < iter; i++)
	//{
	//	  exp = pow(2, i);
	//   L_color = gpid/(2*exp);
	//	  L_color = exp*L_color + (gpid%exp);

	//   MPI_Comm_split(MPI_COMM_WORLD, L_color, gpid, &L_comm);
	//   MPI_Comm_rank(L_comm, &lpid);
	//   MPI_Comm_size(L_comm, &lsize);

	//   //printf("%02d/%02d \t %02d/%02d\n", gpid, gsize, lpid, lsize);
	//   MPI_Allreduce(&lsum, &gsum, 1, MPI_INT, MPI_SUM, L_comm);

	//   MPI_Comm_free(&L_comm);
	//   //printf("%02d/%02d \t %02d/%02d\n", gpid, gsize, lpid, lsize);
	//   lsum = gsum;
	//   MPI_Barrier(MPI_COMM_WORLD);
	//}

	exp = 1;
	for(i = 0; i < iter; i++)
	{
	   if( (gpid%(2*exp)) >= exp )
	   {
	   	MPI_Send(&lsum, 1, MPI_INT, (gpid-exp), (gpid-exp), MPI_COMM_WORLD);
	   	//printf("Send %02d -> %02d\n", gpid, (gpid-exp));
	   	MPI_Recv(&gsum, 1, MPI_INT, (gpid-exp), gpid, MPI_COMM_WORLD, MPI_STATUSES_IGNORE);
	   }
	   else
	   {
	   	MPI_Recv(&gsum, 1, MPI_INT, (gpid+exp), gpid, MPI_COMM_WORLD, MPI_STATUSES_IGNORE);
	   	//printf("Send %02d -> %02d\n", gpid, (gpid+exp));
	   	MPI_Send(&lsum, 1, MPI_INT, (gpid+exp), (gpid+exp), MPI_COMM_WORLD);
	   }
	  	lsum += gsum;
		gsum = lsum;
		exp = 2*exp;
	}

	time1 = MPI_Wtime() - time1;

	if(gpid == 0) printf("%02d : Elapsed Time is %fs\n", gpid, time1);
	
	MPI_Barrier(MPI_COMM_WORLD);
	printf("pid %02d has gsum %d\n", gpid, gsum);

	MPI_Barrier(MPI_COMM_WORLD);
   MPI_Finalize();

   return 0;
}
