// This file is part of the uSTL library, an STL implementation.
//
// Copyright (c) 2005 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#pragma once

namespace ustl {

/// \class queue uqueue.h ustl.h
/// \ingroup Sequences
///
/// \brief Queue adapter to uSTL containers.
///
/// The most efficient way to use this implementation is to fill the queue
/// and the completely empty it before filling again.
///
template <typename T>
class queue {
public:
    typedef T			value_type;
    typedef size_t		size_type;
    typedef ptrdiff_t		difference_type;
    typedef T&			reference;
    typedef const T&		const_reference;
    typedef T*			pointer;
    typedef const T*		const_pointer;
public:
    inline			queue (void)			: _storage(), _front (0) { }
    explicit inline		queue (const vector<T>& s)	: _storage (s), _front (0) { }
    explicit inline		queue (const queue& s)		: _storage (s._storage), _front (0) { }
    inline size_type		size (void) const		{ return _storage.size() - _front; }
    inline bool			empty (void) const		{ return !size(); }
    inline reference		front (void)			{ return _storage [_front]; }
    inline const_reference	front (void) const		{ return _storage [_front]; }
    inline reference		back (void)			{ return _storage.back(); }
    inline const_reference	back (void) const		{ return _storage.back(); }
    inline void			push (const value_type& v);
    inline void			pop (void);
    inline bool			operator== (const queue& s) const	{ return _storage == s._storage && _front == s._front; }
    inline bool			operator< (const queue& s) const	{ return size() < s.size(); }
private:
    vector<T>			_storage;	///< Where the data actually is.
    size_type			_front;	///< Index of the element returned by next pop.
};

/// Pushes \p v on the queue.
template <typename T>
inline void queue<T>::push (const value_type& v)
{
    if (_front) {
	_storage.erase (_storage.begin(), _front);
	_front = 0;
    }
    _storage.push_back (v);
}

/// Pops the topmost element from the queue.
template <typename T>
inline void queue<T>::pop (void)
{
    if (++_front >= _storage.size())
	_storage.resize (_front = 0);
}

} // namespace ustl
