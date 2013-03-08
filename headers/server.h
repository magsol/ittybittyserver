#ifndef _SERVER_
#define _SERVER_

/* this file contains all the necessary includes for the server */

/* first, a few header files */

#include "constants.h"
#include "returncodes.h"
#include "conList.h"
#include "memList.h"

/* implementations */

#include "../functions/sendError.c"
#include "../functions/sendResponse.c"
#include "../functions/sendAll.c"
#include "recvAll.h"
#include "../functions/copyLength.c"
#include "../functions/appendStr.c"
#include "../functions/contentType.c"
#include "../functions/fileContents.c"
#include "../functions/strDecode.c"
#include "../functions/isOptimized.c"
#include "../functions/printArgs.c"

#endif /* _SERVER_ */
