/*
 * TO compile
 * mpicc -o hw10 hw10.c
 *
 * TO run on local machine
 *
 * To run on server
 * qsub -q scholar -l nodes=1:ppn=16,walltime=00:02:00 ./hw10.sub
 */




#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct Q
{
	float *q; // Delay in (0-1) seconds.
	int pos;
	int size;
} Q;

void doWork(float t)
{
	int delay;
	delay = (int) (t*1000000);
	usleep(delay); // Sleep in microseconds.
}

struct Q* initWork(int n)
{
	struct Q* newQ = (struct Q*) malloc(sizeof(Q));
	newQ->q = (float*) malloc(n*sizeof(float));
	newQ->pos = -1;
	newQ->size = n-1;
	return newQ;
}

float getWork(struct Q* workQ)
{
	if(workQ->pos > -1)
	{
		float w = workQ->q[workQ->pos];
		workQ->pos--;
		return w;
	}
	else
	{
		return -1;
	}
}

void putWork(struct Q* workQ)
{
	if(workQ->pos < workQ->size)
	{
		workQ->pos++;
		workQ->q[workQ->pos] = (float) rand()/RAND_MAX;
	   //printf("q %0.2f\n", workQ->q[workQ->pos]);
	}
	else
	{
		return;
	}
}

int main (int argc, char *argv[])
{
   int p, numP, i, j, flag, W;
   double time1;
	int rc;
	struct Q* wQ;
	float d, rb, sb, work[16];

   float *msg;

	int rcnt;
	int ids[15];

	MPI_Request reqs[15];
	MPI_Status stats[15];
	int rcvd[16];

   MPI_Init(&argc, &argv);

	MPI_Comm_rank(MPI_COMM_WORLD, &p);
	MPI_Comm_size(MPI_COMM_WORLD, &numP);

	time1 = MPI_Wtime(); // Time after start up (p > 0). time1 reinitialized only for p = 0 later.
	
	if(argc >= 2)
	{
		if(p == 0) printf("\n\nW = %s, numP = %02d\n\n", argv[1], numP);
		W = atoi(argv[1]);
	}
	else
	{
		if(p == 0)
		{
			printf("Command Line : %s\n", argv[0]);
		   for(i = 1; i < argc; i++) printf("%s ", argv[i]);
		}
		MPI_Finalize();
		return 0;
	}

   // Initialize the random seed.
   srand(2);



	//if(p == 0)
	//{
	//	bufsize = MPI_BSEND_OVERHEAD + W*sizeof(int); 
	//	buf = (int*) malloc(bufsize);
	//	MPI_Buffer_attach(buf, bufsize);

	//	msg[0] = 7;
	//	msg[1] = 14;
	//	msg[2] = 16;
	//	msg[3] = 23;
	//	printf("HELLO 0\n");
	//	MPI_Bsend(msg, W, MPI_INT, 1, 1, MPI_COMM_WORLD);
	//	MPI_Buffer_detach(&buf, &bufsize);
	//	printf("HELLO 0\n");
	//}
	//if(p == 1)
	//{
	//	usleep(90000);
	//	usleep(90000);
	//	usleep(90000);
	//	usleep(90000);
	//	printf("HELLO 1\n");
	//	MPI_Recv(msg, W, MPI_INT, 0, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	//	for(i = 0; i < W; i++) printf("%02d ", msg[i]);
	//	printf("\n\n");
	//}


	
   if(p == 0)
	{
		sb = 0;
		rb = 0;
		flag = 0;

		printf("%02d : Hello World\n", p);

		wQ =  initWork(256);
		for(i = 0; i < 256; i++) putWork(wQ);
		for(i = 0; i < 16; i++) rcvd[i] = 1;

		msg = (float*) malloc(16*sizeof(float));

		while(sb != -1)
		{
		   for(i = 1; i < numP; i++)
		   {
		   	if(rcvd[i] == 1)
				{
					rc = MPI_Irecv(&msg[i], 1, MPI_FLOAT, i, i, MPI_COMM_WORLD, &reqs[(i-1)]); // Non-blocking Receive requests from process.
					rcvd[i] = 0;
				}
		   }
	      MPI_Waitsome(15, reqs, &rcnt, ids, stats); // Wait for few completed requests.
			if(flag == 0)
		   {
		   	time1 = MPI_Wtime(); // Time after the first request for process 0.
		   	flag = 1;
		   }
			
		   if(rcnt == MPI_UNDEFINED) printf("ERROR in Irecv!\n");
		   for(i = 0; i < rcnt; i++) // Process work items to send for the few completed requests.
		   {
		   	//printf("%02d : Requesting Work...\n", stats[i].MPI_SOURCE);
				for(j = 0; j < W; j++)
		   	{
		   		sb = getWork(wQ);
		   		if(sb == -1) j = W;
					else rc = MPI_Send(&sb, 1, MPI_FLOAT, stats[i].MPI_SOURCE, stats[i].MPI_SOURCE, MPI_COMM_WORLD); // Send "No More Work" to pending process.
		   	}
		   	if(sb == -1) rcvd[stats[i].MPI_SOURCE] = 0; // disable receive from process which has been notified of "No More Work".
				else rcvd[stats[i].MPI_SOURCE] = 1; // enable receive from process which completed.
		   }
			if(sb == -1)
			{
				for(i = 1; i < numP; i++)
				{
               if(rcvd[i] == 1)
					{
						rc = MPI_Recv(&rb, 1, MPI_FLOAT, i, i, MPI_COMM_WORLD, MPI_STATUSES_IGNORE); // Receive requests from process which got W work items.
		   	      //printf("%02d : Requesting Work...\n", i);
					}
					rc = MPI_Send(&sb, 1, MPI_FLOAT, i, i, MPI_COMM_WORLD); // Send "No More Work" to these process.
				}
			}
		}
		
		rb = -2; // To Bcast "All Done!" to other processes.
	}
	else
	{
		sb = (float) 2*p;
		rb = 0;

		while(rb != -1)
		{
		   rc = MPI_Send(&sb, 1, MPI_FLOAT, 0, p, MPI_COMM_WORLD);
		   for(j = 0; j < W; j++)
		   {
		      rc = MPI_Recv(&rb, 1, MPI_FLOAT, 0, p, MPI_COMM_WORLD, MPI_STATUSES_IGNORE);
				work[j] = rb;			
		   	if(rb == -1)
				{
	            time1 = MPI_Wtime() - time1; // Time after "No More Work!" (p > 0).
					j = W;
				}
			}
			
			//printf("%02d : Work Received.\n", p);
			for(j = 0; j < W; j++)
			{
				if(work[j] == -1)
				{
					j = W;
		   	   //printf("%02d : No More Work!\n", p);
				}
				else
				{
					//printf("%02d : Sleep for %fs\n", p, work[j]);
					doWork(work[j]);
				}
		   }
		}
	}

	rc = MPI_Bcast(&rb, 1, MPI_FLOAT, 0, MPI_COMM_WORLD);
   if(p == 0) time1 = MPI_Wtime() - time1; // Time after "All Done!" (p = 0).

	while(rb != -2);
	if(rb == -2) printf("%02d : DONE! in %fs\n", p, time1); // Completed on receiving broadcast of "All Done".
   

	MPI_Barrier(MPI_COMM_WORLD);
	MPI_Finalize();

   return 0;
}
