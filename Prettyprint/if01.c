// PIPS prettyprinter does not regenerate the superfluous braces
// for the true branch of the test, which leads to a warning by gcc,
// even though the input code is OK with gcc

// FI: I do not know if minimizing the number of braces is or not
// a better goal than minimizing the number of warnings generated by a
// changing gcc. I suggest a Prettyprinter property for compatibility
// with gcc, PRETTYPRINT_FOR_GCC.

int main()
{
  int i, c= 0;
  if(c>1) {
    if(c>2)
      i =1;
    else
      i= 2;
  }
  return i;
}
