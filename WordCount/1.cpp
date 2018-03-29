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

// Deprecated in C++.
//char* fnos[20] = {"1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13", "14", "15", "16", "17", "18", "19", "20"};

char* fname[20]; // Dynamic File names for Reader.
char* lines[20]; // Single Line in a file.

char* mdata[20]; // Store a Long String in Mapper.

typedef queue<char*, list<char*> > Qreader;
Qreader rQ1[20]; // Queue for Reader.
Qreader rQ2[20]; // Exchange Queue Mapper-Reader.

int x[20]; // Total Chars Read for Reader.
int y[20]; // Total Chars Read for Mapper.
int uw1;
int uw2;
int rsize[20]; // Chars Read per file for Reader.

int msrc[20]; // SRC queue number for Mapper.

omp_lock_t l0;
omp_lock_t l1;
omp_lock_t l2;
omp_lock_t l3;
omp_lock_t l4;
omp_lock_t l5;

typedef queue<int, list<int> > Qid;
Qid fQids; // File Queue numbers for Reader.
Qid rQids; // Reader Queue numbers for Mapper.
Qid mQids; // Mapper Queue numbers for Reducer.


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
Lrecords Urecords[20];



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
			if(0 == strcasecmp((*it1).words, split))
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

	free(sentence);
	free(mdata[mt]);
	printf("Map %02d : Unique Words (%d)\n", omp_get_thread_num(), nUniqueWords);
}


void MapRecordsToReducers(int mt, int ct)
{
	Lrecords::iterator it1;
	Lrecords::iterator it2;
	record t1;
	int i;

	for(i = ct; i < 128; i = i+nReducers)
	{
		it1 = W[mt].wmap[i].begin();
		it2 = W[mt].wmap[i].end();

		while(it1 != it2)
		{
		   t1.words = (char*) malloc((strlen((*it1).words)+1) * sizeof(char));
		   strcpy(t1.words, (*it1).words);
		   t1.wc = (*it1).wc;
		   Crecords[ct].push_back(t1);
		   ++it1;
		}
	}
}


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
}


void DestroyReducerRecords(int ct)
{
	Lrecords::iterator it1;
	Lrecords::iterator it2;

	omp_set_lock(&l5);
	it1 = Crecords[ct].begin();
	it2 = Crecords[ct].end();

	while(it1 != it2)
	{
		free((*it1).words);
		(*it1).words = NULL;
		++it1;
	}
	Crecords[ct].clear();
	omp_unset_lock(&l5);
}

