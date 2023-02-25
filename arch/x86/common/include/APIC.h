#pragma once

#include "CPUID.h"
#include "types.h"
#include "ACPI.h"
#include "EASTL/vector.h"
#include "EASTL/bit.h"
#include "IrqDomain.h"
#include "DeviceDriver.h"
#include "Device.h"
#include "ports.h"

extern "C" void arch_irqHandler_127();


class Apic : public InterruptController, public IrqDomain, public Device
{
public:

    // ##########################################
    // Register value types

    enum class IPIDestination : uint32_t
    {
        TARGET = 0,
        SELF   = 1,
        ALL    = 2,
        OTHERS = 3,
    };

    enum class IPIType : uint32_t
    {
        FIXED        = 0,
        LOW_PRIORITY = 1,
        SMI          = 2,
        NMI          = 4,
        INIT         = 5,
        SIPI         = 6,
    };

    enum class IPIDestinationMode : uint32_t
    {
        PHYSICAL = 0,
        LOGICAL  = 1,
    };

    enum class IPILevel : uint32_t
    {
        DEASSERT = 0,
        ASSERT   = 1,
    };

    enum class IntPinPolarity : uint32_t
    {
        ACTIVE_HIGH = 0,
        ACTIVE_LOW  = 1,
    };

    enum class IntTriggerMode : uint32_t
    {
        EDGE  = 0,
        LEVEL = 1,
    };

    enum class DeliveryStatus : uint32_t
    {
        IDLE    = 0,
        PENDING = 1,
    };

    enum class Mask : uint32_t
    {
        UNMASKED = 0,
        MASKED   = 1,
    };

    enum class TimerMode : uint32_t
    {
        ONESHOT      = 0,
        PERIODIC     = 1,
        TSC_DEADLINE = 2,
    };

    struct [[gnu::packed]] InterruptCommandRegisterLow
    {
        uint32_t vector                : 8;  // 0-7
        uint32_t delivery_mode         : 3;  // 8-10
        uint32_t destination_mode      : 1;  // 11
        uint32_t delivery_status       : 1;  // 12
        uint32_t reserved              : 1;  // 13
        uint32_t level                 : 1;  // 14
        uint32_t trigger_mode          : 1;  // 15
        uint32_t reserved2             : 2;  // 16-17
        uint32_t destination_shorthand : 2;  // 18-19
        uint32_t reserved3             : 12; // 20-31
    };
    static_assert(sizeof(InterruptCommandRegisterLow) == 4, "Invalid size for InterruptCommandRegisterLow");

    struct [[gnu::packed]] InterruptCommandRegisterHigh
    {
        union
        {
            struct
            {
                uint32_t reserved          : 24; //  0-23
                uint32_t xapic_destination : 8;  // 24-31
            };
            uint32_t x2apic_destination;
        };
    };
    static_assert(sizeof(InterruptCommandRegisterHigh) == 4, "Invalid size for InterruptCommandRegisterHigh");

    struct [[gnu::packed]] InterruptCommandRegister
    {
        InterruptCommandRegisterLow l;
        InterruptCommandRegisterHigh h;
    };
    static_assert(sizeof(InterruptCommandRegister) == 8, "Invalid size for InterruptCommandRegister");

    struct [[gnu::packed]] SpuriousInterruptVectorRegister
    {
        uint32_t vector         : 8;
        uint32_t enable         : 1;
        uint32_t focus_checking : 1;
        uint32_t reserved       : 22;

        void setSpuriousInterruptNumber(uint8 num);
    };
    static_assert(sizeof(SpuriousInterruptVectorRegister) == 4, "Invalid size for SpuriousInterruptVectorRegister");

    struct [[gnu::packed]] IdRegister
    {
        union [[gnu::packed]]
        {
            struct [[gnu::packed]]
            {
                uint32_t reserved : 24;
                uint32_t xapic_id : 8;
            };
            uint32_t x2apic_id;
        };

    };
    static_assert(sizeof(IdRegister) == 4, "Invalid size for IdRegister");

    struct [[gnu::packed]] VersionRegister
    {
        uint32_t version                              : 8;
        uint32_t reserved1                            : 8;
        uint32_t max_lvt_entry                        : 8;
        uint32_t eoi_broadcast_suppression_supported  : 1;
        uint32_t reserved2                            : 7;
    };
    static_assert(sizeof(VersionRegister) == 4, "Invalid size for VersionRegister");

