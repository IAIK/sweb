
#ifndef FS_TESTS_H__
#define FS_TESTS_H__

class Terminal;

void fsTests(Terminal *term);


void testRegFS();
void testMount();
void testUmount();
void testPathWalker();
void testVfsSyscall();
void testSyscallMkdir();
void testSyscallReaddir();
void testSyscallChdir();
void testSyscallRmdir();
void testSyscallRmdirExtern();


void testMini();


#endif
