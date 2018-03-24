#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <queue>
#include <list>
#include <fstream>
#include <iostream>
using namespace std;



// Global Declarations.

//char* files[20] = {
//"CleanText/1.txt",
//"CleanText/2.txt",
//"CleanText/3.txt",
//"CleanText/4.txt",
//"CleanText/5.txt",
//"CleanText/6.txt",
//"CleanText/7.txt",
//"CleanText/8.txt",
//"CleanText/9.txt",
//"CleanText/10.txt",
//"CleanText/11.txt",
//"CleanText/12.txt",
//"CleanText/13.txt",
//"CleanText/14.txt",
//"CleanText/15.txt",
//"CleanText/16.txt",
//"CleanText/17.txt",
//"CleanText/18.txt",
//"CleanText/19.txt",
//"CleanText/20.txt"
//};


char* fnos[20] = {"1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13", "14", "15", "16", "17", "18", "19", "20"};
int findex;

typedef queue<char*, list<char*> > Qreader;
char* fname[20];
char* lines[20];
Qreader rQ[20];
Qreader rQ1[20];

int x[20];
int y[20];
int rsize[20];

int msrc[20];

omp_lock_t l0; // File Index.
omp_lock_t l1;
omp_lock_t l2;
omp_lock_t l3;

typedef queue<int, list<int> > Qid;
Qid rQids;










void pushRQ(Qreader *rQ, char* s, int size)
{
	//printf("%s\n", s);
	char* str = (char*) malloc((size+1)*sizeof(char));
	strcpy(str, s);
	(*rQ).push(str);	
}


int hash(char* x)
{
	int i;
	unsigned int h;
	unsigned long g;

   /* Dan Bernstein Hashing */
	g = 5381;
	for(i = 0; x[i] != '\0'; i++)
	{
		g = ( (33*g) + x[i] );
	}
	g = g%1024;

	h = (unsigned int)g;
	
	return h;
}


char* readFile(string fname, int& cc)
{
	int i, j;
	string line;
	char* fdata;

	ifstream fin;
	fin.open(fname.c_str());

	if(!fin)
	{
		printf("Stale File Handle!\n");
		return NULL;
	}

	i = 0;
	while(!fin.eof())
	{
		getline(fin, line);
		if(line.empty()) continue;
		fdata = (char*) malloc((strlen(line.c_str())+1)*sizeof(char));
		strcpy(fdata, line.c_str());
	}

	cc = strlen(fdata);

	fin.close();
	return fdata;
}