    struct [[gnu::packed]] LVT_TimerRegister
    {
        uint32_t vector          :  8;
        uint32_t reserved1       :  4;
        uint32_t delivery_status :  1;
        uint32_t reserved2       :  3;
        uint32_t mask            :  1;
        uint32_t timer_mode      :  2; // 0: one-shot, 1: periodic, 3: TSC-Deadline
        uint32_t reserved3       : 13;

        void setVector(uint8_t vector);
        void setMode(TimerMode mode);
        void setMask(bool mask);
    };
    static_assert(sizeof(LVT_TimerRegister) == 4, "Invalid size for LVT_TimerRegister");

    struct [[gnu::packed]] LVT_LINTRegister
    {
        uint32_t vector          :  8;
        uint32_t delivery_mode   :  3;
        uint32_t reserved1       :  1;
        uint32_t delivery_status :  1;
        uint32_t pin_polarity    :  1;
        uint32_t remote_irr      :  1;
        uint32_t trigger_mode    :  1;
        uint32_t mask            :  1;
        uint32_t reserved3       : 15;
    };
    static_assert(sizeof(LVT_LINTRegister) == 4, "Invalid size for LVT_LINTRegister");

    struct [[gnu::packed]] LVT_ErrorRegister
    {
        uint32_t vector          :  8;
        uint32_t reserved1       :  4;
        uint32_t delivery_status :  1;
        uint32_t reserved2       :  3;
        uint32_t mask            :  1;
        uint32_t reserved3       : 15;
    };
    static_assert(sizeof(LVT_ErrorRegister) == 4, "Invalid size for LVT_ErrorRegister");

    struct [[gnu::packed]] ErrorStatusRegister
    {
        uint32_t illegal_register_access : 1;
        uint32_t recv_illegal_vector     : 1;
        uint32_t send_illegal_vector     : 1;
        uint32_t redirectable_ipi        : 1;
        uint32_t recv_accept_error       : 1;
        uint32_t send_accept_error       : 1;
        uint32_t recv_checksum_error     : 1;
        uint32_t send_checksum_error     : 1;
        uint32_t reserved                : 24;
    };
    static_assert(sizeof(ErrorStatusRegister) == 4, "Invalid size for ErrorStatusRegister");

    struct [[gnu::packed]] TimerDivideConfigRegister
    {
        uint32_t divisor_l : 2;
        uint32_t reserved1 : 1;
        uint32_t divisor_h : 1;
        uint32_t reserved2 : 28;

        void setTimerDivisor(uint8 divisor);
    };
    static_assert(sizeof(TimerDivideConfigRegister) == 4, "Invalid size for TimerDivideConfigRegister");

    struct [[gnu::packed]] PriorityRegister
    {
        uint32_t priority_sub_class : 4;
        uint32_t priority_class     : 4;
        uint32_t reserved           : 24;
    };
    static_assert(sizeof(PriorityRegister) == 4, "Invalid size for PriorityRegister");

    // ##########################################
    // Helpers for type safe access

    enum class ApicRegisterOffset : unsigned int
    {
        ID = 0x20,
        VERSION = 0x30,
        TASK_PRIORITY = 0x80,
        PROCESSOR_PRIORITY = 0xA0,
        EOI = 0xB0,
        LOGICAL_DESTINATION = 0xD0,
        SPURIOUS_INTERRUPT_VECTOR = 0xF0,
        ISR_31_0    = 0x100,
        ISR_63_32   = 0x110,
        ISR_95_64   = 0x120,
        ISR_127_96  = 0x130,
        ISR_159_128 = 0x140,
        ISR_191_160 = 0x150,
        ISR_223_192 = 0x160,
        ISR_255_224 = 0x170,
        TMR_31_0    = 0x180,
        TMR_63_32   = 0x190,
        TMR_95_64   = 0x1A0,
        TMR_127_96  = 0x1B0,
        TMR_159_128 = 0x1C0,
        TMR_191_160 = 0x1D0,
        TMR_223_192 = 0x1E0,
        TMR_255_224 = 0x1F0,
        IRR_31_0    = 0x200,
        IRR_63_32   = 0x210,
        IRR_95_64   = 0x220,
        IRR_127_96  = 0x230,
        IRR_159_128 = 0x240,
        IRR_191_160 = 0x250,
        IRR_223_192 = 0x260,
        IRR_255_224 = 0x270,
        ERROR_STATUS = 0x280,
        LVT_CMCI = 0x2F0,
        INTERRUPT_COMMAND = 0x300,
        INTERRUPT_COMMAND_H = 0x310,
        LVT_TIMER = 0x320,
        LVT_THERMAL_SENSOR = 0x330,
        LVT_PERFMON = 0x340,
        LVT_LINT0 = 0x350,
        LVT_LINT1 = 0x360,
        LVT_ERROR = 0x370,
        TIMER_INITIAL_COUNT = 0x380,
        TIMER_CURRENT_COUNT = 0x390,
        TIMER_DIVIDE_CONFIG = 0x3E0,
        SELF_IPI = 0x3F0,
    };

