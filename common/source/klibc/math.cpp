#include "math.h"
#include "inttypes.h"

// float ceilf(float x)
// {
// 	union {float f; uint32_t i;} u = {x};
// 	int e = (int)(u.i >> 23 & 0xff) - 0x7f;
// 	uint32_t m;

// 	if (e >= 23)
// 		return x;
// 	if (e >= 0) {
// 		m = 0x007fffff >> e;
// 		if ((u.i & m) == 0)
// 			return x;
// 		FORCE_EVAL(x + 0x1p120f);
// 		if (u.i >> 31 == 0)
// 			u.i += m;
// 		u.i &= ~m;
// 	} else {
// 		FORCE_EVAL(x + 0x1p120f);
// 		if (u.i >> 31)
// 			u.f = -0.0;
// 		else if (u.i << 1)
// 			u.f = 1.0;
// 	}
// 	return u.f;
// }


// #if FLT_EVAL_METHOD==0 || FLT_EVAL_METHOD==1
// #define EPS DBL_EPSILON
// #elif FLT_EVAL_METHOD==2
// #define EPS LDBL_EPSILON
// #endif
// static const double_t toint = 1/EPS;

// double ceil(double x)
// {
// 	union {double f; uint64_t i;} u = {x};
// 	int e = u.i >> 52 & 0x7ff;
// 	double_t y;

// 	if (e >= 0x3ff+52 || x == 0)
// 		return x;
// 	/* y = int(x) - x, where int(x) is an integer neighbor of x */
// 	if (u.i >> 63)
// 		y = x - toint + toint - x;
// 	else
// 		y = x + toint - toint - x;
// 	/* special case because of non-nearest rounding modes */
// 	if (e <= 0x3ff-1) {
// 		FORCE_EVAL(y);
// 		return u.i >> 63 ? -0.0 : 1;
// 	}
// 	if (y < 0)
// 		return x + y + 1;
// 	return x + y;
// }
