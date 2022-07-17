#include "logging.h"

__thread char logFileName[1024] = "";
__thread FILE* logFileHandle = NULL;