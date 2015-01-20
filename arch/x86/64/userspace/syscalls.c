void __syscall()
{
  asm("int $0x80");
}
