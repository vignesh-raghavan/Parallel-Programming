#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <queue>
#include <list>
#include <vector>
#include <fstream>
using namespace std;


// Global Declarations.
int nReaders;
int nMappers;
int nReducers;
int nWriters;

char* fname[20]; // Dynamic File names for Reader.
char* lines[20]; // Single Line in a file.

char* mdata[20]; // Store a Long String in Mapper.

typedef queue<char*, list<char*> > Qreader;
Qreader rQ1;

int x[20]; // Total Chars Read for Reader.
int y[20]; // Total Chars Read for Mapper.
int tc1;
int tc2;
int uw1;
int uw2;
int rsize[20]; // Chars Read per file for Reader.

int msrc[20]; // SRC queue number for Mapper.
int csrc[20]; // SRC queue number for Mapper.

omp_lock_t l0;
omp_lock_t l1;
omp_lock_t l2;
omp_lock_t l3;
omp_lock_t l4;
omp_lock_t l5;
omp_lock_t l6;
omp_lock_t l7;

typedef queue<int, list<int> > Qid;
Qid fQids; // File Queue numbers for Reader.
Qid rQids; // Reader Queue numbers for Mapper.
Qid mQids; // Mapper Queue numbers for Reducer.
Qid cQids; // Mapper Queue numbers for Reducer.

Qid ENDr; // Reader END detection for Mapper.
Qid ENDm[20]; // Mapper END detection for Reducer.


// <word, word-count> as Record.
typedef struct record
{
	char* words;
	int wc;
} record;

// List of records.
typedef vector<record> Lrecords;

// A structure of 128 record list.
typedef struct wfreq
{
	Lrecords wmap[128];
} wfreq;

// Record List Array for each mapper.
wfreq W[20];

// Record List Array for each reducer.
Lrecords Crecords[20];
//Lrecords Urecords[20];



// Helper Functions.
void pushRQ(Qreader *rQ, char* s)
{
	char* str = (char*) malloc((strlen(s)+1)*sizeof(char));
	strcpy(str, s);
	
	(*rQ).push(str);	
}

char* popRQ(Qreader *rQ)
{
	char* str;
	str = (char*) malloc( (strlen((*rQ).front())+1) * sizeof(char) );
	strcpy(str, (*rQ).front());
	
	free((*rQ).front());
	(*rQ).pop();
	//if((*rQ).empty()) printf("RQ Empty\n");
	return str;
}

int isRQfull(Qreader *rQ)
{
	//if((*rQ).size() >= 1) return 1;
	if((*rQ).size() >= (nReaders)) return 1;
	else return 0;
}


