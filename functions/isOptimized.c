#ifndef _ISOPTIMIZED_
#define _ISOPTIMIZED_

#include <stdlib.h>
#include <stdio.h>

/* Helper function.  If the optimization flag is found,
 * 1 is returned.  If not, then a 0 is returned.  If an
 * error occurs, a -1 is returned.
 *
 * @param argc The argument count.
 * @param argv The argument array.
 * @return 1 if the optimization flag was passed, 0 if no optimization
 *         is to take place, -1 if an error occurred.
 */
int isOptimized(int argc, char** argv) {
  int i;

  /* loop on the arguments */
  for (i = 1; i < argc; i++) {
    if (argv[i][0] == '-' && argv[i][1] == 'o') { /* that's it! */
      return 1;
    }
  }

  /* if we reach this point, no optimization was passed */
  return 0;
}

#endif /* _ISOPTIMIZED_ */