    struct Register
    {
        template<ApicRegisterOffset reg, typename T, bool readable_ = true, bool writeable_ = true>
        struct ApicRegister
        {
            using value_type = T;
            static constexpr ApicRegisterOffset reg_offset = reg;
            static constexpr bool readable = readable_;
            static constexpr bool writeable = writeable_;
        };

        using ID = ApicRegister<ApicRegisterOffset::ID, IdRegister, true, false>;
        using VERSION = ApicRegister<ApicRegisterOffset::VERSION, VersionRegister, true, false>;
        using TASK_PRIORITY = ApicRegister<ApicRegisterOffset::TASK_PRIORITY, PriorityRegister, true, true>;
        using PROCESSOR_PRIORITY = ApicRegister<ApicRegisterOffset::PROCESSOR_PRIORITY, PriorityRegister, true, false>;
        using EOI = ApicRegister<ApicRegisterOffset::EOI, uint32_t, false, true>;
        using LOGICAL_DESTINATION = ApicRegister<ApicRegisterOffset::LOGICAL_DESTINATION, uint32_t, true, false>;
        using SPURIOUS_INTERRUPT_VECTOR = ApicRegister<ApicRegisterOffset::SPURIOUS_INTERRUPT_VECTOR, SpuriousInterruptVectorRegister, true, true>;
        using ISR_31_0 = ApicRegister<ApicRegisterOffset::ISR_31_0, uint32_t, true, false>;
        using ISR_63_32 = ApicRegister<ApicRegisterOffset::ISR_63_32, uint32_t, true, false>;
        using ISR_95_64 = ApicRegister<ApicRegisterOffset::ISR_95_64, uint32_t, true, false>;
        using ISR_127_96 = ApicRegister<ApicRegisterOffset::ISR_127_96, uint32_t, true, false>;
        using ISR_159_128 = ApicRegister<ApicRegisterOffset::ISR_159_128, uint32_t, true, false>;
        using ISR_191_160 = ApicRegister<ApicRegisterOffset::ISR_191_160, uint32_t, true, false>;
        using ISR_223_192 = ApicRegister<ApicRegisterOffset::ISR_223_192, uint32_t, true, false>;
        using ISR_255_224 = ApicRegister<ApicRegisterOffset::ISR_255_224, uint32_t, true, false>;
        using TMR_31_0 = ApicRegister<ApicRegisterOffset::TMR_31_0, uint32_t, true, false>;
        using TMR_63_32 = ApicRegister<ApicRegisterOffset::TMR_63_32, uint32_t, true, false>;
        using TMR_95_64 = ApicRegister<ApicRegisterOffset::TMR_95_64, uint32_t, true, false>;
        using TMR_127_96 = ApicRegister<ApicRegisterOffset::TMR_127_96, uint32_t, true, false>;
        using TMR_159_128 = ApicRegister<ApicRegisterOffset::TMR_159_128, uint32_t, true, false>;
        using TMR_191_160 = ApicRegister<ApicRegisterOffset::TMR_191_160, uint32_t, true, false>;
        using TMR_223_192 = ApicRegister<ApicRegisterOffset::TMR_223_192, uint32_t, true, false>;
        using TMR_255_224 = ApicRegister<ApicRegisterOffset::TMR_255_224, uint32_t, true, false>;
        using IRR_31_0 = ApicRegister<ApicRegisterOffset::IRR_31_0, uint32_t, true, false>;
        using IRR_63_32 = ApicRegister<ApicRegisterOffset::IRR_63_32, uint32_t, true, false>;
        using IRR_95_64 = ApicRegister<ApicRegisterOffset::IRR_95_64, uint32_t, true, false>;
        using IRR_127_96 = ApicRegister<ApicRegisterOffset::IRR_127_96, uint32_t, true, false>;
        using IRR_159_128 = ApicRegister<ApicRegisterOffset::IRR_159_128, uint32_t, true, false>;
        using IRR_191_160 = ApicRegister<ApicRegisterOffset::IRR_191_160, uint32_t, true, false>;
        using IRR_223_192 = ApicRegister<ApicRegisterOffset::IRR_223_192, uint32_t, true, false>;
        using IRR_255_224 = ApicRegister<ApicRegisterOffset::IRR_255_224, uint32_t, true, false>;
        using ERROR_STATUS = ApicRegister<ApicRegisterOffset::ERROR_STATUS, ErrorStatusRegister, true, true>;
        using LVT_CMCI = ApicRegister<ApicRegisterOffset::LVT_CMCI, uint32_t, true, true>;
        using INTERRUPT_COMMAND = ApicRegister<ApicRegisterOffset::INTERRUPT_COMMAND, InterruptCommandRegister, true, true>;
        using LVT_TIMER = ApicRegister<ApicRegisterOffset::LVT_TIMER, LVT_TimerRegister, true, true>;
        using LVT_THERMAL_SENSOR = ApicRegister<ApicRegisterOffset::LVT_THERMAL_SENSOR, uint32_t, true, true>;
        using LVT_PERFMON = ApicRegister<ApicRegisterOffset::LVT_PERFMON, uint32_t, true, true>;
        using LVT_LINT0 = ApicRegister<ApicRegisterOffset::LVT_LINT0, LVT_LINTRegister, true, true>;
        using LVT_LINT1 = ApicRegister<ApicRegisterOffset::LVT_LINT1, LVT_LINTRegister, true, true>;
        using LVT_ERROR = ApicRegister<ApicRegisterOffset::LVT_ERROR, LVT_ErrorRegister, true, true>;
        using TIMER_INITIAL_COUNT = ApicRegister<ApicRegisterOffset::TIMER_INITIAL_COUNT, uint32_t, true, true>;
        using TIMER_CURRENT_COUNT = ApicRegister<ApicRegisterOffset::TIMER_CURRENT_COUNT, uint32_t, true, false>;
        using TIMER_DIVIDE_CONFIG = ApicRegister<ApicRegisterOffset::TIMER_DIVIDE_CONFIG, TimerDivideConfigRegister, true, true>;
        using SELF_IPI = ApicRegister<ApicRegisterOffset::SELF_IPI, LVT_ErrorRegister, false, true>;
    };



