#include "logging.h"

volatile char logFileName[1024] = "";
volatile FILE* logFileHandle = NULL;