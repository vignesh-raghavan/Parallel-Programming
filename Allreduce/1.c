#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

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

	int L0_color, L1_color, L2_color, L3_color;

	MPI_Comm L0_comm;
	MPI_Comm L1_comm;
	MPI_Comm L2_comm;
	MPI_Comm L3_comm;

   MPI_Init(&argc, &argv);
   MPI_Comm_rank(MPI_COMM_WORLD, &gpid);
   MPI_Comm_size(MPI_COMM_WORLD, &gsize);
 
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
	
	//lsum = gpid*gpid;

	L0_color = gpid/2;
	MPI_Comm_split(MPI_COMM_WORLD, L0_color, gpid, &L0_comm);
   MPI_Comm_rank(L0_comm, &lpid);
   MPI_Comm_size(L0_comm, &lsize);

	//printf("%02d/%02d \t %02d/%02d\n", gpid, gsize, lpid, lsize);
	MPI_Allreduce(&lsum, &gsum, 1, MPI_INT, MPI_SUM, L0_comm);

	MPI_Comm_free(&L0_comm);
	//printf("L0 %02d : lsum = %5d,  gsum = %5d\n", gpid, lsum, gsum);
	lsum = gsum;
   MPI_Barrier(MPI_COMM_WORLD);



	L1_color = gpid/4;
	L1_color = 4*L1_color + (gpid%2);
	MPI_Comm_split(MPI_COMM_WORLD, L1_color, gpid, &L1_comm);
   MPI_Comm_rank(L1_comm, &lpid);
   MPI_Comm_size(L1_comm, &lsize);
	
	//printf("%02d/%02d \t %02d/%02d\n", gpid, gsize, lpid, lsize);
	MPI_Allreduce(&lsum, &gsum, 1, MPI_INT, MPI_SUM, L1_comm);
	
	MPI_Comm_free(&L1_comm);
	//printf("L1 %02d : lsum = %5d,  gsum = %5d\n", gpid, lsum, gsum);
	lsum = gsum;
	MPI_Barrier(MPI_COMM_WORLD);

	if(gsize == 16)
	{
	   L2_color = gpid/8;
	   L2_color = 8*L2_color + (gpid%4);

	   MPI_Comm_split(MPI_COMM_WORLD, L2_color, gpid, &L2_comm);
      MPI_Comm_rank(L2_comm, &lpid);
      MPI_Comm_size(L2_comm, &lsize);
	   
	   //printf("%02d/%02d \t %02d/%02d\n", gpid, gsize, lpid, lsize);
	   MPI_Allreduce(&lsum, &gsum, 1, MPI_INT, MPI_SUM, L2_comm);
	   
	   MPI_Comm_free(&L2_comm);
	   //printf("L2 %02d : lsum = %5d,  gsum = %5d\n", gpid, lsum, gsum);
	   lsum = gsum;
	   MPI_Barrier(MPI_COMM_WORLD);
	   
		
		
		L3_color = gpid%8;

		MPI_Comm_split(MPI_COMM_WORLD, L3_color, gpid, &L3_comm);
      MPI_Comm_rank(L3_comm, &lpid);
      MPI_Comm_size(L3_comm, &lsize);
	   
		//printf("%02d/%02d \t %02d/%02d\n", gpid, gsize, lpid, lsize);
	   MPI_Allreduce(&lsum, &gsum, 1, MPI_INT, MPI_SUM, L3_comm);
	   
		MPI_Comm_free(&L3_comm);
	   //printf("L3 %02d : lsum = %5d,  gsum = %5d\n", gpid, lsum, gsum);
	   lsum = gsum;
	   MPI_Barrier(MPI_COMM_WORLD);
	}

	time1 = MPI_Wtime() - time1;

	if(gpid == 0) printf("%02d : Elapsed Time is %fs\n", gpid, time1);
	
	MPI_Barrier(MPI_COMM_WORLD);
	printf("pid %02d has sum %d\n", gpid, gsum);

	MPI_Barrier(MPI_COMM_WORLD);
   MPI_Finalize();

   return 0;
}
