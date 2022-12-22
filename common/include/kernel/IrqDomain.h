#pragma once

#include "ranges.h"
#include <cstddef>
#include "EASTL/fixed_function.h"
#include "EASTL/intrusive_list.h"
#include "EASTL/iterator.h"
#include "EASTL/map.h"
#include "EASTL/string.h"
#include "EASTL/type_traits.h"
#include "EASTL/vector.h"

using irqnum_t = size_t;

struct InterruptController
{
    virtual bool mask(irqnum_t irq, bool mask) = 0;
    virtual bool ack(irqnum_t irq) = 0;

    virtual bool irqStart([[maybe_unused]] irqnum_t irq)
    {
        ++pending_EOIs;
        return true;
    }

    size_t pending_EOIs = 0;
};

class IrqDomain
{
public:
    struct IrqMappingTarget : public eastl::intrusive_list_node
    {
        constexpr IrqMappingTarget() = default;

        constexpr IrqMappingTarget(IrqDomain* domain,
                                   irqnum_t irqnum,
                                   IrqMappingTarget* reverse_ref) :
            domain(domain),
            irqnum(irqnum),
            reverse_ref(reverse_ref)
        {
        }

        constexpr IrqMappingTarget(const IrqMappingTarget&) = default;
        constexpr IrqMappingTarget(IrqMappingTarget&&) = default;
        constexpr IrqMappingTarget& operator=(const IrqMappingTarget&) = default;
        constexpr IrqMappingTarget& operator=(IrqMappingTarget&&) = default;

        friend constexpr bool operator==(const IrqMappingTarget& l,
                                         const IrqMappingTarget& r)
        {
            return l.domain == r.domain && l.irqnum == r.irqnum;
        }

        IrqDomain* domain;
        irqnum_t irqnum;
        IrqMappingTarget* reverse_ref;
    };

    struct IrqInfo
    {
        using handler_func_t = eastl::fixed_function<16, void(void)>;

        IrqMappingTarget map_to;
        eastl::intrusive_list<IrqMappingTarget> mapped_by;
        handler_func_t handler = nullptr;
    };

    IrqDomain(const eastl::string& name, InterruptController* controller = nullptr);

    [[nodiscard]] constexpr const eastl::string& name() const { return name_; }

    [[nodiscard]] constexpr InterruptController* controller() const
    {
        return controller_;
    }

    class DomainIrqHandle
    {
    public:
        class iterator
        {
        private:
            IrqDomain* domain_ = nullptr;
            irqnum_t irq_ = 0;

        public:
            using value_type = DomainIrqHandle;
            using difference_type = size_t;
            using pointer = value_type*;
            using reference = value_type&;
            using iterator_category = eastl::forward_iterator_tag;

            constexpr iterator() = default;
            constexpr iterator(const iterator& other) = default;
            constexpr iterator(iterator&& other) = default;

            constexpr iterator(IrqDomain* domain, irqnum_t irq) :
                domain_(domain),
                irq_(irq)
            {
            }

            constexpr iterator(const DomainIrqHandle& handle) :
                iterator(handle.domain_, handle.irq_)
            {
            }

            friend constexpr bool operator==(const iterator& lhs, const iterator& rhs)
            {
                return lhs.domain_ == rhs.domain_ && lhs.irq_ == rhs.irq_;
            }

            class sentinel_t
            {
            };

            friend constexpr bool operator==(const iterator& lhs, sentinel_t)
            {
                return lhs.domain_ == nullptr;
            }

            constexpr DomainIrqHandle operator*() const { return {*domain_, irq_}; }

            const iterator& operator++()
            {
                auto info = domain_->irqInfo(irq_);
                domain_ = info->map_to.domain;
                irq_ = info->map_to.irqnum;

                return *this;
            }

            iterator operator++(int)
            {
                auto tmp = *this;
                ++(*this);
                return tmp;
            }
        };

        class tree_iterator
        {
        private:
            IrqDomain* domain_ = nullptr;
            irqnum_t irq_ = 0;

        public:
            size_t level = 0;

            using value_type = DomainIrqHandle;
            using difference_type = size_t;
            using pointer = value_type*;
            using reference = value_type&;
            using iterator_category = eastl::forward_iterator_tag;

            constexpr tree_iterator() = default;
            constexpr tree_iterator(const tree_iterator& other) = default;
            constexpr tree_iterator(tree_iterator&& other) = default;

            constexpr tree_iterator(IrqDomain* domain, irqnum_t irq) :
                domain_(domain),
                irq_(irq)
            {
            }

            constexpr tree_iterator(const DomainIrqHandle& handle) :
                tree_iterator(handle.domain_, handle.irq_)
            {
            }

            friend constexpr bool operator==(const tree_iterator& lhs,
                                             const tree_iterator& rhs)
            {
                return lhs.domain_ == rhs.domain_ && lhs.irq_ == rhs.irq_;
            }

            class sentinel_t { };

            friend constexpr bool operator==(const tree_iterator& lhs, sentinel_t)
            {
                return lhs.domain_ == nullptr;
            }