int H(char* x)
{
	int i;
	unsigned int h;
	unsigned long g;

   /* Dan Bernstein Hashing */
	g = 5381;
	for(i = 0; x[i] != '\0'; i++)
	{
		g = ( (33*g) + tolower(x[i]) );
	}
	g = g%128;
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


void DestroyWordFrequency(int mt)
{
	int i;
	Lrecords::iterator it1;
	Lrecords::iterator it2;
	
	for(i = 0; i < 128; i++)
	{
		it1 = W[mt].wmap[i].begin();
		it2 = W[mt].wmap[i].end();

		while(it1 != it2)
		{
			free((*it1).words);
			(*it1).words = NULL;
			//printf("%s\n", (*it1).words);
			++it1;
		}
		W[mt].wmap[i].clear();
	}
}


void CreateWordFrequency(int mt)
{
	char* word;
	char* sentence;
	char* split;
	unsigned int key;
	record t1;
	//int found;
	Lrecords::iterator it1;
	Lrecords::iterator it2;
	int i;
	int nUniqueWords;

	nUniqueWords = 0;
	sentence = (char*) malloc((strlen(mdata[mt]) + 1)*sizeof(char));
	strcpy(sentence, mdata[mt]);
	
	// GNU C compiler defines a re-entrant MT-Safe String to Token function.
	// strtok was not MT-Safe and hence, was causing issues.
	split = strtok_r(sentence, " ", &word);
	while(split != NULL)
	{
		// key should also ignore case.
		key = H(split);
		
		it1 = W[mt].wmap[key].begin();
		it2 = W[mt].wmap[key].end();

		//found = 0;
		while(it1 != it2)
		{
			// GNU C compiler does not support strcmpi or stricmp functions.
			if(strcasecmp((*it1).words, split) == 0)
			{
				//found = 1;
				(*it1).wc += 1;
				break;
			}
			++it1;
		}
		if(it1 == it2)
		{
		   //printf("%s\n", split);
			t1.words = (char*) malloc((strlen(split) + 1) * sizeof(char));
			strcpy(t1.words, split);
			//t1.words = split;
			t1.wc = 1;
			W[mt].wmap[key].push_back(t1);
			++nUniqueWords;
		}

		split = strtok_r(NULL, " ", &word);
	}

	omp_set_lock(&l3);
	uw1 += nUniqueWords;
	omp_unset_lock(&l3);

	free(sentence);
	printf("Map %02d : Unique Words (%d)\n", omp_get_thread_num(), nUniqueWords);
}


void MapRecordsAndReduce(int mt, int ct)
{
	Lrecords::iterator it1;
	Lrecords::iterator it2;
	record t1;
	int i;
	Lrecords::iterator it3;
	Lrecords::iterator it4;

	for(i = ct; i < 128; i = i+nReducers)
	{
		it1 = W[mt].wmap[i].begin();
		it2 = W[mt].wmap[i].end();

		while(it1 != it2)
		{
			it3 = Crecords[ct].begin();
			it4 = Crecords[ct].end();

			while(it3 != it4)
			{
				if(strcasecmp((*it1).words, (*it3).words) == 0)
				{
					(*it3).wc += (*it1).wc;
					break;
				}
				++it3;
			}
			if(it3 == it4)
			{
		      t1.words = (char*) malloc((strlen((*it1).words)+1) * sizeof(char));
		      strcpy(t1.words, (*it1).words);
		      t1.wc = (*it1).wc;
		      Crecords[ct].push_back(t1);
			}
		   ++it1;
		}
	}
}

/* Not USED Anymore...
void ReduceRecords(int ct)
{
	Lrecords::iterator it1;
	Lrecords::iterator it2;
	Lrecords::iterator it3;
   int i;

	i = 0;

	it1 = Crecords[ct].begin();
	it3 = Crecords[ct].end();
	
	while(it1 != it3)
	{
		if((*it1).wc > 0)
		{
			it2 = it1+1;
		   while(it2 != it3)
		   {
				if((*it2).wc > 0)
				{
			      if(strcasecmp((*it1).words, (*it2).words) == 0)
		   	   {
		   	   	(*it1).wc += (*it2).wc;
		   	   	(*it2).wc = 0;
		   	   }
				}
		   	++it2;
		   }
		   ++i;
		}
		++it1;
	}
	//printf("%02d, %d\n", omp_get_thread_num(), i);

	i = 0;
	it1 = Crecords[ct].begin();
	it3 = Crecords[ct].end();

	while(it1 != it3)
	{
		if((*it1).wc > 0)
		{
			Urecords[ct].push_back((*it1));
		   ++i;
		}
		++it1;
	}
	//printf("%02d, %d\n", omp_get_thread_num(), i);
}*/


void DestroyReducerRecords(int ct)
{
	Lrecords::iterator it1;
	Lrecords::iterator it2;

	//omp_set_lock(&l5);
	it1 = Crecords[ct].begin();
	it2 = Crecords[ct].end();

	while(it1 != it2)
	{
		free((*it1).words);
		(*it1).words = NULL;
		++it1;
	}
	Crecords[ct].clear();
	//omp_unset_lock(&l5);
}

// Debug Helper Function for verifying the correctness of implementation.
int FindRecord(char* test, int ct)
{
	Lrecords::iterator it1;
	Lrecords::iterator it2;

	it1 = Crecords[ct].begin();
	it2 = Crecords[ct].end();

	while(it1 != it2)
	{
		if(strcasecmp((*it1).words, test) == 0)
		{
			return (*it1).wc;
		}
		++it1;
	}
	return 0;
}

int main(int argc, char* argv[])
{
	int i, j, k, l, r1, m1, c1, w1;
	int rdone;
	int mdone[20];
	int cdone;
	int q[20];
	
	char fid[2];
	char* test;
	int check;
	char* files[20];
	char* ofiles[20];

	for(i = 0; i < 20; i++)
	{
	   files[i] = (char*) malloc(20*sizeof(char));
		strcpy(files[i], "CleanText/");
		sprintf(fid, "%d", (i+1));
		strcat(files[i], fid);
		strcat(files[i], ".txt");
		//printf("%s\n", files[i]);
		fQids.push(i);
		ofiles[i] = (char*) malloc(20*sizeof(char));
		strcpy(ofiles[i], "Output/");
		strcat(ofiles[i], fid);
		strcat(ofiles[i], ".o");
		//printf("%s\n", ofiles[i]);
	   //test = files[i];
	   //printf("Hash : %d\n", H(test));
	}

	
	omp_init_lock(&l0);
	omp_init_lock(&l1);
	omp_init_lock(&l2);
	omp_init_lock(&l3);
	omp_init_lock(&l4);
	omp_init_lock(&l5);
	omp_init_lock(&l6);
	omp_init_lock(&l7);



	omp_set_num_threads(16);
   #pragma omp parallel
	{
		#pragma omp master
		{
			r1 = 0;
			m1 = 0;
			c1 = 0;
			w1 = 0;
			uw1 = 0;
			uw2 = 0;
			rdone = 0;
			cdone = 0;
			nReaders = 2;
			nMappers = 4;
			nReducers = 8;
			nWriters = 1;
			printf("Master %02d : ", omp_get_thread_num());
			printf("nReaders (%02d); ", nReaders);
         printf("nMappers (%02d); ", nMappers);
			printf("nReducers (%02d); ", nReducers);
			printf("nWriters (%02d)\n", nWriters);

			for(i = 0; i < nReaders; i++)
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
					while(!fQids.empty())
					{
					   omp_set_lock(&l0);
					   if(!fQids.empty())
					   {
					   	fname[rt] = files[ fQids.front() ];
		               fQids.pop();
							//printf("Read %02d : %s \n", rid, fname[rt]);
				      }
					   omp_unset_lock(&l0);

		            if(fname[rt])
						{
						   lines[rt] = readFile(fname[rt], rsize[rt]);
		               //printf("Read %02d : %s  cc (%02d) = %d\n", rid, fname[rt], rt, rsize[rt]);

							omp_set_lock(&l1);
							while(isRQfull(&rQ1))
							{
								usleep(500);
							}
							if(!isRQfull(&rQ1))
							{
								pushRQ(&rQ1, lines[rt]);
							}
							omp_unset_lock(&l1);

							omp_set_lock(&l2);
							rQids.push(rt);
							omp_unset_lock(&l2);

		               x[rt] += rsize[rt];
							
							free(lines[rt]);
		               lines[rt] = NULL;
						}
					}

					omp_set_lock(&l3);
					ENDr.push(rt);
					tc1 += x[rt];
					omp_unset_lock(&l3);
					printf("Read %02d : Total Chars (%d)\n", rid, x[rt]);
		      }

			}


			for(j = 0; j < nMappers; j++)
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
				
					while(rdone < nReaders)
					{
						omp_set_lock(&l3);
						if(!ENDr.empty())
						{
							++rdone;
							ENDr.pop();
						}
						omp_unset_lock(&l3);

						while(!rQ1.empty())
						{
						   omp_set_lock(&l4);
						   if(!rQ1.empty())
						   {
						   	mdata[mt] = popRQ(&rQ1);
					         tc2 += strlen(mdata[mt]);
						   }
						   omp_unset_lock(&l4);

						   if(mdata[mt] != NULL)
						   {
								// For Debug.
						   	omp_set_lock(&l2);
						   	msrc[mt] = rQids.front();
						   	rQids.pop();
						   	omp_unset_lock(&l2);

						   	y[mt] += strlen(mdata[mt]);

						   	CreateWordFrequency(mt);

						      //printf("Map %02d : FROM (%02d) cc %d $$\n", mid, msrc[mt], strlen(mdata[mt]));
                     	free(mdata[mt]);
                     	mdata[mt] = NULL;
						   }
						}
						usleep(500);
					}
					
					omp_set_lock(&l4);
					for(q[mt] = 0; q[mt] < nReducers; q[mt] += 1)
					{
						ENDm[q[mt]].push(mt);
					}
					omp_unset_lock(&l4);
					printf("Map %02d :  Total Chars (%d)\n", mid, y[mt]);
				}
			}


			for(k = 0; k < nReducers; k++)
			{
            #pragma omp task // Reducer Threads.
				{
					int ct, cid;
					cid = omp_get_thread_num();

					omp_set_lock(&l4);
					ct = c1;
					++c1;
					omp_unset_lock(&l4);

					mdone[ct] = 0;
					while(mdone[ct] < nMappers)
					{
						omp_set_lock(&l4);
						if(!ENDm[ct].empty())
						{
							++mdone[ct];
							csrc[ct] = ENDm[ct].front();
							ENDm[ct].pop();
						}
						else csrc[ct] = -1;
						omp_unset_lock(&l4);

						if(csrc[ct] > -1)
						{
							//printf("C %02d : From %d\n", cid, csrc[ct]);
							MapRecordsAndReduce(csrc[ct], ct);
						}
						else usleep(500);
					}
					
					//ReduceRecords(ct);

					omp_set_lock(&l5);
					cQids.push(ct);
					omp_unset_lock(&l5);
				
					printf("Reduce %02d : Reduced Words (%d)\n", cid, Crecords[ct].size());
				}
			}

			
			/*for(l = 0; l < nWriters; l++)
			{
            #pragma omp task // Writer Threads.
				{
					int wid, wt;
					wid = omp_get_thread_num();

					omp_set_lock(&l6);
					wt = w1;
					++w1;
					omp_unset_lock(&l6);

					while(cdone < nReducers)
					{
						omp_set_lock(&l6);
						if(cdone == nReducers)
						{
							while(!cQids.empty()) cQids.pop();
							omp_unset_lock(&l6);
							break;
						}
						omp_unset_lock(&l6);

						omp_set_lock(&l6);
						if(!cQids.empty())
						{
							wsrc[wt] = cQids.front();
							cQids.pop();
						}
						else wsrc[wt] = -1;
						omp_unset_lock(&l6);

						if(wsrc[wt] > -1)
						{
							while(!Urecords[wsrc[wt]].empty())
							{
								Urecords[wsrc[wt]].clear();
							}
						}
						else usleep(500);
					}
					
					printf("Writer %02d : %02d\n", wid, wt);
				}
			}*/
		}
	}

	printf("Total Characters : (Reader) %d = (Mapper) %d\n", tc1, tc2);
	for(i = 0; i < nMappers; i++)
	{
		DestroyWordFrequency(i);
	}

	test = (char*) malloc(10*sizeof(char));
	strcpy(test, "ThE");
	for(i = 0; i < nReducers; i++)
	{
		uw2 += Crecords[i].size();
		check = FindRecord(test, i);
		printf("CHECK %02d : wc(THE) = %d\n", i, check);
		DestroyReducerRecords(i);
	}
	free(test);

	printf("Total Words : (Mapper) %d -> (Reducer) %d\n", uw1, uw2);
	for(i = 0; i < 20; i++) free(files[i]);
	for(i = 0; i < 20; i++) free(ofiles[i]);

	omp_destroy_lock(&l0);
	omp_destroy_lock(&l1);
	omp_destroy_lock(&l2);
	omp_destroy_lock(&l3);
	omp_destroy_lock(&l4);
	omp_destroy_lock(&l5);
	omp_destroy_lock(&l6);
	omp_destroy_lock(&l7);

	return 0;
}




//void MapRecordsToReducers(int mt, int ct)
//{
//	Lrecords::iterator it1;
//	Lrecords::iterator it2;
//	record t1;
//	int i;
//
//	for(i = ct; i < 128; i = i+nReducers)
//	{
//		it1 = W[mt].wmap[i].begin();
//		it2 = W[mt].wmap[i].end();
//
//		while(it1 != it2)
//		{
//		   t1.words = (char*) malloc((strlen((*it1).words)+1) * sizeof(char));
//		   strcpy(t1.words, (*it1).words);
//		   t1.wc = (*it1).wc;
//		   Crecords[ct].push_back(t1);
//		   ++it1;
//		}
//	}
//}