int main(int argc, char* argv[])
{
	int i, j, k, r1, m1, c1;
	int rdone;
	int mdone;
	int q[20];
	
	char fid[2];
	char* test;
	char* files[20];

	for(i = 0; i < 20; i++)
	{
	   files[i] = (char*) malloc(20*sizeof(char));
		strcpy(files[i], "CleanText/");
		sprintf(fid, "%d", (i+1));
		strcat(files[i], fid);
		strcat(files[i], ".txt");
		//printf("%s\n", files[i]);
		fQids.push(i);
	   //test = files[i];
	   //printf("Hash : %d\n", H(test));
	}

	
	omp_init_lock(&l0);
	omp_init_lock(&l1);
	omp_init_lock(&l2);
	omp_init_lock(&l3);
	omp_init_lock(&l4);
	omp_init_lock(&l5);



	omp_set_num_threads(16);
   #pragma omp parallel
	{
		#pragma omp master
		{
			r1 = 0;
			m1 = 0;
			c1 = 0;
			uw1 = 0;
			uw2 = 0;
			rdone = 0;
			mdone = 0;
			nReaders = 4;
			nMappers = 4;
			nReducers = 7;
			printf("Master %02d : nReaders (%02d); nMappers (%02d); nReducers (%02d)\n", omp_get_thread_num(), nReaders, nMappers, nReducers);

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
		               //printf("Read %02d : %1d \n", rid, 1);
		               //printf("Read %02d : %s  cc (%02d) = %d\n", rid, fname[rt], rt, rsize[rt]);
							pushRQ(&rQ1[rt], lines[rt]);
		               //printf("Read %02d : %1d \n", rid, 2);
		               x[rt] += rsize[rt];
		               //printf("Read %02d : %1d \n", rid, 3);
							
							omp_set_lock(&l1);
							rQids.push(rt);
							omp_unset_lock(&l1);

		               //printf("Read %02d : %1d \n", rid, 4);
						   while(!rQ1[rt].empty())
							{
								//rQ1[rt].pop();
								omp_set_lock(&l2);
								if(rQ1[rt].empty())
								{
		                     //printf("Read %02d : %1d \n", rid, 5);
									omp_unset_lock(&l2);
									break;
								}
								omp_unset_lock(&l2);
								usleep(500);
							}

		               //printf("Read %02d : %1d \n", rid, 6);
							free(lines[rt]);
		               lines[rt] = NULL;
						}
					}

					omp_set_lock(&l3);
					rdone++;
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
						if(rdone == nReaders)
						{
							while(!rQids.empty()) rQids.pop();
							omp_unset_lock(&l3);
							break;
						}
						omp_unset_lock(&l3);

		            //printf("Map %02d : %1d \n", mid, 1);
						omp_set_lock(&l1);
						if(!rQids.empty())
						{
							msrc[mt] = rQids.front();
							rQids.pop();
						}
						else msrc[mt] = -1;
						omp_unset_lock(&l1);
						
		            //printf("Map %02d : %1d \n", mid, 2);
						if(msrc[mt] > -1)
						{
							omp_set_lock(&l2);
							swap(rQ1[msrc[mt]], rQ2[mt]);
							omp_unset_lock(&l2);

		               //printf("Map %02d : %1d \n", mid, 3);
							y[mt] += strlen(rQ2[mt].front());
							while(!rQ2[mt].empty())
							{
								//free(rQ2[mt].front());
								//rQ2[mt].pop();
								mdata[mt] = popRQ(&rQ2[mt]);

								CreateWordFrequency(mt);
							}
							
							q[mt] = 0;
							for(q[mt] = 0; q[mt] < nReducers; q[mt] += 1)
							{
								omp_set_lock(&l5);
								MapRecordsToReducers(mt, q[mt]);
								omp_unset_lock(&l5);
							}

							omp_set_lock(&l4);
							mQids.push(mt);
							omp_unset_lock(&l4);

	                  DestroyWordFrequency(mt);
						   //printf("Map %02d : FROM (%02d) cc %d $$\n", mid, msrc[mt], y[mt]);
						}
						else usleep(500);
					}
					omp_set_lock(&l4);
					++mdone;
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

					while(mdone < nMappers)
					{
						omp_set_lock(&l4);
						if(mdone == nMappers)
						{
							omp_unset_lock(&l4);
							break;
						}
						omp_unset_lock(&l4);
						usleep(500);
					}
					//omp_set_lock(&l5);
					//uw1 += Crecords[ct].size();
					//omp_unset_lock(&l5);
					
					ReduceRecords(ct);
					
					//omp_set_lock(&l5);
					//uw2 += Urecords[ct].size();
					//omp_unset_lock(&l5);

					printf("Reduce %02d : Total Words (%d -> %d)\n", cid, Crecords[ct].size(), Urecords[ct].size());
					//DestroyReducerRecords(ct);
				}
			}
		}
	}

	for(i = 0; i < nReducers; i++)
	{
		uw1 += Crecords[i].size();
		uw2 += Urecords[i].size();
		DestroyReducerRecords(i);
	}
	printf("Final : Total Words (%d -> %d)\n", uw1, uw2);
	for(i = 0; i < 20; i++) free(files[i]);

	omp_destroy_lock(&l0);
	omp_destroy_lock(&l1);
	omp_destroy_lock(&l2);
	omp_destroy_lock(&l3);
	omp_destroy_lock(&l4);
	omp_destroy_lock(&l5);

	return 0;
}
