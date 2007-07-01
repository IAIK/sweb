#ifndef MAP_H___
#define MAP_H___

#ifdef NON_SWEB_DEBUG___
typedef unsigned int uint32;
#else
#include <types.h>
#endif // NON_SWEB_DEBUG___

#include "ListNode.h"
#include "Pair.h"

/**
 * class Map provides a implementation of a map container
 *
 */

template<typename KeyType, typename ValueType>
class Map
{
 public:

  typedef uint32 size_type;


  typedef KeyType key_type;

  typedef KeyType* key_pointer;

  typedef KeyType& key_reference;

  typedef const KeyType& key_const_reference;


  typedef ValueType value_type;

  typedef ValueType* value_pointer;

  typedef ValueType& value_reference;

  typedef const ValueType& value_const_reference;


  typedef Pair<KeyType, ValueType> pair_value_type;

  typedef Pair<KeyType, ValueType>* pair_pointer;

  typedef Pair<KeyType, ValueType>& pair_reference;

  typedef const Pair<KeyType, ValueType>& pair_const_reference;


 protected:


  /**
   * pointer to the first node
   *
   */
  ListNode< pair_value_type > *first_node_;

  /**
   * pointer to the last node
   *
   */
  ListNode< pair_value_type > *last_node_;

  /**
   * the number of elements contained in the list
   *
   */
  size_type number_of_elements_;


 public:
  /**
   * default constructor for class Map
   * the node size will be set to a default value
   */
  Map();

  /**
   * destructor for class Map
   */
  virtual ~Map();


  /**
   * copy constructor for class Map
   * @param instance is a constant reference to an object of
   * type Map
   */
  Map(const Map<KeyType, ValueType> &instance);

  /**
   * = operator for class Map
   * @param instance is a reference to a Map object
   * @return is a reference to a Map object
   */
  virtual Map<KeyType, ValueType>& operator = (const Map<KeyType, ValueType> &instance);


  /**
   * returns the size (number of elements) of the map
   * @return the size (number of elements) of the map
   *
   */
  size_type size() const;

  /**
   * checks, if the map is empty
   * @return true, if the map is empty
   *
   */
  bool empty() const;

  /**
   * returns a reference to the element pair at the given index
   * @param index The index of the searched element pair
   * @return the element pair at the given index
   *
   */
  pair_reference operator[](size_type index);

  /**
   * returns a const reference to the element pair at the given index
   * @param index The index of the searched element pair
   * @return the element pair at the given index
   *
   */
  pair_const_reference operator[](size_type index) const;

  /**
   * adds the given element to the end of the Map
   * @param element a Pair with the Map's types
   *
   */
  void pushBack(pair_const_reference element);

  /**
   * adds the given element to the end of the Map
   * @param key_value the key to add
   * @param value_value the value to add
   *
   */
  void pushBack(KeyType key, ValueType value);

  /**
   * returns the index of the element with the given key
   * or size if not found
   * @param key the key to look for
   * @return the index of the element
   *
   */
  size_type find(KeyType key);

  /**
   * returns the Pair with the given key or a null-pointer if not found
   * @param key the key
   * @return the Pair looked for
   *
   */
  pair_pointer get(KeyType key);

  /**
   * returns the key at the given index
   * @param index the index
   * @return the key
   *
   */
  KeyType getKey(size_type index);

  /**
   * returns the value at the given index
   * @param index the index
   * @return the value
   *
   */
  ValueType getValue(size_type index);

  /**
   * removes an element by index
   * @param index the index
   *
   */
  void remove(size_type index);

  /**
   * removes an element by key
   * @param key the key
   *
   */
  void removeByKey(KeyType key);

  /**
   * cleares the map
   *
   */
  void clear();

};


#include "Map.tcpp"

#endif // MAP_H___
