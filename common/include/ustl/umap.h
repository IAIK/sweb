// This file is part of the uSTL library, an STL implementation.
//
// Copyright (c) 2005 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#ifndef UMAP_H_45643F516E02A87A3DCEA5024052A6F5
#define UMAP_H_45643F516E02A87A3DCEA5024052A6F5

#include "uvector.h"
#include "ufunction.h"

namespace ustl {

/// \class map umap.h ustl.h
/// \ingroup AssociativeContainers
///
/// \brief A sorted associative container of pair<K,V>
///
template <typename K, typename V, typename Comp = less<K> >
class map : public vector<pair<K,V> > {
public:
    typedef K						key_type;
    typedef V						data_type;
    typedef const K&					const_key_ref;
    typedef const V&					const_data_ref;
    typedef const map<K,V,Comp>&			rcself_t;
    typedef vector<pair<K,V> >				base_class;
    typedef typename base_class::value_type		value_type;
    typedef typename base_class::size_type		size_type;
    typedef typename base_class::pointer		pointer;
    typedef typename base_class::const_pointer		const_pointer;
    typedef typename base_class::reference		reference;
    typedef typename base_class::const_reference	const_reference;
    typedef typename base_class::const_iterator		const_iterator;
    typedef typename base_class::iterator		iterator;
    typedef typename base_class::reverse_iterator	reverse_iterator;
    typedef typename base_class::const_reverse_iterator	const_reverse_iterator;
    typedef pair<const_iterator,const_iterator>		const_range_t;
    typedef pair<iterator,iterator>			range_t;
    typedef pair<iterator,bool>				insertrv_t;
public:
    inline			map (void)			: base_class() {}
    explicit inline		map (size_type n)		: base_class (n) {}
    inline			map (rcself_t v)		: base_class (v) {}
    inline			map (const_iterator i1, const_iterator i2) : base_class() { insert (i1, i2); }
    inline rcself_t		operator= (rcself_t v)		{ base_class::operator= (v); return (*this); }
    inline const_data_ref	operator[] (const_key_ref i) const;
    data_type&			operator[] (const_key_ref i);
    inline size_type		size (void) const		{ return (base_class::size()); }
    inline iterator		begin (void)			{ return (base_class::begin()); }
    inline const_iterator	begin (void) const		{ return (base_class::begin()); }
    inline iterator		end (void)			{ return (base_class::end()); }
    inline const_iterator	end (void) const		{ return (base_class::end()); }
    inline void			assign (const_iterator i1, const_iterator i2)	{ clear(); insert (i1, i2); }
    inline void			push_back (const_reference v)			{ insert (v); }
    inline const_iterator	find (const_key_ref k) const;
    inline iterator		find (const_key_ref k)	{ return (const_cast<iterator> (const_cast<rcself_t>(*this).find (k))); }
    inline const_iterator	find_data (const_data_ref v, const_iterator first = NULL, const_iterator last = NULL) const;
    inline iterator		find_data (const_data_ref v, iterator first = NULL, iterator last = NULL)	{ return (const_cast<iterator> (find_data (v, const_cast<const_iterator>(first), const_cast<const_iterator>(last)))); }
    insertrv_t			insert (const_reference v);
    inline iterator		insert (iterator, const_reference v)	{ return (insert(v).first); }
    void			insert (const_iterator i1, const_iterator i2);
    inline void			erase (const_key_ref k);
    inline iterator		erase (iterator ep)	{ return (base_class::erase (ep)); }
    inline iterator		erase (iterator ep1, iterator ep2) { return (base_class::erase (ep1, ep2)); }
    inline void			clear (void)		{ base_class::clear(); }
    const_iterator		lower_bound (const_key_ref k) const;
    inline iterator		lower_bound (const_key_ref k) { return (const_cast<iterator>(const_cast<rcself_t>(*this).lower_bound (k))); }
    const_iterator		upper_bound (const_key_ref k) const;
    inline iterator		upper_bound (const_key_ref k) { return (const_cast<iterator>(const_cast<rcself_t>(*this).upper_bound (k))); }
    const_range_t		equal_range (const_key_ref k) const;
    inline range_t		equal_range (const_key_ref k) { return (const_cast<const_range_t>(const_cast<rcself_t>(*this).equal_range (k))); }
};

template <typename K, typename V, typename Comp>
typename map<K,V,Comp>::const_iterator map<K,V,Comp>::lower_bound (const_key_ref k) const
{
    const_iterator first (begin()), last (end());
    while (first != last) {
	const_iterator mid = advance (first, distance (first,last) / 2);
	if (Comp()(mid->first, k))
	    first = advance (mid, 1);
	else
	    last = mid;
    }
    return (first);
}

template <typename K, typename V, typename Comp>
typename map<K,V,Comp>::const_iterator map<K,V,Comp>::upper_bound (const_key_ref k) const
{
    const_iterator first (begin()), last (end());
    while (first != last) {
	const_iterator mid = advance (first, distance (first,last) / 2);
	if (Comp()(k, mid->first))
	    last = mid;
	else
	    first = mid + 1;
    }
    return (last);
}

template <typename K, typename V, typename Comp>
typename map<K,V,Comp>::const_range_t map<K,V,Comp>::equal_range (const_key_ref k) const
{
    const_range_t rv;
    rv.second = rv.first = lower_bound (k);
    while (rv.second != end() && !Comp()(k, rv.second->first))
	++rv.second;
    return (rv);
}

/// Returns the pair<K,V> where K = \p k.
template <typename K, typename V, typename Comp>
inline typename map<K,V,Comp>::const_iterator map<K,V,Comp>::find (const_key_ref k) const
{
    const_iterator i = lower_bound (k);
    return ((i < end() && Comp()(k,i->first)) ? end() : i);
}

/// Returns the pair<K,V> where V = \p v, occuring in range [first,last).
template <typename K, typename V, typename Comp>
inline typename map<K,V,Comp>::const_iterator map<K,V,Comp>::find_data (const_data_ref v, const_iterator first, const_iterator last) const
{
    if (!first) first = begin();
    if (!last) last = end();
    for (; first != last && first->second != v; ++first) ;
    return (first);
}

/// Returns data associated with key \p k.
template <typename K, typename V, typename Comp>
inline const typename map<K,V,Comp>::data_type& map<K,V,Comp>::operator[] (const_key_ref k) const
{
    assert (find(k) != end() && "operator[] const can not insert non-existent keys");
    return (find(k)->second);
}

/// Returns data associated with key \p k.
template <typename K, typename V, typename Comp>
typename map<K,V,Comp>::data_type& map<K,V,Comp>::operator[] (const_key_ref k)
{
    iterator ip = lower_bound (k);
    if (ip == end() || Comp()(k,ip->first))
	ip = base_class::insert (ip, make_pair (k, V()));
    return (ip->second);
}

/// Inserts the pair into the container.
template <typename K, typename V, typename Comp>
typename map<K,V,Comp>::insertrv_t map<K,V,Comp>::insert (const_reference v)
{
    iterator ip = lower_bound (v.first);
    bool bInserted = ip == end() || Comp()(v.first, ip->first);
    if (bInserted)
	ip = base_class::insert (ip, v);
    return (make_pair (ip, bInserted));
}

/// Inserts elements from range [i1,i2) into the container.
template <typename K, typename V, typename Comp>
void map<K,V,Comp>::insert (const_iterator i1, const_iterator i2)
{
    assert (i1 <= i2);
    base_class::reserve (size() + distance (i1, i2));
    for (; i1 != i2; ++i1)
	insert (*i1);
}

/// Erases the element with key value \p k.
template <typename K, typename V, typename Comp>
inline void map<K,V,Comp>::erase (const_key_ref k)
{
    iterator ip = find (k);
    if (ip != end())
	erase (ip);
}

} // namespace ustl

#endif
