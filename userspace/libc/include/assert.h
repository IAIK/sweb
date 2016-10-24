#pragma once

#include <stdio.h>
#include <stdlib.h>

#define assert(X) do { if (!(X)) {\
    printf("Assertion failed: '" #X "', file %s, function %s, line %d\n", __FILE__, __FUNCTION__, __LINE__); \
 exit(-1); } } while (0)

