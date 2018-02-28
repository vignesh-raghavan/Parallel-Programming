#include <stdio.h>
#include <mpi.h>

// For Block (Second method) distribution
#define BSIZE(p, numP, N)           (((p+1)*N/numP) - (p*N/numP))
#define BINIT(p, numP, N, i)        ((p*N/numP) + i)

int main(int argc, char* argv[])
{
   int p;
   int numP, N;
   int i, j, alen, t, sum, rc;

   int* A;

   double time1, time2;

   MPI_Request reqs[16];
   MPI_Status stats[16];


   MPI_Init(&argc, &argv);

   MPI_Comm_rank(MPI_COMM_WORLD, &p);

   
   if(argc == 3)
   {
      if(p == 0) printf("\n\nCommand Line. : %s numP=%s N=%s\n", argv[0], argv[1], argv[2]);
      //Convert from String to Integer.
      numP = atoi(argv[1]);
      N = atoi(argv[2]);
   }
   else
   {
      if(p == 0)
      {
         printf("Command Line : %s", argv[0]);
         for(i = 1; i < argc; i++) printf(" %s", argv[i]);
      }
      MPI_Finalize();
      exit(1);
   }

   sum = 0;
   MPI_Barrier(MPI_COMM_WORLD);

   if(p < numP)
   {
      alen = BSIZE(p, numP, N);
      A = (int*) malloc(alen*sizeof(int));
      for(i = 0; i < alen; i++)
      {
         A[i] = 1;
      }
   }

   for(j = 0; j < numP; j++)
   {
      if(p == j)
      {
         //printf("%02d:   A[%02d]   = {", p, alen);
         //for(i = 0; i < alen; i++)
         //{
         //   printf(" %03d", A[i]);
         //}
         //printf(" }\n");
      }
   
      MPI_Barrier(MPI_COMM_WORLD);
   }
   MPI_Barrier(MPI_COMM_WORLD);

   time2 = MPI_Wtime();
   MPI_Barrier(MPI_COMM_WORLD);   

   if(p < numP)
   {
      for(j = 0; j < 1000; j++)
      {
         t = 0;
         for(i = 0; i < alen; i++)
         {
            t += A[i];
         }
         
         if(p == 10) MPI_Reduce(MPI_IN_PLACE, &t, 1, MPI_INT, MPI_SUM, 10, MPI_COMM_WORLD);
         else MPI_Reduce(&t, NULL, 1, MPI_INT, MPI_SUM, 10, MPI_COMM_WORLD);
      }
   }

   MPI_Barrier(MPI_COMM_WORLD);   
   time1 = (MPI_Wtime() - time2)/1000;

   if(p == 10) printf("(MPI_Reduce) Time Elapsed (averaged over 1000 runs) for %d size : %f\n", N, time1);
   if(p == 10) printf("p : %02d,   sum : %03d\n", p, t);


   MPI_Barrier(MPI_COMM_WORLD);
   time2 = MPI_Wtime();
   MPI_Barrier(MPI_COMM_WORLD);

   if(p < numP)
   {
      for(j = 0; j < 1000; j++)
      {
         t = 0;
         for(i = 0; i < alen; i++) t += A[i];
       
         if(p%2 == 1) rc = MPI_Send(&t, 1, MPI_INT, (p-1), p, MPI_COMM_WORLD);
         else
         {
            rc = MPI_Recv(&sum, 1, MPI_INT, (p+1), (p+1), MPI_COMM_WORLD, &stats[p]);
            if(j == 0) printf("Recv %02d : %d\n", p, t);
            t += sum;
            if(j == 0) printf("Recv %02d : %d\n", p, t);
         }
         
         if(p%4 == 2) rc = MPI_Send(&t, 1, MPI_INT, (p - 2), p, MPI_COMM_WORLD);
         else if(p%4 == 0)
         {
            rc = MPI_Recv(&sum, 1, MPI_INT, (p+2), (p+2), MPI_COMM_WORLD, &stats[p]);
            t += sum;
            if(j == 0) printf("Recv %02d : %d\n", p, t);
         }

         if(p%8 == 4) rc = MPI_Send(&t, 1, MPI_INT, (p-4), p, MPI_COMM_WORLD);
         else if(p%8 == 0)
         {
            rc = MPI_Recv(&sum, 1, MPI_INT, (p+4), (p+4), MPI_COMM_WORLD, &stats[p]);
            t += sum;
            if(j == 0) printf("Recv %02d : %d\n", p, t);
         }

         if(p%16 == 8) rc = MPI_Send(&t, 1, MPI_INT, (p-8), p, MPI_COMM_WORLD);
         else if(p%16 == 0)
         {
            rc = MPI_Recv(&sum, 1, MPI_INT, (p+8), (p+8), MPI_COMM_WORLD, &stats[p]);
            t += sum;
            if(j == 0) printf("Recv %02d : %d\n", p, t);
         }
      }
   }

   MPI_Barrier(MPI_COMM_WORLD);   
   time1 = (MPI_Wtime() - time2)/1000;

   if(p == 0) printf("(Send/Recv)  Time Elapsed (averaged over 1000 runs) for size %d  : %f\n", N, time1);
   if(p == 0) printf("p : %02d,   sum : %03d\n", p, t);

   MPI_Barrier(MPI_COMM_WORLD);   
   MPI_Finalize();

   return 0;
}
