#pragma once

#ifndef __cplusplus

#define bool	_Bool
#if defined __STDC_VERSION__ && __STDC_VERSION__ > 201710L
#define true	((_Bool)+1u)
#define false	((_Bool)+0u)
#else
#define true	1
#define false	0
#endif

#else /* __cplusplus */

/* Supporting _Bool in C++ is a GCC extension.  */
#define _Bool	bool

#endif /* __cplusplus */
