#ifndef wait_h___
#define wait_h___

#ifdef __cplusplus
extern "C" {
#endif

#define WEXITED 4

//pid typedefs
#ifndef PID_T_DEFINED
#define PID_T_DEFINED
typedef int pid_t;
#endif // PID_T_DEFINED

extern pid_t waitpid(pid_t pid, int *status, int options);


#ifdef __cplusplus
}
#endif

#endif // wait_h___


