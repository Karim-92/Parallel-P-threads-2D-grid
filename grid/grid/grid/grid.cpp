// ConsoleApplication1.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <iostream>
#include <pthread.h>
#include <conio.h>
#include "iomanip"

#include <windows.h>
#include <immintrin.h>

using namespace std;

#define NN 1000
#define DD 0.01

void init(int N,	float* x);
void show(int N,	float* x);
void compare(int N,	float* x,	float* y);
void serial(int N,	float* x,	float delta);
void serial_rb(int N,	float* x,	float delta);
void parallel_rb(int N,	float* x,	float delta);
long long GetTimeMs64();
void * pthreadhandler(void*);
bool done=false;
float diff[3];
pthread_mutex_t mylock;
pthread_barrier_t myBarrier;
pthread_barrier_t barrier2;


int main()
{
	float  *x;
	float  *y;
	float  *z;

	
	x = new float[(NN+2)*(NN+2)];
	y = new float[(NN+2)*(NN+2)];
	z = new float[(NN+2)*(NN+2)];

    init(NN , x);
    init(NN , y);
	init(NN , z);
     
	long startTime =  GetTimeMs64();

	serial(NN,x,  DD);

	printf("\nserial        : %d m sec\n", GetTimeMs64()-startTime);
	
	startTime =  GetTimeMs64();

	serial_rb(NN,y,  DD);

	printf("\nserial rb     : %d m sec\n", GetTimeMs64()-startTime);

	startTime =  GetTimeMs64();

	parallel_rb(NN,z,  DD);

	printf("\nparallel rb   : %d m sec\n", GetTimeMs64()-startTime);

	//show(NN , x);
	//show(NN , y);
	//show(NN , z);
	compare(NN , z,y);
	_getch();
	return 0;
}

void init(int n,	float* A)
{
		for(int i=0;i<n+2;i++)
		for(int j=0;j<n+2;j++)
	       {
			   if(i==0)
	        	A[i*(n+2)+j]= -10;
			   else if(i==n+1)
				   A[i*(n+2)+j]=100;
			   else if(j==0)
				   A[i*(n+2)+j]=100;
			   else if(j==n+1)
				   A[i*(n+2)+j]= -10;
			   else A[i*(n+2)+j]=0;
	       }
}
void show(int n,	float* A)
{cout<<"\n\n";
		for(int i=0;i<n+2;i++)
		{
		 for(int j=0;j<n+2;j++)
	       {
			  printf("%5.2f\t",A[i*(n+2)+j]);
	       }
		 cout<<"\n";
		}
}

void compare(int n,	float* A, float* B)
{
		for(int i=0;i<(n+2)*(n+2);i++)
		
		if(fabs(A[i]-B[i]) > 0.1)
		  	  cout<<"\nError: Wrong Output\n";
		       
		cout<<"\nCorrect Output\n";
}


void serial(int n,	float* A,	float TOL)
{
	int done=0;
	float diff,temp;
	int i,j;
               
	while (!done)                                     /*outermost loop over sweeps*/
	{
      diff = 0;                                     /*initialize maximum difference to 0*/
      for (i = 1; i<n+1 ; i++)                       /*sweep over nonborder points of grid*/
	  {                         
		  for (j =1; j<n+1; j++) 
		  {
             temp = A[i*(n+2)+j];     /*save old value of element*/
             A[i*(n+2)+j]= 0.2*(A[i*(n+2)+j]+A[i*(n+2)+j-1]+A[(i-1)*(n+2)+j]+A[i*(n+2)+j+1] + A[(i+1)*(n+2)+j]);  
              diff += abs(A[i*(n+2)+j] - temp);    
		  }
	  }
       //   cout<< diff/(n*n)<<"  ";                  
     if (diff/(n*n) < TOL)  done = 1;  
	}
                   
}
void serial_rb(int n,	float* A,	float TOL)
{
	float diff,temp;
	int i,j, jstart;

  while (!done)                                 /*outermost loop over sweeps*/
  {
      diff = 0;                         /*initialize maximum difference to 0*/
      for (i = 1 ; i< n+1; i++)           /*Red Sweep over nonborder points*/
	  {
          jstart =  2 - i % 2 ;         /*To visit odd numbered pixels*/
          for (j = jstart ; j< n+1 ; j+=2)
		  {
			  temp = A[i*(n+2)+j];     /*save old value of element*/
              A[i*(n+2)+j]= 0.2*(A[i*(n+2)+j]+A[i*(n+2)+j-1]+A[(i-1)*(n+2)+j]+A[i*(n+2)+j+1] + A[(i+1)*(n+2)+j]);          
              diff += abs(A[i*(n+2)+j] - temp);                                                                 
		  }
	  }
                                            
 
      for( i = 1 ;i < n+1; i++)                         /*Black sweep over nonborder points*/
	  {
		  jstart =  i % 2 +1;     /*To visit even numbered pixels*/
          for (j = jstart ; j< n+1 ; j+=2)
		  {
              temp = A[i*(n+2)+j];     /*save old value of element*/
              A[i*(n+2)+j]= 0.2*(A[i*(n+2)+j]+A[i*(n+2)+j-1]+A[(i-1)*(n+2)+j]+A[i*(n+2)+j+1] + A[(i+1)*(n+2)+j]);          
              diff += abs(A[i*(n+2)+j] - temp);        
		  }
	  }
	  
  	 // cout<< "\n"<<diff/(n*n)<<"\n";  

       if (diff/(n*n) < TOL)  done = true;                                                                              
   }                           
 
}

