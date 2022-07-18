#include "logging.h"

_Thread_local char logFileName[1024] = "";
_Thread_local FILE* logFileHandle = NULL;