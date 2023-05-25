#include "IrqDomain.h"

#include "debug.h"

IrqDomain::IrqDomain(const eastl::string& name,
                     size_t num_irqs,
                     InterruptController* controller) :
    controller_(controller),
    name_(name)
{
    setNumIrqs(num_irqs);
}

void IrqDomain::setNumIrqs(size_t num_irqs)
{
    irqs_.resize(num_irqs);
}

// Unsafe if irq could be used concurrently (by any cpu)
void IrqDomain::mapIrqTo(irqnum_t irq, IrqDomain& target_domain, irqnum_t target_irq)
{
    assert(&target_domain);
    debug(A_INTERRUPTS, "Mapping %s IRQ: %zx -> %s IRQ: %zx\n", name_.c_str(), irq,
          target_domain.name().c_str(), target_irq);

    auto& info = irqs_.at(irq);

    if (irqs_[irq].map_to.domain)
        debug(A_INTERRUPTS, "WARNING: %s IRQ: %zx already mapped to %s IRQ: %zx\n",
              name_.c_str(), irq, info.map_to.domain->name().c_str(),
              info.map_to.irqnum);

    info.map_to = {&target_domain, target_irq, nullptr};
    target_domain.irqMappedBy(target_irq, *this, irq, info.map_to);
}

void IrqDomain::irqMappedBy(irqnum_t irq,
                            IrqDomain& source_domain,
                            irqnum_t source_irq,
                            IrqMappingTarget& mapped_by_entry)
{
    auto& info = irqs_.at(irq);
    auto it = eastl::find_if(
        info.mapped_by.begin(), info.mapped_by.end(),
        [&](auto&& x) { return x.domain == &source_domain && x.irqnum == source_irq; });
    assert(it == info.mapped_by.end());

    auto entry = new IrqMappingTarget{&source_domain, source_irq, &mapped_by_entry};
    mapped_by_entry.reverse_ref = entry;
    info.mapped_by.push_back(*entry);
}

// Unsafe if irq could be used concurrently (by any cpu)
void IrqDomain::removeIrq(irqnum_t irq)
{
    auto& info = irqs_.at(irq);

    auto& map_to = info.map_to;
    debug(A_INTERRUPTS, "Remove IRQ mapping %s IRQ: %zx -> %s IRQ: %zx\n", name_.c_str(),
          irq, map_to.domain->name().c_str(), map_to.irqnum);

    activateIrq(irq, false);

    assert(map_to.domain);
    map_to.domain->removeIrqMappedBy(map_to.irqnum, *this, irq);
    info = {};
}

void IrqDomain::removeIrqMappedBy(irqnum_t irq,
                                  IrqDomain& source_domain,
                                  irqnum_t source_irq)
{
    auto& info = irqs_.at(irq);

    auto& mapped_by = info.mapped_by;
    auto m_it = eastl::find_if(
        mapped_by.begin(), mapped_by.end(),
        [&](auto&& x) { return x.domain == &source_domain && x.irqnum == source_irq; });
    assert(m_it != mapped_by.end());
    IrqMappingTarget* entry = &*m_it;
    mapped_by.erase(m_it);
    delete entry;
}

void IrqDomain::setIrqHandler(irqnum_t irq, const IrqInfo::handler_func_t& handler)
{
    irqs_.at(irq).handler = handler;
}

IrqDomain::IrqInfo::handler_func_t IrqDomain::getIrqHandler(irqnum_t irq)
{
    return irqs_.at(irq).handler;
}

void IrqDomain::handleIrq(irqnum_t irqnum)
{
    debugAdvanced(A_INTERRUPTS, "Irq domain %s handle irq %zu\n", name().c_str(), irqnum);
    auto info = irqInfo(irqnum);
    assert(info);
    if (info->handler)
    {
        info->handler();
    }
}

void IrqDomain::activateIrq(irqnum_t irq, bool activate)
{
    debug(A_INTERRUPTS, "Irq domain %s %s irq %zu\n", name().c_str(),
          activate ? "activate" : "deactivate", irq);
    if (controller())
    {
        controller()->mask(irq, !activate);
    }
}

IrqDomain::IrqInfo* IrqDomain::irqInfo(irqnum_t irqnum)
{
    auto& info = irqs_.at(irqnum);
    return &info;
}

void IrqDomain::printReverseMappingTree(irqnum_t irqnum)
{
    auto mtree = irq(irqnum).reverseMappingTree();
    for (auto it = mtree.begin(); it != mtree.end(); ++it)
    {
        auto&& [domain, local_irq] = *it;
        assert(domain);
        auto info = domain->irqInfo(local_irq);
        if (it.level > 0 || (info && (!info->mapped_by.empty() || info->handler)))
        {
            kprintfd("%*s- %s:%zu\n", (int)it.level * 2, "", domain->name().c_str(), local_irq);
        }

    }
}

void IrqDomain::printAllReverseMappings()
{
    debugAlways(A_INTERRUPTS, "Interrupt mappings for '%s' domain:\n", name().c_str());
    for (size_t i = 0; i < irqs_.size(); ++i)
    {
        printReverseMappingTree(i);
    }
}
