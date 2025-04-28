#ifndef CODEREVIEWTASK_MYVECTOR_HPP
#define CODEREVIEWTASK_MYVECTOR_HPP

#include <vector>
#include <string>
#include <algorithm>
#include <stdexcept>
#include <iostream> // Added for potential debugging, but generally avoid in headers

/*
 * MyVector stores a collection of objects with their names.
 * For each object T, MyVector stores T`s name as std::string.
 * Several objects can have similar name.
 * operator[](const std::string& name) should return the first object
 * with the given name.
 *
 * Your task is to find as many mistakes and drawbacks in this code
 * (according to the presentation) as you can.
 * Annotate these mistakes with comments.
 *
 * Once you have found all the mistakes, rewrite the code
 * so it would not change its original purpose
 * and it would contain no mistakes.
 * Try to make the code more efficient without premature optimization.
 *
 * You can change MyVector interface completely, but there are several rules:
 * 1) you should correctly and fully implement copy-on-write idiom.
 * 2) std::pair<const T&, const std::string&> operator[](int index) const must take constant time at worst.
 * 3) const T& operator[](const std::string& name) const should be present.
 * 4) both operator[] should have non-const version.
 * 5) your implementation should provide all the member types of std::vector.
 * 6) your implementation should provide the following functions:
 * 1) begin(), cbegin(), end(), cend()
 * 2) empty(), size()
 * 3) reserve(), clear()
 */

template <typename T>
// MISTAKE 1: Public inheritance from std::vector is dangerous.
// std::vector (like other standard containers) does not have a virtual destructor.
// If a MyVector object is deleted via a std::vector<T>* pointer, the MyVector
// destructor will not be called, leading to resource leaks (m_ref_ptr, m_names).
// This violates the Liskov Substitution Principle in practice for polymorphic deletion.
// Composition should be preferred over inheritance here.
class MyVector : public std::vector<T>
{
public:
    MyVector()
    {
        // MISTAKE 2: Manual memory management with raw pointers.
        // Using `new` for the reference count and the names vector is prone to errors
        // (memory leaks, double deletes, exception safety issues).
        // std::shared_ptr should be used for reference counting and managing shared resources.
        m_ref_ptr = new size_t(1);
        m_names = new std::vector<std::string>();
        // DRAWBACK 1: The constructor doesn't pre-allocate memory for m_names,
        // even though it will likely grow in sync with the base vector.
    }

    // MISTAKE 3: Incorrect Copy-on-Write (CoW) implementation in copy constructor.
    // This constructor only shares the `m_names` and `m_ref_ptr`. The actual vector data
    // (inherited from std::vector<T>) is *deep copied* by the std::vector copy constructor.
    // This means modifying the vector data (e.g., `this->push_back(some_T)`) in one copy
    // will not affect the other, but modifying `m_names` (via `push_back(T, string)`)
    // will potentially affect the other (until `copy_names` is called).
    // A true CoW implementation should share *all* relevant data (vector elements and names)
    // until a modification occurs.
    MyVector(const MyVector& other)
        : std::vector<T>(other), // Deep copies vector data - breaks CoW for vector part
          m_ref_ptr(other.m_ref_ptr),
          m_names(other.m_names)
    {
        // MISTAKE 4: Potential race condition in multi-threaded environments.
        // Incrementing the reference count is not an atomic operation.
        (*m_ref_ptr)++;
    }

    // MISTAKE 5: Missing copy assignment operator (operator=).
    // The compiler-generated assignment operator will perform a shallow copy of
    // m_ref_ptr and m_names. This will lead to incorrect reference counting
    // (e.g., the resources of the assigned-to object might be leaked, and the
    // resources of the assigned-from object might be prematurely deleted).
    // It also doesn't handle self-assignment correctly.

    ~MyVector()
    {
        // MISTAKE 6: Potential race condition in multi-threaded environments.
        // Decrementing and checking the reference count is not atomic.
        // MISTAKE 7: Manual memory management cleanup is error-prone.
        // If an exception occurs during construction or elsewhere, this cleanup
        // might not happen correctly. Smart pointers handle this automatically.
        if (--*m_ref_ptr == 0)
        {
            delete m_ref_ptr; // Potential double delete if copy assignment was used incorrectly.
            delete m_names;   // Potential double delete.
        }
        // The base class (std::vector<T>) destructor is called automatically.
    }

