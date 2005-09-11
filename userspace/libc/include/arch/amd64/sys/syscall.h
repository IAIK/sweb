// Projectname: SWEB
// Simple operating system for educational purposes
//
// Copyright (C) 2005  Andreas Niederl
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.


/**
 * CVS Log Info for $RCSfile: syscall.h,v $
 *
 * $Id: syscall.h,v 1.1 2005/09/11 10:17:53 aniederl Exp $
 * $Log$
 *
 */


#ifndef syscall_h___
#define syscall_h___

#include "sys/syscall_numbers.h"

// variable containing a possible error number
extern unsigned int errno;

// error number setting
#define __set_errno(err_number) errno = (err_number)

// macros for pushing the argument registers on the stack, so they can safely
// be used
#define SAVE_ARGUMENT_REGISTERS_0
#define SAVE_ARGUMENT_REGISTERS_1 \
  "pushq %rbx\n"
#define SAVE_ARGUMENT_REGISTERS_2 \
  SAVE_ARGUMENT_REGISTERS_1 \
  "pushq %rcx\n"
#define SAVE_ARGUMENT_REGISTERS_3 \
  SAVE_ARGUMENT_REGISTERS_2 \
  "pushq %rdx\n"
#define SAVE_ARGUMENT_REGISTERS_4 \
  SAVE_ARGUMENT_REGISTERS_3 \
  "pushq %rsi\n"
#define SAVE_ARGUMENT_REGISTERS_5 \
  SAVE_ARGUMENT_REGISTERS_4 \
  "pushq %rdi\n"
#define SAVE_ARGUMENT_REGISTERS_6 \
  SAVE_ARGUMENT_REGISTERS_5 \
  "pushq %rbp\n"

// macros for restoring the argument registers
#define RESTORE_ARGUMENT_REGISTERS_0
#define RESTORE_ARGUMENT_REGISTERS_1 \
  "popq %rbx\n"
#define RESTORE_ARGUMENT_REGISTERS_2 \
  "popq %rcx\n" \
  RESTORE_ARGUMENT_REGISTERS_1
#define RESTORE_ARGUMENT_REGISTERS_3 \
  "popq %rdx\n" \
  RESTORE_ARGUMENT_REGISTERS_2
#define RESTORE_ARGUMENT_REGISTERS_4 \
  "popq %rsi\n" \
  RESTORE_ARGUMENT_REGISTERS_3
#define RESTORE_ARGUMENT_REGISTERS_5 \
  "popq %rdi\n" \
  RESTORE_ARGUMENT_REGISTERS_4
#define RESTORE_ARGUMENT_REGISTERS_6 \
  "popq %rbp\n" \
  RESTORE_ARGUMENT_REGISTERS_5


#define SYSCALL_BODY(name, num_args, args...)                                 \
(                                                                             \
{                                                                             \
  unsigned int result;                                                        \
                                                                              \
  __asm__ __volatile__(SAVE_ARGUMENT_REGISTERS_##num_args);                   \
                                                                              \
  __asm__ __volatile__("int $0x80\n"                                          \
                       : "=a"(result)                                         \
                       : "0"(__NR_##name) ARGUMENT_REGISTERS_##num_args(args) \
    );                                                                        \
                                                                              \
  __asm__ __volatile__(RESTORE_ARGUMENT_REGISTERS_##num_args);                \
                                                                              \
  if(result >= 0xfffff001)                                                    \
  {                                                                           \
    __set_errno(-result);                                                     \
    result = 0xffffffff;                                                      \
  }                                                                           \
                                                                              \
  (int) result;                                                               \
}                                                                             \
)

#define ARGUMENT_REGISTERS_0()
#define ARGUMENT_REGISTERS_1(arg1) , "b"(arg1)
#define ARGUMENT_REGISTERS_2(arg1, arg2) , "b"(arg1), "c"(arg2)
#define ARGUMENT_REGISTERS_3(arg1, arg2, arg3) \
  , "b"(arg1), "c"(arg2), "d"(arg3)
#define ARGUMENT_REGISTERS_4(arg1, arg2, arg3, arg4) \
  , "b"(arg1), "c"(arg2), "d"(arg3), "S"(arg4)
#define ARGUMENT_REGISTERS_5(arg1, arg2, arg3, arg4, arg5) \
  , "b"(arg1), "c"(arg2), "d"(arg3), "S"(arg4), "D"(arg5)
#define ARGUMENT_REGISTERS_5(arg1, arg2, arg3, arg4, arg5) \
  , "b"(arg1), "c"(arg2), "d"(arg3), "S"(arg4), "D"(arg5)
#define ARGUMENT_REGISTERS_6(arg1, arg2, arg3, arg4, arg5, arg6) \
  , "b"(arg1), "c"(arg2), "d"(arg3), "S"(arg4), "D"(arg5), "B"(arg6)


#define __syscall_0(return_type, name) \
return_type name() \
{ \
  return (return_type) (SYSCALL_BODY(name, 0)); \
}

#define __syscall_1(return_type, name, type1, arg1) \
return_type name(type1 arg1) \
{ \
  return (return_type) (SYSCALL_BODY(name, 1, arg1)); \
}

#define __syscall_2(return_type, name, type1, arg1, type2, arg2) \
return_type name(type1 arg1, type2 arg2) \
{ \
  return (return_type) (SYSCALL_BODY(name, 2, arg1, arg2)); \
}

#define __syscall_3(return_type, name, type1, arg1, type2, arg2, type3, arg3) \
return_type name(type1 arg1, type2 arg2, type3 arg3) \
{ \
  return (return_type) (SYSCALL_BODY(name, 3, arg1, arg2, arg3)); \
}

#define __syscall_4(return_type, name, type1, arg1, type2, arg2, type3, arg3,\
                    type4, arg4) \
return_type name(type1 arg1, type2 arg2, type3 arg3, type4 arg4) \
{ \
  return (return_type) (SYSCALL_BODY(name, 4, arg1, arg2, arg3, arg4)); \
}

#define __syscall_5(return_type, name, type1, arg1, type2, arg2, type3, arg3,\
                    type4, arg4, type5, arg5) \
return_type name(type1 arg1, type2 arg2, type3 arg3, type4 arg4, type5 arg5) \
{ \
  return (return_type) (SYSCALL_BODY(name, 5, arg1, arg2, arg3, arg4, arg5)); \
}

#define __syscall_6(return_type, name, type1, arg1, type2, arg2, type3, arg3,\
                    type4, arg4, type5, arg5, type6, arg6) \
return_type name(type1 arg1, type2 arg2, type3 arg3, type4 arg4, type5 arg5,\
                 type6 arg6) \
{ \
  return (return_type) (SYSCALL_BODY(name, 6, arg1, arg2, arg3, arg4, arg5,\
                                     arg6)); \
}




#endif // syscall_h___


