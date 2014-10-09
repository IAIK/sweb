#ifndef time_h___
#define time_h___

#define CLOCKS_PER_SEC 1000000

#ifndef CLOCK_T_DEFINED
#define CLOCK_T_DEFINED
typedef unsigned int clock_t;
#endif // CLOCK_T_DEFINED

/**
 * posix function signature
 * do not change the signature!
 */
extern clock_t clock(void);



#endif // time_h___