    // ##########################################

    static constexpr uint8_t ERROR_INTERRUPT_VECTOR = 91;
    static constexpr uint8_t TIMER_DIVISOR = 16;
    static constexpr uint8_t NUM_ISA_INTERRUPTS = 16;
    static constexpr uint8_t IRQ_VECTOR_OFFSET = 0x20; // Offset for IRQ remapping so that they don't overlap with cpu exceptions
    static constexpr uint8_t APIC_TIMER_VECTOR = 127;

    Apic(const eastl::string& name = "APIC");
    ~Apic() override = default;
    Apic(const Apic &) = delete;
    Apic &operator=(const Apic &) = delete;

     template <typename R> void writeRegister(const typename R::value_type &v) {
       static_assert(sizeof(v) == 4 || sizeof(v) == 8);
       if constexpr (sizeof(v) == 8) {
         writeRegisterImpl(R::reg_offset, eastl::bit_cast<uint64_t>(v));
       } else {
         writeRegisterImpl(R::reg_offset, eastl::bit_cast<uint32_t>(v));
       }
    }

    template <typename R>
    typename R::value_type readRegister()
    {
        static_assert(sizeof(typename R::value_type) == 4 || sizeof(typename R::value_type) == 8);
        union
        {
            uint32_t u32;
            uint64_t u64;
        } v;

        v.u64 = readRegisterImpl(R::reg_offset);
        if constexpr (sizeof(typename R::value_type) == 8)
        {
            return eastl::bit_cast<typename R::value_type>(v.u64);
        }
        else
        {
            return eastl::bit_cast<typename R::value_type>(v.u32);
        }
    }

