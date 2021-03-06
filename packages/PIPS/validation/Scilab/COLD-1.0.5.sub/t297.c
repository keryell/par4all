/*
 * (c) HPC Project - 2010-2011 - All rights reserved
 *
 */

#include "scilab_rt.h"


int __lv0;
int __lv1;
int __lv2;
int __lv3;

/*----------------------------------------------------*/


/*----------------------------------------------------*/

int main(int argc, char* argv[])
{
  scilab_rt_init(argc, argv, COLD_MODE_STANDALONE);

  /*  t297.sce _ Fixed a bug when the RT is called with the new type of an array */
  double _u_a[1][3];
  _u_a[0][0]=3.0;
  _u_a[0][1]=5.0;
  _u_a[0][2]=8.0;
  scilab_rt_display_s0d2_("a",1,3,_u_a);
  double complex _u_b[1][3];
  scilab_rt_fft_d2_z2(1,3,_u_a,1,3,_u_b);
  scilab_rt_display_s0z2_("b",1,3,_u_b);
  double _tmpCT0[1][3];
  scilab_rt_ones_i0i0_d2(1,3,1,3,_tmpCT0);
  
  scilab_rt_assign_d2_z2(1,3,_tmpCT0,1,3,_u_b);
  scilab_rt_display_s0z2_("b",1,3,_u_b);

  scilab_rt_terminate();
}

