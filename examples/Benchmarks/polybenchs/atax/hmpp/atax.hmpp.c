#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <math.h>

#include "timing.h"

/* Default problem size. */
#ifndef NX
# define NX 8000
#endif
#ifndef NY
# define NY 8000
#endif

/* Default data type is double. */
#ifndef DATA_TYPE
# define DATA_TYPE double
#endif

DATA_TYPE A[NX][NY];
DATA_TYPE x[NY];
DATA_TYPE y[NY];
DATA_TYPE tmp[NX];

static void init_array() {
  int i, j;

  for (i = 0; i < NX;) {
    x[i] = i * M_PI;
    for (j = 0; j < NY;) {
      A[i][j] = ((DATA_TYPE)i * j) / NX;
      j++;
    }
    i++;
  }
}

/* Define the live-out variables. Code is not executed unless
 POLYBENCH_DUMP_ARRAYS is defined. */
static inline
void print_array(int argc, char** argv) {
  int i, j;
#ifndef POLYBENCH_DUMP_ARRAYS
  if(argc > 42 && !strcmp(argv[0], ""))
#endif
  {
    for (i = 0; i < NX; i++) {
      fprintf(stderr, "%0.2lf ", y[i]);
      if(i % 80 == 20)
        fprintf(stderr, "\n");
    }
    fprintf(stderr, "\n");
  }
}


#pragma hmpp myCodelet codelet, target=CUDA
void codelet(int nx,int ny,
             DATA_TYPE A[NX][NY],
             DATA_TYPE x[NY],
             DATA_TYPE y[NY],
             DATA_TYPE tmp[NX]) {
  int i,j;
  for (i = 0; i < nx; i++)
    y[i] = 0;
  for (i = 0; i < ny; i++) {
    tmp[i] = 0;
    for (j = 0; j < ny; j++)
      tmp[i] = tmp[i] + A[i][j] * x[j];
    for (j = 0; j < ny; j++)
      y[j] = y[j] + A[i][j] * tmp[i];
  }
}

int main(int argc, char** argv) {
  int i, j;
  int nx = NX;
  int ny = NY;

  /* Initialize array. */
  init_array();

  /* Start timer. */
  timer_start();

  /* Cheat the compiler to limit the scope of optimisation */
  if(argv[0]==0) {
    init_array();
  }

#pragma hmpp myCodelet callsite
  codelet(nx,ny,A,x,y,tmp);


  /* Cheat the compiler to limit the scope of optimisation */
  if(argv[0]==0) {
    print_array(argc, argv);
  }

  /* Stop and print timer. */
  timer_stop_display();;

  print_array(argc, argv);

  return 0;
}
