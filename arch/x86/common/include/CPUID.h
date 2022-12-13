#pragma once

#include <cstdint>
#include "EASTL/bitset.h"
#include "ArchCpuLocalStorage.h"

namespace CPUID
{
    void cpuid(uint32_t selector, uint32_t subselector, uint32_t& eax, uint32_t& ebx, uint32_t& ecx, uint32_t& edx);

    uint32_t highestSupportedLeaf();
    uint32_t highestSupportedExtendedLeaf();
    uint32_t localApicId();
    uint32_t localX2ApicId();
};

class CpuFeatures
{
public:
    enum X86Feature
    {
        // cpuid eax=1 edx
        FPU,
        VME,
        DEBUGGING_EXTENSIONS,
        PSE,
        TSC,
        MSR,
        PAE,
        MACHINE_CHECK_EXCEPTION,
        CMPXCHG8,
        APIC,
        SYSENTER_SYSEXIT,
        MTRR,
        PGE,
        MCA,
        CMOV,
        PAT,
        PSE36,
        PSN,
        CLFLUSH,
        DEBUG_STORE,
        ACPI_THERMAL_MSR,
        MMX,
        FXSAVE_FXRESTOR,
        SSE,
        SSE2,
        CACHE_SELF_SNOOP,
        HYPERTHREADING,
        THERMAL_MONITOR,
        IA64,
        PBE,

        // cpuid eax=1 ecx
        SSE3,
        PCMULQDQ,
        DTES64,
        MONITOR,
        DS_CPL,
        VMX,
        SMX,
        ENHANCED_SPEED_STEP,
        THERMAL_MONITOR_2,
        SSSE3,
        L1_CONTEXT_ID,
        SILICON_DEBUG_INTERFACE,
        FMA3,
        CMPXCHG16B,
        TASK_PRIORITY_MESSAGE_DISABLE,
        PERFMON_DEBUG_CAPABILITY,
        PCID,
        DMA_DIRECT_CACHE_ACCESS,
        SSE4_1,
        SSE4_2,
        X2APIC,
        MOVBE,
        POPCNT,
        TSC_DEADLINE,
        AES,
        XSAVE,
        OS_XSAVE,
        AVX,
        F16C,
        RDRAND,
        HYPERVISOR,

        // cpuid eax=7 ebx
        FSGSBASE,
        TSC_ADJUST_MSR,
        SGX,
        BMI1,
        TSX_HLE,
        AVX2,
        SMEP,
        BMI2,
        ENHANCED_REP_MOVSB,
        INVPCID,
        TSX_RTM,
        RESOURCE_DIRECTOR_MONITORING,
        FPU_CS_DS_DEPRECATED,
        MPX,
        RESOURCE_DIRECTOR_ALLOCATION,
        AVX512_F,
        AVX512_DQ,
        RDSEED,
        ADX,
        SMAP,
        AVX512_IFMA,
        CLFLUSHOPT,
        CLWB,
        PROCESSOR_TRACE,
        AVX512_PF,
        AVX512_ER,
        AVX512_CD,
        SHA,
        AVX512_BW,
        AVX512_VL,

        // cpuid eax=7 ecx
        PREFETCHW1,
        AVX512_VBMI,
        UMIP,
        PKU,
        OS_PKU,
        WAITPKG,
        AVX512_VBMI2,
        CET_SS,
        GFNI,
        VAES,
        CLMUL,
        AVX512_VNNI,
        AVX512_BITALG,
        TME,
        AVX512_VPOPCNTDQ,
        FIVE_LEVEL_PAGING,
        RDPID,
        PKS,

        // cpuid eax=7 edx
        AVX512_4VNNIW,
        AVX512_4FMAPS,
        FSRM,
        UINTR,
        AVX512_VP2INTERSECT,
        SPECIAL_REGISTER_BUFFER_DATA_SAMPLING_MITIGATIONS,
        VERW_CPU_BUFFER_CLEAR,
        TSX_ALWAYS_ABORT,
        TSX_FORCE_ABORT_MSR,
        SERIALIZE,
        HYBRID,
        TSXLDTRK,
        PCONFIG,
        ARCH_LAST_BRANCH_RECORDS,
        CET_IBT,
        AMX_BF16,
        AVX512_FP16,
        AMX_TILE,
        AMX_INT8,
        IBRS_IBPB,
        STIBP,
        FLUSH_CMD_MSR,
        ARCH_CAPABILITIES_MSR,
        CORE_CAPABILITIES_MSR,
        SSBD,

        // AMD cpuid eax=80000001h edx
        SYSCALL_SYSRET,
        MULTIPROCESSOR_CAPABLE,
        NX,
        MMX_EXT,
        FXSAVE_FXRESTOR_OPT,
        PDPE1GB,
        RDTSCP,
        LONG_MODE,
        THREE_D_NOW_EXT,
        THREE_D_NOW,

        // AMD cpuid eax=80000001h ecx
        LAHF_SAHF_LONG_MODE,
        SVM,
        EXTAPIC,
        CR8_LEGACY,
        LZCNT,
        SSE4A,
        SSE_MISALIGNED,
        PREFETCH_PREFETCHW,
        INSTRUCTION_BASED_SAMPLING,
        XOP,
        SKINIT_STGI,
        WATCHDOG_TIMER,
        LIGHT_WEIGHT_PROFILING,
        FMA4,
        TRANSLATION_CACHE_EXTENSION,
        NODE_ID_MSR,
        TBM,
        TOPOEXT,
        PERFCTR_CORE,
        PERFCTR_NB,
        DBX,
        PERFTSC,
        PCX_L2I,
        MONITORX,

        // AMD cpuid eax=8000001Fh eax
        SME,
        SEV,
        PAGE_FLUSH_MSR,
        SEV_ES,
        SEV_SNP,
        VM_PERM_LVL,
        VTE,

        FEATURE_COUNT // dummy value to get enum size for bitset
    };

    CpuFeatures();

    void initCpuFeatures();
    bool cpuHasFeature(X86Feature feature);
private:
    eastl::bitset<X86Feature::FEATURE_COUNT> features_;
};

extern cpu_local CpuFeatures cpu_features;
