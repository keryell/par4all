int main() {
   // Generated by Pass VARIABLE_REPLICATION
   int __i_0, __i_1, __i_2, __i_3;
   // Generated by Pass VARIABLE_REPLICATION
   int __a_0[10], __a_1[10], __a_2[10], __a_3[10];
   // Generated by Pass VARIABLE_REPLICATION
   int __r_0, __r_1, __r_2, __r_3;
   // Generated 
   int __return__value;
   __i_0 = 0;
   __i_0++;
   __i_1 = 0;
   __i_1++;
   __i_2 = 0;
   __i_2++;
   __i_3 = 0;
   __i_3++;

#pragma distributed on_cluster=0
   {
      __a_0[__i_0] = 42;
      __a_1[__i_0] = __a_0[__i_0];
      __a_2[__i_0] = __a_0[__i_0];
      __a_3[__i_0] = __a_0[__i_0];
      __i_0++;
      __i_1 = __i_0;
      __i_2 = __i_0;
      __i_3 = __i_0;
   }
#pragma distributed on_cluster=1
   {
      __i_1--;
      __i_0 = __i_1;
      __i_2 = __i_1;
      __i_3 = __i_1;
      __r_1 = __a_1[__i_1]+__i_1-1;
      __r_0 = __r_1;
      __r_2 = __r_1;
      __r_3 = __r_1;
   }
   
   __return__value = __r_0;
   //i=1, a[1]=42, r=42
   return __return__value;
}
