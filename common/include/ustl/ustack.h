// This file is part of the uSTL library, an STL implementation.
//
// Copyright (c) 2005 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#pragma once

namespace ustl {

/// \class stack ustack.h ustl.h
/// \ingroup Sequences
///
/// \brief Stack adapter to uSTL containers.
///
template <typename T>
class stack {
public:
    typedef T			value_type;
    typedef size_t		size_type;
    typedef ptrdiff_t		difference_type;
    typedef T&			reference;
    typedef const T&		const_reference;
    typedef T*			pointer;
    typedef const T*		const_pointer;
public:
    inline			stack (void)			: _storage () { }
    explicit inline		stack (const vector<T>& s)	: _storage (s) { }
    explicit inline		stack (const stack& s)		: _storage (s._storage) { }
    inline bool			empty (void) const		{ return _storage.empty(); }
    inline size_type		size (void) const		{ return _storage.size(); }
    inline reference		top (void)			{ return _storage.back(); }
    inline const_reference	top (void) const		{ return _storage.back(); }
    inline void			push (const value_type& v)	{ _storage.push_back (v); }
    inline void			pop (void)			{ _storage.pop_back(); }
    inline bool			operator== (const stack& s) const	{ return _storage == s._storage; }
    inline bool			operator< (const stack& s) const	{ return _storage.size() < s._storage.size(); }
private:
    vector<T>			_storage;	///< Where the data actually is.
};

} // namespace ustl
