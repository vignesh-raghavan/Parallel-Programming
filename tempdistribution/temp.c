#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <mpi.h>

double findAvg( size_t cols, double (*strip)[cols], size_t rows)
{
   int i, j;
	double tot = 0.0;
   for (i = 1; i < (rows-1); i++)
   //for (j = 1; j < (cols-1); j++)
	{
      for (j = 1; j < (cols-1); j++)
      //for (i = 1; i < (rows-1); i++)
		{
         tot += strip[i][j];
      }
   }
   tot = tot / ((cols - 2)*(rows-2));
   return tot;
}


int main(int argc, char* argv[])
{
	int i, j;
   size_t rows, cols;
   rows = 1002;
   cols = 30002;
   double time1;

	MPI_Init(&argc, &argv);
	printf("\n\nnumP = 01\n");

	//rows = 6;
	//cols = 18;
   double (*strip)[cols] = malloc(sizeof *strip * rows);

	//for(i = 0; i < cols; i++) strip[i] = (double*) malloc(rows*sizeof(double));

   if (strip)
	{ // make sure the malloc worked   

      // init the interior nodes
      for (i=1; i < (rows-1); i++)
		{
         for (j=1; j < (cols-1); j++)
			{
            strip[i][j] = 0.0;
         }
      }
      
      // init the top and bottom row to be hot
      for (j = 0; j < cols; j++)
		{
         strip[0][j] = 100.0;
         strip[rows-1][j] = 100.0;
      }
   
      // init the left and right column to be cold.
      for (i = 0; i < rows; i++)
		{
         strip[i][0] = -50.0;
         strip[i][cols-1] = -50.0;
      }
   
      // iterate repeatedly until the temperature converges. 
      double avg = 0.0;
      double lastAvg = -100;
      int count = 0;
   
		time1 = MPI_Wtime();

      while (fabs(avg - lastAvg) > 0.005)
		{
         for (i=1; i < (rows-1); i++)
			{
            for (j=1; j < (cols-1); j++)
				{
               strip[i][j] = (strip[(i-1)][j] + strip[(i+1)][j] + strip[i][(j-1)] + strip[i][(j+1)])/4.0; 
            }
         }
         lastAvg = avg;
         avg = findAvg(cols, strip, rows);
         count++;
         //printf("%02d : %f\n", count, avg);
      }
		time1 = MPI_Wtime() - time1;

      printf("Final Average %f, count %02d\n", avg, count);
		printf("Elapsed Time is %fs\n", time1);

		//for(i = 0; i < rows; i++)
		//{
		//	for(j = 0; j < cols; j++)
		//	{
		//		printf("%3.1f   ", strip[i][j]);
		//	}
		//	printf("\n");
		//}

   }
	else
	{ // go here if the malloc failed
      printf("malloc error\n");
   }

	MPI_Barrier(MPI_COMM_WORLD);
	MPI_Finalize();
	return 0;
}