int main(int argc, char* argv[])
{
	int i, j, r1, m1;
	int rdone;
	
	char* files[20];

	for(i = 0; i < 20; i++)
	{
	   files[i] = (char*) malloc(20*sizeof(char));
		strcpy(files[i], "CleanText/");
		strcat(files[i], fnos[i]);
		strcat(files[i], ".txt");
		//printf("%s\n", files[i]);
	}
	
	omp_init_lock(&l0);
	omp_init_lock(&l1);
	omp_init_lock(&l2);
	omp_init_lock(&l3);



	omp_set_num_threads(9);
   #pragma omp parallel
	{
		
		#pragma omp master
		{
			findex = 0;
			r1 = 0;
			m1 = 0;
			rdone = 0;

			for(i = 0; i < 4; i++)
			{
			   #pragma omp task // Reader Threads.
		      {
		      	int rt, rid;
		         rid = omp_get_thread_num();

		         omp_set_lock(&l0);
					rt = r1;
					r1++;
					omp_unset_lock(&l0);

					x[rt] = 0;
					while(findex < 20)
					{
					   omp_set_lock(&l0);
					   if(findex < 20)
					   {
					   	fname[rt] = files[findex];
		               //printf("R %02d : %s \n", rid, fname[rt]);
					      findex++;
				      }
					   omp_unset_lock(&l0);

		            if(fname[rt])
						{
						   lines[rt] = readFile(fname[rt], rsize[rt]);
		               //printf("R %02d : %1d \n", rid, 1);
		               //printf("R %02d : %s  cc (%02d) = %d\n", rid, fname[rt], rt, rsize[rt]);
							pushRQ(&rQ[rt], lines[rt], rsize[rt]);
		               //printf("R %02d : %1d \n", rid, 2);
		               x[rt] += rsize[rt];
		               //printf("R %02d : %1d \n", rid, 3);
							
							omp_set_lock(&l1);
							rQids.push(rt);
							omp_unset_lock(&l1);

		               //printf("R %02d : %1d \n", rid, 4);
						   while(!rQ[rt].empty())
							{
								//rQ[rt].pop();
								omp_set_lock(&l2);
								if(rQ[rt].empty())
								{
		                     //printf("R %02d : %1d \n", rid, 5);
									omp_unset_lock(&l2);
									break;
								}
								omp_unset_lock(&l2);
								usleep(500);
							}

		               //printf("R %02d : %1d \n", rid, 6);
							free(lines[rt]);
		               lines[rt] = NULL;
						}
					}
					omp_set_lock(&l3);
					rdone++;
					omp_unset_lock(&l3);
					printf("R %02d : Total Chars Read (%d)\n", rid, x[rt]);
		      }

			}


			for(j = 0; j < 4; j++)
			{
            #pragma omp task // Mapper Threads.
				{
					int mt, mid;
					mid = omp_get_thread_num();

					omp_set_lock(&l3);
					mt = m1;
					m1++;
					omp_unset_lock(&l3);
					y[mt] = 0;
				
					while(rdone < 4)
					{
						omp_set_lock(&l3);
						if(rdone == 4)
						{
							omp_unset_lock(&l3);
							break;
						}
						omp_unset_lock(&l3);

		            //printf("M %02d : %1d \n", mid, 1);
						omp_set_lock(&l1);
						if(!rQids.empty())
						{
							msrc[mt] = rQids.front();
							rQids.pop();
						}
						else msrc[mt] = -1;
						omp_unset_lock(&l1);
						
		            //printf("M %02d : %1d \n", mid, 2);
						if(msrc[mt] > -1)
						{
							omp_set_lock(&l2);
							swap(rQ[msrc[mt]], rQ1[mt]);
							omp_unset_lock(&l2);

		               //printf("M %02d : %1d \n", mid, 3);
							y[mt] += strlen(rQ1[mt].front());
							while(!rQ1[mt].empty())
							{
								free(rQ1[mt].front());
								rQ1[mt].pop();
							}
						   //printf("M %02d : FROM (%02d) cc %d $$\n", mid, msrc[mt], y[mt]);
						}
						else usleep(500);
						
		            //printf("M %02d : %1d \n", mid, 4);
						//for(; k < 4;k++) 
						//{
						//   omp_set_lock(&l1);
						//	if(!rQ[k].empty())
						//	{
						//		swap(rQ[k], rQ1[mt]);
						//		omp_unset_lock(&l1);
						//		y[mt] = strlen(rQ1[mt].front());
						//		while(!rQ1[mt].empty()) rQ1[mt].pop();
						//      printf("M %02d : $$ src (%02d) chars %d $$\n", mt, k, y[mt]);
						//		k++;
						//		break;
						//	}
						//   omp_unset_lock(&l1);
						//	k++;
						//}
						//k = k%4;
						//usleep(5000);
					}
					printf("M %02d : Total Chars Read (%d)\n", mid, y[mt]);
				}
			}
		}
	}




	/*
	//omp_set_num_threads(2);
   //#pragma omp parallel
	for(j = 0; j < 2; j+=1)
	{
		int rt;
		rt = j;
		//rt = omp_get_thread_num();
	   
	   fname[rt] = files[rt];
	   lines[rt] = readFile(fname[rt]);
		printf("wc (%s) = %d\n", fname[rt], strlen(lines[rt]));
		pushRQ(&rQ[rt], lines[rt]);
		//omp_set_lock(&l);
		//x = strlen(rQ[rt].front());
		//omp_unset_lock(&l);
		//printf("rQ (%s) = %d\n", fname[rt], x);
		//while(!rQ[rt].empty()) rQ[rt].pop();
		free(lines[rt]);
		lines[rt] = NULL;
	}*/

	omp_destroy_lock(&l0);
	omp_destroy_lock(&l1);
	omp_destroy_lock(&l2);
	omp_destroy_lock(&l3);

	return 0;
}