    // MISTAKE 8: Incomplete CoW implementation in `push_back`.
    // `copy_names()` only copies the names vector if it's shared.
    // However, `std::vector<T>::push_back(obj)` modifies the base vector part,
    // which might *already* be shared conceptually (due to the flawed copy constructor).
    // This modification happens *before* checking/copying the vector data itself,
    // violating the CoW principle for the vector elements.
    // A correct CoW would check the reference count *before* any modification
    // (to either names or vector data) and copy *all* shared state if necessary.
    void push_back(const T& obj, const std::string& name)
    {
        copy_names(); // Only copies names, not the vector data.

        // Modifies the base vector directly. If the MyVector instance was copied,
        // this modifies the original vector data as well, which is incorrect for CoW.
        std::vector<T>::push_back(obj);
        m_names->push_back(name); // Modifies the potentially newly copied names vector.

        // DRAWBACK 2: Inconsistent state if std::vector<T>::push_back throws an exception
        // after copy_names() has potentially created new storage but before m_names->push_back().
        // The names vector might have been detached, but the element wasn't added.
    }

    // REQUIREMENT FULFILLED: O(1) complexity for const operator[](int).
    // DRAWBACK 3: Returning std::pair<const T&, const std::string&> prevents modification.
    // The requirement asks for a non-const version as well.
    std::pair<const T&, const std::string&> operator[](int index) const
    {
        // MISTAKE 9: Comparing signed int with unsigned size_t.
        // `index` should ideally be `size_t`. Comparing signed and unsigned integers
        // can lead to unexpected behavior if index is negative (though unlikely here).
        // Using `at()` method from std::vector is safer as it performs bounds checking.
        if (index >= std::vector<T>::size()) // size() returns size_t
        {
            // MISTAKE 10: Throwing exception objects by pointer (`new`).
            // Exceptions should be thrown by value: `throw std::out_of_range(...)`.
            // Throwing by pointer requires the catcher to delete it, which is rarely done
            // and leads to memory leaks.
            throw new std::out_of_range("Index is out of range");
        }

        // Accessing base class operator[] and m_names is O(1).
        return std::pair<const T&, const std::string&>(std::vector<T>::operator[](index), (*m_names)[index]);
    }

    // REQUIREMENT FULFILLED: const T& operator[](const std::string&) const exists.
    // DRAWBACK 4: Linear search complexity O(N).
    // Finding an element by name takes time proportional to the number of elements.
    // This might be inefficient for large vectors or frequent lookups.
    // A map-like structure could offer better performance (O(log N) or O(1) on average).
    // DRAWBACK 5: Only returns the object T, not the name. The interface might be
    // more consistent if it returned a pair like operator[](int).
    // DRAWBACK 6: Missing non-const version as required.
    const T& operator[](const std::string& name) const
    {
        // Uses std::find, which is O(N)
        std::vector<std::string>::const_iterator iter = std::find(m_names->begin(), m_names->end(), name);
        if (iter == m_names->end())
        {
            // MISTAKE 11: Throwing exception objects by pointer (`new`).
            throw new std::invalid_argument(name + " is not found in the MyVector");
        }

        // Calculates index using pointer arithmetic, then accesses base vector O(1).
        return std::vector<T>::operator[](iter - m_names->begin());
    }

private:
    // This function implements the "copy" part of CoW, but only for m_names.
    void copy_names()
    {
        // MISTAKE 12: Potential race condition (reading *m_ref_ptr). Not atomic.
        if (*m_ref_ptr == 1)
        {
            return; // No need to copy if not shared.
        }

        // MISTAKE 13: Potential memory leak if `new std::vector<std::string>` throws.
        // If the allocation for temp_names fails and throws an exception,
        // temp_ref_ptr (allocated with `new size_t(1)`) will be leaked because
        // there's no try/catch block or RAII wrapper (like unique_ptr) to clean it up.
        size_t* temp_ref_ptr = new size_t(1);
        std::vector<std::string>* temp_names = new std::vector<std::string>(*m_names); // Potential exception

        // MISTAKE 14: Potential race condition (decrementing *m_ref_ptr). Not atomic.
        (*m_ref_ptr)--;
        m_ref_ptr = temp_ref_ptr; // Raw pointer assignment.
        m_names = temp_names;     // Raw pointer assignment.
    }

private:
    // Use copy-on-write idiom for efficiency (not a premature optimization) <-- This comment is misleading, as CoW is only partially and incorrectly applied.
    std::vector<std::string>* m_names; // Raw pointer to shared names.
    size_t* m_ref_ptr;                 // Raw pointer to shared reference count.

    // MISTAKE 15: Missing other required members/types.
    // The class doesn't provide non-const operator[], begin(), cbegin(), end(), cend(),
    // empty(), size(), reserve(), clear(), or the member types (value_type, etc.)
    // explicitly as required. Some are inherited but might not behave correctly with CoW.
    // For example, calling an inherited non-const method like `clear()` would modify
    // the base vector part without triggering a copy, breaking CoW.
};


#endif //CODEREVIEWTASK_MYVECTOR_HPP
