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

  double _u_year1[1][4];
  _u_year1[0][0]=12547.0;
  _u_year1[0][1]=18965.0;
  _u_year1[0][2]=14587.0;
  _u_year1[0][3]=13658.0;
  double _u_year2[1][4];
  _u_year2[0][0]=14587.0;
  _u_year2[0][1]=12589.0;
  _u_year2[0][2]=13658.0;
  _u_year2[0][3]=15784.0;
  double _u_year3[1][4];
  scilab_rt_yearfrac_d2d2_d2(1,4,_u_year1,1,4,_u_year2,1,4,_u_year3);
  scilab_rt_display_s0d2_("year3",1,4,_u_year3);
  double _u_a[9][1];
  _u_a[0][0]=1.0;
  _u_a[1][0]=2;
  _u_a[2][0]=3;
  _u_a[3][0]=4;
  _u_a[4][0]=5;
  _u_a[5][0]=6;
  _u_a[6][0]=7;
  _u_a[7][0]=8;
  _u_a[8][0]=9;
  int _u_b[9][1];
  _u_b[0][0]=10;
  _u_b[1][0]=20;
  _u_b[2][0]=30;
  _u_b[3][0]=40;
  _u_b[4][0]=50;
  _u_b[5][0]=60;
  _u_b[6][0]=70;
  _u_b[7][0]=80;
  _u_b[8][0]=90;
  double _u_c[9][1];
  scilab_rt_yearfrac_d2i2_d2(9,1,_u_a,9,1,_u_b,9,1,_u_c);
  scilab_rt_display_s0d2_("c",9,1,_u_c);
  int _u_d[3][3];
  _u_d[0][0]=1;
  _u_d[0][1]=2;
  _u_d[0][2]=3;
  _u_d[1][0]=4;
  _u_d[1][1]=5;
  _u_d[1][2]=6;
  _u_d[2][0]=7;
  _u_d[2][1]=8;
  _u_d[2][2]=9;
  int _u_e[3][3];
  _u_e[0][0]=10;
  _u_e[0][1]=20;
  _u_e[0][2]=30;
  _u_e[1][0]=40;
  _u_e[1][1]=50;
  _u_e[1][2]=60;
  _u_e[2][0]=70;
  _u_e[2][1]=80;
  _u_e[2][2]=90;
  double _tmp0[3][3];
  scilab_rt_yearfrac_i2i2_d2(3,3,_u_d,3,3,_u_e,3,3,_tmp0);
  scilab_rt_display_s0d2_("ans",3,3,_tmp0);

  scilab_rt_terminate();
}

