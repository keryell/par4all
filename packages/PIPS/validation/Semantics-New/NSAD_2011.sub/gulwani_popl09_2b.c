// Sumit Gulwani, Krishna K. Mehra, Trishul M. Chilimbi: SPEED: precise and
// efficient static estimation of program computational complexity.
// POPL 2009: 127-139
// figure 2b
// Byron Cook, Andreas Podelski, Andrey Rybalchenko: Termination proofs for
// systems code. PLDI 2006: 415-426
// figure 3a

// $Id$

// parameters

#define DO_CONTROL 0
#define DO_CHECKING 1
#define GOOD (c1 + c2 <= n - x0 + n - z0)

// tools

#include <stdlib.h>
#include <stdio.h>

int rand_b(void) {
	return rand() % 2;
}
int rand_z(void) {
	return rand() - rand();
}

#define OR(t1, t2) {if (rand_b()) {t1} else {t2}}
#define LOOP(j) {while (rand_b()) {j}}

void deadlock() {
	printf("deadlock\n");
	while (1);
}
#define ASSUME(c) {if (!(c)) deadlock();}

#if DO_CONTROL == 0
#define CONTROL(c)
#else
void control_error(void) {
	fprintf(stderr, "control error");
	exit(1);
}
#define CONTROL(c) {if (!(c)) control_error();}
#endif

#if DO_CHECKING == 0
#define CHECK
#else
void checking_error(void) {
	fprintf(stderr, "checking error");
	exit(2);
}
#ifdef GOOD
#define CHECK {if (!(GOOD)) checking_error();}
#else
#ifdef BAD
#define CHECK {if (BAD) checking_error();}
#endif
#endif
#endif

#define COMMAND_NOCHECK(g, a) {ASSUME(g); a;}
#define COMMAND(g, a) {COMMAND_NOCHECK(g, a); CHECK;}

// control and commands

#define S1 CONTROL(x < n && z > x)
#define S2 CONTROL(x < n && z <= x)
#define S3 CONTROL(x >= n)

#define G1 (x < n && z > x)
#define G1a (x < n - 1 && z > x + 1)
#define G1b (x < n - 1 && z == x + 1)
#define G1c (x == n - 1 && z > x)
#define A1 {x++; c1++;}
#define C1 COMMAND(G1, A1)
#define C1a COMMAND(G1a, A1)
#define C1b COMMAND(G1b, A1)
#define C1c COMMAND(G1c, A1)

#define G2 (x < n && z <= x)
#define G2a (x < n && z < x)
#define G2b (x < n && z == x)
#define A2 {z++; c2++;}
#define C2 COMMAND(G2, A2)
#define C2a COMMAND(G2a, A2)
#define C2b COMMAND(G2b, A2)

#define INI {\
	x0 = rand(); z0 = rand(); n = rand();\
	ASSUME(n >= x0 && n >= z0);\
	x = x0; z = z0;\
	c1 = c2 = 0;\
}

// transition system

void ts_singlestate(void) {
	int x0, z0, n, x, z, c1, c2;
	INI; CHECK;
	LOOP(OR(C1, C2))
}

void ts_restructured(void) {
	int x0, z0, n, x, z, c1, c2;
	INI; CHECK;
	if (x < n)
		if (z > x) goto L1;
		else goto L2;
	else goto L3;
L2:
	S2; LOOP(C2a; S2); C2b;
L1:
	S1;
	LOOP(
		OR(
			C1a; S1,

			C1b; S2; LOOP(C2a; S2); C2b; S1
		)
	)
	C1c;
L3:
	S3;
}

int main(void) {
	ts_singlestate();
	ts_restructured();
	return 0;
}