struct args
{
	int n;
	float* A;
	float TOL;
	int tid;
};

void parallel_rb(int n,	float* A,	float TOL)
{
	pthread_mutex_init(&mylock, nullptr);
	pthread_barrier_init(&myBarrier, nullptr, 4);
	pthread_barrier_init(&barrier2, nullptr, 4);
	pthread_t p1;
	pthread_t p2;
	pthread_t p3;
	pthread_t p4;
	args arg1={n, A, TOL, 0};
	args arg2={n, A, TOL, 1};
	args arg3={n ,A, TOL, 2};
	args arg4={n, A, TOL, 3};

	pthread_create(&p1, NULL, pthreadhandler, &arg1);
	pthread_create(&p2, NULL, pthreadhandler, &arg2);
	pthread_create(&p3, NULL, pthreadhandler, &arg3);
	pthread_create(&p4, NULL, pthreadhandler, &arg4);
	
	pthread_join(p1,nullptr);
	pthread_join(p2, nullptr);
	pthread_join(p3, nullptr);
	pthread_join(p4, nullptr);
	pthread_mutex_destroy(&mylock);
	pthread_barrier_destroy(&myBarrier);
}

void * pthreadhandler(void*thread){
	args* myargs=(args*) thread;
	float* A=myargs->A;
	float TOL=myargs->TOL;
	int tid=myargs->tid;
	int n=myargs->n;
	float mydiff,temp;
	int i,j, jstart;
	int my_min=1+(tid*n/4);
	int my_max=1+((tid+1)*(float)n/4);
	int index=0;
	done=false;


	  while (!done)                                 /*outermost loop over sweeps*/
  {
      mydiff = 0;                         /*initialize maximum difference to 0*/
      for (i = my_min ; i<my_max ; i++)           /*Red Sweep over nonborder points*/
	  {
          jstart =  2 - i % 2 ;         /*To visit odd numbered pixels*/
          for (j = jstart ; j< n+1 ; j+=2)
		  {
			  temp = A[i*(n+2)+j];     /*save old value of element*/
              A[i*(n+2)+j]= 0.2*(A[i*(n+2)+j]+A[i*(n+2)+j-1]+A[(i-1)*(n+2)+j]+A[i*(n+2)+j+1] + A[(i+1)*(n+2)+j]);          
              mydiff += abs(A[i*(n+2)+j] - temp);                                                                 
		  }
	  }
      pthread_barrier_wait(&barrier2);                   
      for( i =my_min ;i < my_max; i++)                         /*Black sweep over nonborder points*/
	  {
		  jstart =  i % 2 +1;     /*To visit even numbered pixels*/
          for (j = jstart ; j< n+1 ; j+=2)
		  {
              temp = A[i*(n+2)+j];     /*save old value of element*/
              A[i*(n+2)+j]= 0.2*(A[i*(n+2)+j]+A[i*(n+2)+j-1]+A[(i-1)*(n+2)+j]+A[i*(n+2)+j+1] + A[(i+1)*(n+2)+j]);          
              mydiff += abs(A[i*(n+2)+j] - temp);        
		  }
	  }
	  
  	 // cout<< "\n"<<diff/(n*n)<<"\n";  
	  pthread_mutex_lock(&mylock);
	  diff[index]+=mydiff;
	  pthread_mutex_unlock(&mylock);
	  diff[(index+1)%3]=0;
	  pthread_barrier_wait(&myBarrier);
	  


       if (diff[index]/(n*n) < TOL)  done = true;
	   index=(index+1)%3;
   }
	return nullptr;
}


/* Returns the amount of milliseconds elapsed since the UNIX epoch. Works on both
 * windows and linux. */

long long GetTimeMs64()
{
#ifdef WIN32
 /* Windows */
 FILETIME ft;
 LARGE_INTEGER li;

 /* Get the amount of 100 nano seconds intervals elapsed since January 1, 1601 (UTC) and copy it
  * to a LARGE_INTEGER structure. */
 GetSystemTimeAsFileTime(&ft);
 li.LowPart = ft.dwLowDateTime;
 li.HighPart = ft.dwHighDateTime;

 unsigned long long ret = li.QuadPart;
 ret -= 116444736000000000LL; /* Convert from file time to UNIX epoch time. */
 ret /= 10000; /* From 100 nano seconds (10^-7) to 1 millisecond (10^-3) intervals */

 return ret;
#else
 /* Linux */
 struct timeval tv;

 gettimeofday(&tv, NULL);

 uint64 ret = tv.tv_usec;
 /* Convert from micro seconds (10^-6) to milliseconds (10^-3) */
 ret /= 1000;

 /* Adds the seconds (10^0) after converting them to milliseconds (10^-3) */
 ret += (tv.tv_sec * 1000);

 return ret;
#endif
}