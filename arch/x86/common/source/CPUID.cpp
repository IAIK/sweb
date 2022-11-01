#include "CPUID.h"
#include "debug.h"

void CPUID::cpuid(uint32_t selector, uint32_t subselector, uint32_t& eax, uint32_t& ebx, uint32_t& ecx, uint32_t& edx)
{
    asm("cpuid\n"
        :"=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
        :"a"(selector), "c"(subselector));
}

uint32_t CPUID::localApicId()
{
    uint32_t unused;
    uint32_t ebx = 0;
    cpuid(1, 0, unused, ebx, unused, unused);
    return ebx >> 24;
}

bool CpuFeatures::cpuHasFeature(X86Feature feature)
{
    return features_.test(feature);
}

void CpuFeatures::initCpuFeatures()
{
    uint32_t eax, ebx, ecx, edx;

    uint32_t highest_base_cpuid_param;
    uint32_t highest_ext_cpuid_param;
    char manufacturer_id[13];
    memset(manufacturer_id, 0, sizeof(manufacturer_id));
    CPUID::cpuid(0, 0, highest_base_cpuid_param, *(uint32_t*)manufacturer_id, *((uint32_t*)manufacturer_id + 2), *((uint32_t*)manufacturer_id + 1));
    CPUID::cpuid(0x80000000, 0, highest_ext_cpuid_param, ebx, ecx, edx);
    debug(A_MULTICORE, "CPU manufaturer id: %s, highest cpuid param: %x / %x\n", manufacturer_id, highest_base_cpuid_param, highest_ext_cpuid_param);

    CPUID::cpuid(1, 0, eax, ebx, ecx, edx);

    features_.set(FPU, edx & (1 << 0));
    features_.set(VME, edx & (1 << 1));
    features_.set(DEBUGGING_EXTENSIONS, edx & (1 << 2));
    features_.set(PSE, edx & (1 << 3));
    features_.set(TSC, edx & (1 << 4));
    features_.set(MSR, edx & (1 << 5));
    features_.set(PAE, edx & (1 << 6));
    features_.set(MACHINE_CHECK_EXCEPTION, edx & (1 << 7));
    features_.set(CMPXCHG8, edx & (1 << 8));
    features_.set(APIC, edx & (1 << 9));
    features_.set(SYSENTER_SYSEXIT, edx & (1 << 11));
    features_.set(MTRR, edx & (1 << 12));
    features_.set(PGE, edx & (1 << 13));
    features_.set(MCA, edx & (1 << 14));
    features_.set(CMOV, edx & (1 << 15));
    features_.set(PAT, edx & (1 << 16));
    features_.set(PSE36, edx & (1 << 17));
    features_.set(PSN, edx & (1 << 18));
    features_.set(CLFLUSH, edx & (1 << 19));
    features_.set(DEBUG_STORE, edx & (1 << 21));
    features_.set(ACPI_THERMAL_MSR, edx & (1 << 22));
    features_.set(MMX, edx & (1 << 23));
    features_.set(FXSAVE_FXRESTOR, edx & (1 << 24));
    features_.set(SSE, edx & (1 << 25));
    features_.set(SSE2, edx & (1 << 26));
    features_.set(CACHE_SELF_SNOOP, edx & (1 << 27));
    features_.set(HYPERTHREADING, edx & (1 << 28));
    features_.set(THERMAL_MONITOR, edx & (1 << 29));
    features_.set(IA64, edx & (1 << 30));
    features_.set(PBE, edx & (1 << 31));

    features_.set(SSE3, ecx & (1 << 0));
    features_.set(PCMULQDQ, ecx & (1 << 1));
    features_.set(DTES64, ecx & (1 << 3));
    features_.set(MONITOR, ecx & (1 << 4));
    features_.set(DS_CPL, ecx & (1 << 5));
    features_.set(VMX, ecx & (1 << 5));
    features_.set(SMX, ecx & (1 << 6));
    features_.set(ENHANCED_SPEED_STEP, ecx & (1 << 7));
    features_.set(THERMAL_MONITOR_2, ecx & (1 << 8));
    features_.set(SSSE3, ecx & (1 << 9));
    features_.set(L1_CONTEXT_ID, ecx & (1 << 10));
    features_.set(SILICON_DEBUG_INTERFACE, ecx & (1 << 11));
    features_.set(FMA3, ecx & (1 << 12));
    features_.set(CMPXCHG16B, ecx & (1 << 13));
    features_.set(TASK_PRIORITY_MESSAGE_DISABLE, ecx & (1 << 14));
    features_.set(PERFMON_DEBUG_CAPABILITY, ecx & (1 << 15));
    features_.set(PCID, ecx & (1 << 17));
    features_.set(DMA_DIRECT_CACHE_ACCESS, ecx & (1 << 18));
    features_.set(SSE4_1, ecx & (1 << 19));
    features_.set(SSE4_2, ecx & (1 << 20));
    features_.set(X2APIC, ecx & (1 << 21));
    features_.set(MOVBE, ecx & (1 << 22));
    features_.set(POPCNT, ecx & (1 << 23));
    features_.set(TSC_DEADLINE, ecx & (1 << 24));
    features_.set(AES, ecx & (1 << 25));
    features_.set(XSAVE, ecx & (1 << 26));
    features_.set(OS_XSAVE, ecx & (1 << 27));
    features_.set(AVX, ecx & (1 << 28));
    features_.set(F16C, ecx & (1 << 29));
    features_.set(RDRAND, ecx & (1 << 30));
    features_.set(HYPERVISOR, ecx & (1 << 31));

    CPUID::cpuid(7, 0, eax, ebx, ecx, edx);

    features_.set(FSGSBASE, ebx & (1 << 0));
    features_.set(TSC_ADJUST_MSR, ebx & (1 << 1));
    features_.set(SGX, ebx & (1 << 2));
    features_.set(BMI1, ebx & (1 << 3));
    features_.set(TSX_HLE, ebx & (1 << 4));
    features_.set(AVX2, ebx & (1 << 5));
    features_.set(SMEP, ebx & (1 << 7));
    features_.set(BMI2, ebx & (1 << 8));
    features_.set(ENHANCED_REP_MOVSB, ebx & (1 << 9));
    features_.set(INVPCID, ebx & (1 << 10));
    features_.set(TSX_RTM, ebx & (1 << 11));
    features_.set(RESOURCE_DIRECTOR_MONITORING, ebx & (1 << 12));
    features_.set(FPU_CS_DS_DEPRECATED, ebx & (1 << 13));
    features_.set(MPX, ebx & (1 << 14));
    features_.set(RESOURCE_DIRECTOR_ALLOCATION, ebx & (1 << 15));
    features_.set(AVX512_F, ebx & (1 << 16));
    features_.set(AVX512_DQ, ebx & (1 << 17));
    features_.set(RDSEED, ebx & (1 << 18));
    features_.set(ADX, ebx & (1 << 19));
    features_.set(SMAP, ebx & (1 << 20));
    features_.set(AVX512_IFMA, ebx & (1 << 21));
    features_.set(CLFLUSHOPT, ebx & (1 << 23));
    features_.set(CLWB, ebx & (1 << 24));
    features_.set(PROCESSOR_TRACE, ebx & (1 << 25));
    features_.set(AVX512_PF, ebx & (1 << 26));
    features_.set(AVX512_ER, ebx & (1 << 27));
    features_.set(AVX512_CD, ebx & (1 << 28));
    features_.set(SHA, ebx & (1 << 29));
    features_.set(AVX512_BW, ebx & (1 << 30));
    features_.set(AVX512_VL, ebx & (1 << 31));

    features_.set(PREFETCHW1, ecx & (1 << 0));
    features_.set(AVX512_VBMI, ecx & (1 << 1));
    features_.set(UMIP, ecx & (1 << 2));
    features_.set(PKU, ecx & (1 << 3));
    features_.set(OS_PKU, ecx & (1 << 4));
    features_.set(WAITPKG, ecx & (1 << 5));
    features_.set(AVX512_VBMI2, ecx & (1 << 6));
    features_.set(CET_SS, ecx & (1 << 7));
    features_.set(GFNI, ecx & (1 << 8));
    features_.set(VAES, ecx & (1 << 9));
    features_.set(CLMUL, ecx & (1 << 10));
    features_.set(AVX512_VNNI, ecx & (1 << 11));
    features_.set(AVX512_BITALG, ecx & (1 << 12));
    features_.set(TME, ecx & (1 << 13));
    features_.set(AVX512_VPOPCNTDQ, ecx & (1 << 14));
    features_.set(FIVE_LEVEL_PAGING, ecx & (1 << 15));
    features_.set(RDPID, ecx & (1 << 22));
    features_.set(PKS, ecx & (1 << 31));

    features_.set(AVX512_4VNNIW, edx & (1 << 2));
    features_.set(AVX512_4FMAPS, edx & (1 << 3));
    features_.set(FSRM, edx & (1 << 4));
    features_.set(UINTR, edx & (1 << 5));
    features_.set(AVX512_VP2INTERSECT, edx & (1 << 8));
    features_.set(SPECIAL_REGISTER_BUFFER_DATA_SAMPLING_MITIGATIONS, edx & (1 << 9));
    features_.set(VERW_CPU_BUFFER_CLEAR, edx & (1 << 10));
    features_.set(TSX_ALWAYS_ABORT, edx & (1 << 11));
    features_.set(TSX_FORCE_ABORT_MSR, edx & (1 << 13));
    features_.set(SERIALIZE, edx & (1 << 14));
    features_.set(HYBRID, edx & (1 << 15));
    features_.set(TSXLDTRK, edx & (1 << 16));
    features_.set(PCONFIG, edx & (1 << 18));
    features_.set(ARCH_LAST_BRANCH_RECORDS, edx & (1 << 19));
    features_.set(CET_IBT, edx & (1 << 20));
    features_.set(AMX_BF16, edx & (1 << 22));
    features_.set(AVX512_FP16, edx & (1 << 23));
    features_.set(AMX_TILE, edx & (1 << 24));
    features_.set(AMX_INT8, edx & (1 << 25));
    features_.set(IBRS_IBPB, edx & (1 << 26));
    features_.set(STIBP, edx & (1 << 27));
    features_.set(FLUSH_CMD_MSR, edx & (1 << 28));
    features_.set(ARCH_CAPABILITIES_MSR, edx & (1 << 29));
    features_.set(CORE_CAPABILITIES_MSR, edx & (1 << 30));
    features_.set(SSBD, edx & (1 << 31));

    if (highest_ext_cpuid_param >= 0x80000001)
    {
        CPUID::cpuid(0x80000001, 0, eax, ebx, ecx, edx);

        features_.set(SYSCALL_SYSRET, edx & (1 << 11));
        features_.set(MULTIPROCESSOR_CAPABLE, edx & (1 << 19));
        features_.set(NX, edx & (1 << 20));
        features_.set(MMX_EXT, edx & (1 << 21));
        features_.set(FXSAVE_FXRESTOR_OPT, edx & (1 << 25));
        features_.set(PDPE1GB, edx & (1 << 26));
        features_.set(RDTSCP, edx & (1 << 27));
        features_.set(LONG_MODE, edx & (1 << 29));
        features_.set(THREE_D_NOW_EXT, edx & (1 << 30));
        features_.set(THREE_D_NOW, edx & (1 << 31));

        features_.set(LAHF_SAHF_LONG_MODE, ecx & (1 << 0));
        features_.set(SVM, ecx & (1 << 2));
        features_.set(EXTAPIC, ecx & (1 << 3));
        features_.set(CR8_LEGACY, ecx & (1 << 4));
        features_.set(LZCNT, ecx & (1 << 5));
        features_.set(SSE4A, ecx & (1 << 6));
        features_.set(SSE_MISALIGNED, ecx & (1 << 7));
        features_.set(PREFETCH_PREFETCHW, ecx & (1 << 8));
        features_.set(INSTRUCTION_BASED_SAMPLING, ecx & (1 << 10));
        features_.set(XOP, ecx & (1 << 11));
        features_.set(SKINIT_STGI, ecx & (1 << 12));
        features_.set(WATCHDOG_TIMER, ecx & (1 << 13));
        features_.set(LIGHT_WEIGHT_PROFILING, ecx & (1 << 15));
        features_.set(FMA4, ecx & (1 << 16));
        features_.set(TRANSLATION_CACHE_EXTENSION, ecx & (1 << 17));
        features_.set(NODE_ID_MSR, ecx & (1 << 19));
        features_.set(TBM, ecx & (1 << 21));
        features_.set(TOPOEXT, ecx & (1 << 22));
        features_.set(PERFCTR_CORE, ecx & (1 << 23));
        features_.set(PERFCTR_NB, ecx & (1 << 24));
        features_.set(DBX, ecx & (1 << 26));
        features_.set(PERFTSC, ecx & (1 << 27));
        features_.set(PCX_L2I, ecx & (1 << 28));
        features_.set(MONITORX, ecx & (1 << 29));
    }

    if (highest_ext_cpuid_param >= 0x8000001F)
    {
        CPUID::cpuid(0x8000001F, 0, eax, ebx, ecx, edx);

        features_.set(SME, eax & (1 << 0));
        features_.set(SEV, eax & (1 << 1));
        features_.set(PAGE_FLUSH_MSR, eax & (1 << 2));
        features_.set(SEV_ES, eax & (1 << 3));
        features_.set(SEV_SNP, eax & (1 << 4));
        features_.set(VM_PERM_LVL, eax & (1 << 5));
        features_.set(VTE, eax & (1 << 16));
    }
}
