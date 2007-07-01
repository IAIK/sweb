#ifndef PAIR_H___
#define PAIR_H___

#ifdef NON_SWEB_DEBUG___
typedef unsigned int uint32;
#else
#include <types.h>
#endif // NON_SWEB_DEBUG___

#include "ListNode.h"

template<typename FirstValueType, typename SecondValueType>
class Pair
{
 public:

  typedef uint32 size_type;

  typedef FirstValueType first_value_type;

  typedef FirstValueType* first_pointer;

  typedef FirstValueType& first_reference;

  typedef const FirstValueType& first_const_reference;

  typedef SecondValueType second_value_type;

  typedef SecondValueType* second_pointer;

  typedef SecondValueType& second_reference;

  typedef const SecondValueType& second_const_reference;

protected:

  FirstValueType first_element_;

  SecondValueType second_element_;

public:

  /**
   * default constructor for class Pair
   *
   */
  Pair();

  /**
   * constructor for class Pair
   * @param first The first element for the new pair
   * @param second The second element for the new pair
   *
   */
  Pair(first_const_reference first, second_const_reference second);

  /**
   * destructor for class Pair
   */
  virtual ~Pair();

  /**
   * copy constructor for class ListNode
   * @param instance is a constant reference to an object of
   * type ListNode
   */
  Pair(const Pair<FirstValueType, SecondValueType> &instance);

  /**
   * = operator for class ListNode
   * @param instance is a reference to a ListNode object
   * @return is a reference to a ListNode object
   */
  virtual Pair<FirstValueType, SecondValueType>& operator = (
    const Pair<FirstValueType, SecondValueType> &instance);

  /**
   * returns a reference to the first element
   * @return a reference to the first element
   *
   */
  first_reference first();

  /**
   * returns a const reference to the first element
   * @return a const reference to the first element
   *
   */
  first_const_reference first() const;

  /**
   * returns a reference to the second element
   * @return a reference to the second element
   *
   */
  second_reference second();

  /**
   * returns a const reference to the second element
   * @return a const reference to the second element
   *
   */
  second_const_reference second() const;

  /**
   * sets the first element of the pair
   * @param element The element to set
   *
   */
  void setFirst(first_const_reference element);

  /**
   * sets the second element of the pair
   * @param element The element to set
   *
   */
  void setSecond(second_const_reference element);

};

#include "Pair.tcpp"

#endif
