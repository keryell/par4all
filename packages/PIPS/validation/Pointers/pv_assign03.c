// assignment of aggregate structure 
typedef struct {int *a; int *b[10]; int (*c)[10];} mystruct;
int main()
{
  mystruct s1;
  mystruct s2;
  s2=s1;
  return(0);
}
