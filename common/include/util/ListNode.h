/**
 * @file ListNode.h
 */

#ifndef ListNode_h___
#define ListNode_h___


#ifdef NON_SWEB_DEBUG___
typedef unsigned int uint32;
#else
#include <types.h>
#endif // NON_SWEB_DEBUG___


/**
 * @class ListNode provides an abstraction to array access
 */
template<typename ValueType>
class ListNode
{
  public:

    typedef ValueType value_type;

    typedef ValueType* pointer;

    typedef ValueType& reference;

    typedef const ValueType& const_reference;


  protected:

    /**
     * the element of the node
     */
    ValueType element_;

    /**
     * the next list node
     */
    ListNode<ValueType> *next_node_;

    /**
     * the previous list node
     */
    ListNode<ValueType> *previous_node_;


  public:
    /**
     * default constructor for class ListNode
     */
    ListNode();

    /**
     * constructor for class ListNode
     * @param element The element for the new node
     */
    ListNode ( const_reference element );

    /**
     * destructor for class ListNode
     */
    virtual ~ListNode();

    /**
     * copy constructor for class ListNode
     * @param instance is a constant reference to an object of
     * type ListNode
     */
    ListNode ( const ListNode<ValueType> &instance );

    /**
     * = operator for class ListNode
     * @param instance is a reference to a ListNode object
     * @return is a reference to a ListNode object
     */
    virtual ListNode<ValueType>& operator = (
        const ListNode<ValueType> &instance );

    /**
     * checks if the next node exists
     * @return true if the next node exists
     */
    bool hasNextNode() const;

    /**
     * checks if the previous node exists
     * @return true if the previous node exists
     */
    bool hasPreviousNode() const;

    /**
     * returns the next node
     * @return the next node
     */
    ListNode<ValueType> *getNextNode() const;

    /**
     * returns the previous node
     * @return the previous node
     */
    ListNode<ValueType> *getPreviousNode() const;

    /**
     * sets the next node
     * @param next_node the next node
     */
    void setNextNode ( ListNode<ValueType> *next_node );

    /**
     * sets the previous node
     * @param previous_node the previous node
     */
    void setPreviousNode ( ListNode<ValueType> *previous_node );

    /**
     * returns a reference to the node element
     * @return a reference to the node element
     */
    reference getElement();

    /**
     * returns a const reference to the node element
     * @return a const reference to the node element
     */
    const_reference getElement() const;

    /**
     * sets the element of the node
     * @param element The element to set
     */
    void setElement ( const_reference element );

};

#include "ListNode.tcpp"

#endif // ListNode_h___