            constexpr DomainIrqHandle operator*() const { return {*domain_, irq_}; }

            const tree_iterator& operator++()
            {
                auto info = domain_->irqInfo(irq_);
                if (info->mapped_by.empty())
                {
                    // Need to backtrack
                    while (true)
                    {
                        auto& map_to = info->map_to;
                        if (map_to.domain)
                        {
                            assert(map_to.reverse_ref);
                            // find next sibling in parent
                            auto map_to_info = map_to.domain->irqInfo(map_to.irqnum);
                            assert(map_to_info);
                            auto it = map_to_info->mapped_by.locate(*map_to.reverse_ref);
                            assert(it != map_to_info->mapped_by.end());
                            ++it;
                            if (it != map_to_info->mapped_by.end())
                            {
                                // found next sibling
                                domain_ = it->domain;
                                irq_ = it->irqnum;
                                break;
                            }
                            else
                            {
                                // Reached last sibling in parent, backtrack to parent of
                                // parent
                                info = map_to_info;
                                --level;
                            }
                        }
                        else
                        {
                            // Nothing to backtrack to, reached end of tree
                            domain_ = nullptr;
                            irq_ = 0;
                            break;
                        }
                    }
                }
                else
                {
                    domain_ = info->mapped_by.front().domain;
                    irq_ = info->mapped_by.front().irqnum;
                    ++level;
                }

                return *this;
            }

            tree_iterator operator++(int)
            {
                auto tmp = *this;
                ++(*this);
                return tmp;
            }
        };

        constexpr DomainIrqHandle(IrqDomain& domain, irqnum_t irq) :
            domain_(&domain),
            irq_(irq)
        {
        }

        [[nodiscard]] constexpr IrqDomain& domain() const
        {
            assert(domain_);
            return *domain_;
        }

        [[nodiscard]] constexpr irqnum_t irq() const
        {
            assert(domain_);
            return irq_;
        }

        DomainIrqHandle& mapTo(IrqDomain& target_domain, irqnum_t target_irq = 0)
        {
            domain_->mapIrqTo(irq_, target_domain, target_irq);
            return *this;
        }

        DomainIrqHandle& removeMapping()
        {
            domain_->removeIrqMapping(irq_);
            return *this;
        }

        DomainIrqHandle& useHandler(const IrqInfo::handler_func_t& handler)
        {
            domain_->setIrqHandler(irq_, handler);
            return *this;
        }

        [[nodiscard]] auto handler() const { return domain_->getIrqHandler(irq_); }

        DomainIrqHandle& activateInDomain(bool activate)
        {
            domain_->activateIrq(irq_, activate);
            return *this;
        }

        DomainIrqHandle& handleInDomain()
        {
            domain_->handleIrq(irq_);
            return *this;
        }

        [[nodiscard]] auto forwardMappingChain() const
        {
            iterator it{domain_, irq_};
            return ranges::subrange{it, iterator::sentinel_t{}};
        }

        [[nodiscard]] auto reverseMappingTree() const
        {
            return ranges::subrange{tree_iterator(*this), tree_iterator::sentinel_t{}};
        }

        // Enable structured decomposition
        template<size_t N>
            requires(N < 2)
        constexpr auto get() const
        {
            if constexpr (N == 0)
                return domain_;
            else if constexpr (N == 1)
                return irq_;
        }

    private:
        IrqDomain* domain_;
        irqnum_t irq_;
    };

    [[nodiscard]] constexpr DomainIrqHandle irq(irqnum_t irq = 0) { return {*this, irq}; }

    void mapIrqTo(irqnum_t irq, IrqDomain& target_domain, irqnum_t target_irq);
    void removeIrqMapping(irqnum_t irq);

    void irqMappedBy(irqnum_t irq,
                     IrqDomain& source_domain,
                     irqnum_t source_irq,
                     IrqMappingTarget& reverse_ref);
    void removeIrqMappedBy(irqnum_t irq, IrqDomain& source_domain, irqnum_t source_irq);

    void setIrqHandler(irqnum_t irq, const IrqInfo::handler_func_t& handler);
    IrqInfo::handler_func_t getIrqHandler(irqnum_t irq);

    void handleIrq(irqnum_t irqnum);
    void activateIrq(irqnum_t irq, bool activate);

    [[nodiscard]] IrqInfo* irqInfo(irqnum_t irqnum);

    void printReverseMappingTree(irqnum_t irq);
    void printAllReverseMappings();

private:
    InterruptController* controller_ = nullptr;
    eastl::map<irqnum_t, IrqInfo> irqs_;
    eastl::string name_;
};

// Allow structured binding decomposition for DomainIrqHandle
namespace std
{
    template<>
    struct tuple_size<IrqDomain::DomainIrqHandle> : eastl::integral_constant<size_t, 2>
    {
    };

    template<> struct tuple_element<0, IrqDomain::DomainIrqHandle>
    {
        using type = IrqDomain*;
    };

    template<> struct tuple_element<1, IrqDomain::DomainIrqHandle>
    {
        using type = irqnum_t;
    };
} // namespace std
