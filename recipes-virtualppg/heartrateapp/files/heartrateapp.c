#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <fcntl.h>
#include <errno.h>
#include <math.h>
#include <pthread.h>

#define q	11		    /* for 2^11 points */
#define N	(1<<q)		/* N-point FFT, iFFT */

#define pause_ns 20000000

typedef float real;
typedef struct{real Re; real Im;} complex;

#ifndef PI
#define PI	3.14159265358979323846264338327950288
#endif

static void* acquisition(void*);
static void* analysis(void*);
void fft( complex*, int, complex*);

int main(void)
{
  complex v1[N];
  complex v2[N];
  void* v = (void*) v1;

  pthread_t tAcquire, tAnalysis;
  void* tRet;

  printf("Start reading the sensor:\n");

  //first acquisition thread
  pthread_create(&tAcquire, NULL, acquisition, v);
  //wait for the thread in order to have data to be analyzed
  pthread_join(tAcquire, tRet);

  while(1)
  {
    //create a thread to analyze the data stored
    pthread_create(&tAnalysis, NULL, analysis, v);

    //swap the vector on which the data to be analyzed are stored
    v = (void*) (v == (void*) v1 ? v2 : v1);

    //create a thread to store another block of data
    pthread_create(&tAcquire, NULL, acquisition, v);
    //wait for the thread
    pthread_join(tAcquire, tRet);

    //wait for the analysis thread to be completed
    //(it is very unlukely it requires more than 2048*pause_ns ns, around 41 seconds)
    pthread_join(tAnalysis, tRet);
  }
  //in this way the acquisition of data is not influenced by the analyzing time

  exit(EXIT_SUCCESS);
}

static void* acquisition(void* arg)  //acquisition function
{
  int fd;
  static const char* fname = "/dev/vppgmod";
  complex* v = (complex*) arg;

  //open the device file
  if ((fd = open(fname, O_RDONLY)) < 0)
  {
    fprintf(stderr, "Failed opening: %s\n", fname);
    exit(EXIT_FAILURE);
  }

  //cycle to acquire 2048 values
  for(int i=0; i<N; ++i)
  {
    char buf[sizeof(int)];

    //read a value frome the device, it returns an int as a char*, only a cast
    //is needed, if it is returned a different number of bytes an error occurred
    if (read(fd, buf, sizeof(int)) != sizeof(int))
    {
      fprintf(stderr, "Error while reading: %s\n", fname);
      exit(EXIT_FAILURE);
    }

    v[i].Re = (float) *(int*)buf;
    v[i].Im = 0;

    //structs to delay the next acquisition
    struct timespec t1, t2;
    struct timespec* req = &t1;
    struct timespec* rem = &t2;
    req->tv_sec = 0;
    req->tv_nsec = pause_ns;

    //delay procedure
    int ret;
    while ((ret = nanosleep(req, rem)) && errno == EINTR)
    {
      struct timespec* p = req;
      req = rem;
      rem = p;
    }
    if (ret)  //the delay is not complete due to an error
      exit(EXIT_FAILURE);
  }

  //close the file file
  close(fd);

  //thread exit
  pthread_exit(NULL);
}

static void* analysis(void* arg)  //analysis function
{
  complex* v = (complex*) arg;
  complex scratch[N];
  float abs[N];
  int k;
  int m;
  int i;
  int minIdx, maxIdx;

  // FFT computation
  fft(v, N, scratch);

  // PSD computation
  for(k=0; k<N; k++)
	  abs[k] = (50.0/2048)*((v[k].Re*v[k].Re)+(v[k].Im*v[k].Im));

  minIdx = (0.5*2048)/50;   // position in the PSD of the spectral line corresponding to 30 bpm
  maxIdx = 3*2048/50;       // position in the PSD of the spectral line corresponding to 180 bpm

  // Find the peak in the PSD from 30 bpm to 180 bpm
  m = minIdx;
  for(k=minIdx; k<(maxIdx); k++)
  {
    if(abs[k] > abs[m])
	    m = k;
  }

  // Print the heart beat in bpm
  printf("\n\n%d bpm\n\n", (m)*60*50/2048);

  //thread exit
  pthread_exit(NULL);
}

void fft( complex *v, int n, complex *tmp )
{
  if(n>1)       /* otherwise, do nothing and return */
  {
    int k,m;    
    complex z, w, *vo, *ve;
    ve = tmp; vo = tmp+n/2;
    for(k=0; k<n/2; k++)
    {
      ve[k] = v[2*k];
      vo[k] = v[2*k+1];
    }
    fft( ve, n/2, v );		/* FFT on even-indexed elements of v[] */
    fft( vo, n/2, v );		/* FFT on odd-indexed elements of v[] */
    for(m=0; m<n/2; m++) 
    {
      w.Re = cos(2*PI*m/(double)n);
      w.Im = -sin(2*PI*m/(double)n);
      z.Re = w.Re*vo[m].Re - w.Im*vo[m].Im;	/* Re(w*vo[m]) */
      z.Im = w.Re*vo[m].Im + w.Im*vo[m].Re;	/* Im(w*vo[m]) */
      v[  m  ].Re = ve[m].Re + z.Re;
      v[  m  ].Im = ve[m].Im + z.Im;
      v[m+n/2].Re = ve[m].Re - z.Re;
      v[m+n/2].Im = ve[m].Im - z.Im;
    }
  }

  return;
}