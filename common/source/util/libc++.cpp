

extern "C" int __cxa_thread_atexit(__attribute__((unused)) void (*func)(), __attribute__((unused)) void *obj, __attribute__((unused)) void *dso_symbol)
{
        // Required for thread_local keyword (used for cpu local storage). Should technically call destructors of thread local objects, but we don't care about that (for now)
        return 0;
}
