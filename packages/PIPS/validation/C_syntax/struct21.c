/* Make sure s is declared in a cast */

int i;
void * q = (struct s2 {int i; int j;} *) &i;
struct s2 s;
