#pragma once

#include <stdbool.h>

extern bool g_verbose;

#define UTRESET() printf("\033[0m")
#define UTBOLD() printf("\033[1m")

#define UTRED() printf("\033[31m")

#define ARRAY_SIZE(_x) ( sizeof(_x)/sizeof((_x)[0]) )
#define verbose(...) ( g_verbose ? printf(__VA_ARGS__) : 0)
#define error(...) ( UTRED(), printf(__VA_ARGS__), UTRESET())
