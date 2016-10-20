// ------------------------------
// projects/allocator/Allocator.h
// Copyright (C) 2016
// Glenn P. Downing
// ------------------------------

#ifndef Allocator_h
#define Allocator_h

// --------
// includes
// --------

#include <cassert>   // assert
#include <cstddef>   // ptrdiff_t, size_t
#include <new>       // bad_alloc, new
#include <stdexcept> // invalid_argument
#include <stdio.h>
#include <stdlib.h>

// ---------
// Allocator
// ---------

template <typename T, std::size_t N> class my_allocator {
public:
  // --------
  // typedefs
  // --------

  using value_type = T;

  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;

  using pointer = value_type *;
  using const_pointer = const value_type *;

  using reference = value_type &;
  using const_reference = const value_type &;

public:
  // -----------
  // operator ==
  // -----------

  friend bool operator==(const my_allocator &, const my_allocator &) {
    return false;
  } // this is correct

  // -----------
  // operator !=
  // -----------

  friend bool operator!=(const my_allocator &lhs, const my_allocator &rhs) {
    return !(lhs == rhs);
  }

private:
  // ----
  // data
  // ----

  char a[N];

  // -----
  // valid
  // -----

  /**
   * O(1) in space
   * O(n) in time
   * <your documentation>
   */
  bool valid() const {
    // <your code>
    std::size_t i = 0;
    int leading_sent, trailing_sent, last_sent = 0;

    while (i < N) {

      leading_sent = (*this)[i]; // check leading block sentinel

      if ((last_sent > 0) &&
          (leading_sent > 0)) // if this block and last block are both free
        return false;             // return false

      i = i + 4 + abs(leading_sent); // check trailing block sentinel
      trailing_sent = (*this)[i];

      if (leading_sent !=
          trailing_sent) // make sure trailing and leading sentinels match
        return false;

      last_sent = trailing_sent; // advance i and set last_sent
      i += 4;
    }
    return true; // all blocks valid
  }

  /**
   * O(1) in space
   * O(1) in time
   * https://code.google.com/p/googletest/wiki/AdvancedGuide#Private_Class_Members
   */
  FRIEND_TEST(TestAllocator2, index);
  int &operator[](int i) { return *reinterpret_cast<int *>(&a[i]); }

public:
  // ------------
  // constructors
  // ------------

  /**
   * O(1) in space
   * O(1) in time
   * throw a bad_alloc exception, if N is less than sizeof(T) + (2 *
   * sizeof(int))
   */
  my_allocator() {
    // <your code>
    int space_needed = (sizeof(T) + (2 * sizeof(int))); // needs space for 2 int sentinels and at least one T
    if (N < space_needed){
        std::bad_alloc e;
        throw e;
    }

    (*this)[0] = N - 8; // set initial sentinel values
    (*this)[N - 4] = N - 8;

    assert(valid());
  }

  my_allocator(const my_allocator &) = default;
  ~my_allocator() = default;
  my_allocator &operator=(const my_allocator &) = default;

  // --------
  // allocate
  // --------

  /**
   * O(1) in space
   * O(n) in time
   * after allocation there must be enough space left for a valid block
   * the smallest allowable block is sizeof(T) + (2 * sizeof(int))
   * choose the first block that fits
   * throw a bad_alloc exception, if n is invalid
   */
  pointer allocate(size_type n) {
    // <your code>
    assert(valid());

    std::bad_alloc e;
    if (n <= 0) throw e;
    
    int space_needed = 2 * sizeof(int) + (n * sizeof(T));	//space requested plus sentinels
    size_t i = 0;
    int space_available;	//space available including free space and space taken by sentinels

	while(i < N){
		space_available = (*this)[i] + (2 * sizeof(int));
		if(space_available >= space_needed){
			if(space_available - space_needed < sizeof(T) + 2 * sizeof(int)){	//not enough space left over
				(*this)[i] *= -1;
				(*this)[i + space_available - 4] = (*this)[i];			//allocate entire block
				return reinterpret_cast<pointer>(&(*this)[i + 4]);
			}	

			else{
				(*this)[i] = n * sizeof(T) * -1;	//set new block sentinels
				(*this)[i + (n * sizeof(T)) + 4] = n * sizeof(T) * -1;
				(*this)[i + (n * sizeof(T)) + 8] = space_available - space_needed - 8;
				(*this)[i + (n * sizeof(T)) + 4 + space_available - space_needed] = space_available - space_needed - 8;
				return reinterpret_cast<pointer>(&(*this)[i + 4]);
			}
		}

		if((*this)[i] > 0) i = i + (*this)[i] + (2 * sizeof(int));
		else i = i - (*this)[i] + (2 * sizeof(int));
	}
	throw e;

  } 
  // replace!

  // ---------
  // construct
  // ---------

  /**
   * O(1) in space
   * O(1) in time
   */
  void construct(pointer p, const_reference v) {
    new (p) T(v); // this is correct and exempt
    assert(valid());
  } // from the prohibition of new

  // ----------
  // deallocate
  // ----------

  /**
   * O(1) in space
   * O(1) in time
   * after deallocation adjacent free blocks must be coalesced
   * throw an invalid_argument exception, if p is invalid
   * <This will deallocate the given block then coalesce free blocks throughout the backing store>
   */
  void deallocate(pointer p, size_type n) {
	assert(valid());

	if(p == nullptr || p < reinterpret_cast<const pointer>(&(*this)[0]) || p > reinterpret_cast<const pointer>(&(*this)[N - 1])){
		std::invalid_argument e("bad pointer");
		throw e;
	}

	int* leading_sent;	//cast pointer to int pointer
	int* trailing_sent;
	leading_sent = reinterpret_cast<int*>(p) - 1;	//cast p to int pointer, move back 4 bytes to point to sentinel
	int  neg = *leading_sent;
	*leading_sent = neg * -1;			//reassign block as free
	trailing_sent = reinterpret_cast<int*>(p + n);	//point to trailing sentinel
  	*trailing_sent = neg * -1;

	size_t i = 0;

	int leading_sentinel, trailing_sentinel;
        while(i < N){
        	if(i == 0){	//advance i to second leading sentinel, compare with previous trailing sentinel
			int value = abs((*this)[i]);
			i = i + value + 8;
		}

		leading_sentinel = (*this)[i];		//current block leading sentinel
		trailing_sentinel = (*this)[i - 4];	//prev block trailing sentinel
		if(leading_sentinel > 0 && trailing_sentinel > 0){
			int coalesced_space = (*this)[i] + (*this)[i - 4] + 8;	//2 free spaces plus 2 freed sentinels
			(*this)[(*this)[i] + i + 4] = coalesced_space;
			(*this)[i - 8 - (*this)[i-4]] = coalesced_space;
			i = 0;
		}

		i = i + abs((*this)[i]) + 8;
	}
	
  }

  // -------
  // destroy
  // -------

  /**
   * O(1) in space
   * O(1) in time
   */
  void destroy(pointer p) {
    p->~T(); // this is correct
    assert(valid());
  }

  /**
   * O(1) in space
   * O(1) in time
   */
  const int &operator[](int i) const {
    return *reinterpret_cast<const int *>(&a[i]);
  }
};

#endif // Allocator_h
