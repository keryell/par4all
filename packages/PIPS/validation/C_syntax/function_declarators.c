/* This code is extracted from C ISO standard*/

int f(void), *fip(), (*pfi)();
int (*apfi[3])(int *x, int *y);
int (*fpfi())();

int (*fpfi1(int (*)(long), int))(int);

int (*fpfi2(int (*)(long), int,...))(int,...);


