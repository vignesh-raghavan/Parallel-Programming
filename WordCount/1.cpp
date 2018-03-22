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

typedef queue<string, list<string> > Qstring;

typedef struct ReaderQ
{
	char* line;
} ReaderQ;

void pushRQ(Qstring *rQ, string s)
{
	printf("%s\n", s.c_str());
	(*rQ).push(s);	
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


char* readFile(string fname)
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

	fin.close();
	return fdata;
}


int main(int argc, char* argv[])
{
	char* files[20] = {"CleanText/1.txt", "CleanText/2.txt", "CleanText/3.txt", "CleanText/4.txt", "CleanText/5.txt",
	                  "CleanText/6.txt", "CleanText/7.txt", "CleanText/8.txt", "CleanText/9.txt", "CleanText/10.txt",
	                  "CleanText/11.txt", "CleanText/12.txt", "CleanText/13.txt", "CleanText/14.txt", "CleanText/15.txt",
                     "CleanText/16.txt", "CleanText/17.txt", "CleanText/18.txt", "CleanText/19.txt", "CleanText/20.txt"};

	int i, j;
	char* fname[20];
	
	char* lines[20];
	


	//omp_set_num_threads(20);
	for(j = 0; j < 20; j++)
   //#pragma omp parallel
	{
		int nt;
		nt = j;
		//nt = omp_get_thread_num();
	   
	   fname[nt] = files[nt];
	   lines[nt] = readFile(fname[nt]);
		printf("wc (%s) = %d\n", fname[nt], strlen(lines[nt]));
		free(lines[nt]);
	}

	return 0;
}