    [[nodiscard]] uint32_t apicId() const;
    [[nodiscard]] bool isInitialized() const;
    [[nodiscard]] virtual bool isX2Apic();

    virtual void init() = 0;

    static void setIMCRMode(IMCRData mode);

    static void globalEnable(bool = true);
    void enable(bool = true);

    bool mask(irqnum_t irq, bool mask) override;
    bool ack(irqnum_t irq) override;
    bool irqStart(irqnum_t irq) override;


    void sendEOI(size_t num);

    bool checkIRR(uint8 num);
    bool checkISR(uint8 num);

    struct ApicTimer : public InterruptController, public IrqDomain, public Device
    {
        explicit ApicTimer(Apic& apic);

        bool mask(irqnum_t irq, bool mask) override;
        bool ack(irqnum_t irq) override;

        bool isMasked();

    private:
        Apic* apic_;
        bool masked_ = true;
    };

    void initTimer();
    void setTimerPeriod(uint32_t count);
    virtual uint32_t readId() = 0;

    void registerIPI(irqnum_t irqnum);

    void sendIPI(uint8 vector, IPIDestination dest_type = IPIDestination::ALL,
                 size_t target = -1, IPIType ipi_type = IPIType::FIXED,
                 bool wait_for_delivery = false);
    void sendIPI(uint8 vector, const Apic& target, bool wait_for_delivery = false);
    void startAP(uint8_t apic_id, size_t entry_addr);

    [[nodiscard]] bool usingAPICTimer() const;
    void setUsingAPICTimer(bool using_apic_timer);

    void setSpuriousInterruptNumber(uint8_t num);
    void setErrorInterruptVector(uint8_t vector);

    static eastl::vector<MADTProcLocalAPIC> local_apic_list_;
    static void addLocalAPICToList(const MADTProcLocalAPIC&);

    size_t outstanding_EOIs_ = 0;

    ApicTimer timer_interrupt_controller;
    IrqDomain inter_processor_interrupt_domain;

protected:
    virtual void writeRegisterImpl(ApicRegisterOffset offset, uint64_t v) = 0;
    virtual uint64_t readRegisterImpl(ApicRegisterOffset offset) = 0;

    virtual void writeIcr(InterruptCommandRegisterLow icr, uint32_t dest) = 0;

    virtual void waitIpiDelivered() = 0;

    uint32_t id_ = 0;
    bool initialized_ = false;
    bool use_apic_timer_ = false;
};

class XApic : public Apic
{
public:
    XApic() :
        Apic(eastl::string("xAPIC ") + eastl::to_string(CPUID::localApicId()))
    {
    }

    ~XApic() override = default;
    XApic(const XApic &) = delete;
    XApic &operator=(const XApic &) = delete;

    static void foundLocalAPIC(void *reg_phys_addr,
                               MADTExtendedHeader::Flags flags);

    static void mapAt(size_t addr);

    void init() override;

    uint32_t readId() override;

    static void setPhysicalAddress(void* paddr)
    {
        reg_paddr_ = paddr;
    }

    static void* physicalAddress()
    {
        return reg_paddr_;
    }

    static void* virtualAddress()
    {
        return reg_vaddr_;
    }

    static void* readMsrPhysAddr();

    static bool apicSupported();

protected:
     void writeRegisterImpl(ApicRegisterOffset offset, uint64_t v) override;
     uint64_t readRegisterImpl(ApicRegisterOffset offset) override;

     void writeIcr(InterruptCommandRegisterLow icr, uint32_t dest) override;

     void waitIpiDelivered() override;

private:

    static void* reg_paddr_;
    static void* reg_vaddr_;
};

class ApicDriver : public BasicDeviceDriver, public Driver<Apic>
{
public:
    ApicDriver();
    ~ApicDriver() override = default;

    static ApicDriver& instance();

    void doDeviceDetection() override;
    void cpuLocalInit() override;

private:
};

class ApicTimerDriver : public BasicDeviceDriver, public Driver<Apic::ApicTimer>
{
public:
    ApicTimerDriver();
    ~ApicTimerDriver() override = default;

    static ApicTimerDriver& instance();

    void doDeviceDetection() override;
    void cpuLocalInit() override;

private:
};
