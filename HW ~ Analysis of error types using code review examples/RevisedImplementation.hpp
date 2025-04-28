#ifndef REWRITTEN_MYVECTOR_HPP
#define REWRITTEN_MYVECTOR_HPP

#include <vector>
#include <string>
#include <memory>       // For std::shared_ptr
#include <stdexcept>
#include <utility>      // For std::pair, std::make_pair
#include <iterator>     // For iterator tags
#include <algorithm>    // For std::find_if
#include <map>          // Optional: For faster name lookup

// Forward declaration
template <typename T>
class MyVector;

// Define a helper struct/class to hold the actual data and manage CoW state
template <typename T>
struct MyVectorData {
    // Store elements and names together
    std::vector<std::pair<T, std::string>> items;
    // Optional: Add a map for faster name lookup if needed
    // std::map<std::string, size_t> name_to_first_index; // Or unordered_multimap

    // Default constructor
    MyVectorData() = default;

    // Copy constructor (used when detaching for CoW)
    MyVectorData(const MyVectorData& other) : items(other.items) {
        // If using the optional map, copy or rebuild it here
        // build_name_map(); // Example if map needs rebuilding
    }

    // Optional: Helper to build/update the name map
    /*
    void build_name_map() {
        name_to_first_index.clear();
        for (size_t i = 0; i < items.size(); ++i) {
            // Only insert if the name is not already present (for first occurrence)
            name_to_first_index.try_emplace(items[i].second, i);
        }
    }
    */
};

// Iterator class (simplified example, could be more robust)
// Needed to satisfy begin()/end() requirements and provide vector-like iteration
template <typename T>
class MyVectorIterator {
public:
    // --- Member types required by C++ standard iterators ---
    using iterator_category = std::random_access_iterator_tag;
    using value_type = std::pair<T, std::string>;
    using difference_type = std::ptrdiff_t;
    using pointer = value_type*;
    using reference = value_type&;
    // --- Non-standard, but useful ---
    using vector_iterator = typename std::vector<value_type>::iterator;

private:
    vector_iterator m_iter; // Internal iterator for the vector

public:
    // Constructor
    MyVectorIterator(vector_iterator it) : m_iter(it) {}

    // Dereference
    reference operator*() const { return *m_iter; }
    pointer operator->() const { return &(*m_iter); }

    // Increment/Decrement
    MyVectorIterator& operator++() { ++m_iter; return *this; }
    MyVectorIterator operator++(int) { MyVectorIterator temp = *this; ++(*this); return temp; }
    MyVectorIterator& operator--() { --m_iter; return *this; }
    MyVectorIterator operator--(int) { MyVectorIterator temp = *this; --(*this); return temp; }

    // Arithmetic
    MyVectorIterator& operator+=(difference_type n) { m_iter += n; return *this; }
    MyVectorIterator operator+(difference_type n) const { return MyVectorIterator(m_iter + n); }
    friend MyVectorIterator operator+(difference_type n, const MyVectorIterator& it) { return it + n; } // Non-member friend

    MyVectorIterator& operator-=(difference_type n) { m_iter -= n; return *this; }
    MyVectorIterator operator-(difference_type n) const { return MyVectorIterator(m_iter - n); }
    difference_type operator-(const MyVectorIterator& other) const { return m_iter - other.m_iter; }

    // Comparison
    bool operator==(const MyVectorIterator& other) const { return m_iter == other.m_iter; }
    bool operator!=(const MyVectorIterator& other) const { return m_iter != other.m_iter; }
    bool operator<(const MyVectorIterator& other) const { return m_iter < other.m_iter; }
    bool operator>(const MyVectorIterator& other) const { return m_iter > other.m_iter; }
    bool operator<=(const MyVectorIterator& other) const { return m_iter <= other.m_iter; }
    bool operator>=(const MyVectorIterator& other) const { return m_iter >= other.m_iter; }

    // Offset dereference
    reference operator[](difference_type n) const { return m_iter[n]; }
};


template <typename T>
class MyVector {
public:
    // --- Member Types (as required, mirroring std::vector where applicable) ---
    using value_type = std::pair<T, std::string>; // Stores both object and name
    using reference = value_type&;
    using const_reference = const value_type&;
    using pointer = value_type*;
    using const_pointer = const value_type*;
    using iterator = MyVectorIterator<T>; // Use our custom iterator
    using const_iterator = MyVectorIterator<const T>; // Need a const version too (simplified here)
    // Read that a full const_iterator implementation is more involved if T itself needs constness propagation.
    // For simplicity, this example might need refinement for a fully const-correct const_iterator.
    using size_type = typename std::vector<value_type>::size_type;
    using difference_type = typename std::vector<value_type>::difference_type;
    // Add other types like reverse_iterator if needed

private:
    // Shared pointer to the actual data (vector of pairs)
    std::shared_ptr<MyVectorData<T>> m_data;

    // --- Copy-on-Write Helper ---
    // Ensures unique ownership before modification.
    // If the data is shared (ref count > 1), it creates a deep copy.
    void detach() {
        if (!m_data) { // Handle case where vector was moved-from or default constructed
             m_data = std::make_shared<MyVectorData<T>>();
        } else if (m_data.use_count() > 1) {
            // More than one MyVector shares this data, create a copy
            m_data = std::make_shared<MyVectorData<T>>(*m_data); // Deep copy happens here
        }
        // Now m_data points to a unique copy (or was already unique)
    }

public:
    // --- Constructors ---
    // Default constructor
    MyVector() : m_data(std::make_shared<MyVectorData<T>>()) {}

    // Copy constructor (shallow copy, increments ref count via shared_ptr)
    MyVector(const MyVector& other) = default; // Default works perfectly with shared_ptr

    // Move constructor (efficiently transfers ownership)
    MyVector(MyVector&& other) noexcept = default; // Default works perfectly

    // Destructor (automatically handled by shared_ptr)
    ~MyVector() = default;

    // --- Assignment Operators ---
    // Copy assignment (uses copy-and-swap idiom implicitly via default)
    MyVector& operator=(const MyVector& other) = default; // Default handles CoW correctly

    // Move assignment (efficiently transfers ownership)
    MyVector& operator=(MyVector&& other) noexcept = default; // Default handles CoW correctly


    // --- Element Access ---

    // operator[](int index) - const version O(1))
    // Returns a *copy* of the pair to prevent modification through const reference.
    // Returning const references directly can be tricky with CoW if not careful.
    // A const reference to internal data is fine as long as no non-const methods are called.
    const_reference operator[](size_type index) const {
        // shared_ptr provides thread-safe access to the managed object if no modifications occur.
        // I've read that bounds checking'srecommended for robustness, similar to std::vector::at()
        if (index >= m_data->items.size()) {
             throw std::out_of_range("Index out of range");
        }
        return m_data->items[index];
    }

    // operator[](int index) - non-const version (Requirement 4)
    // Must trigger CoW before returning a modifiable reference.
    reference operator[](size_type index) {
        detach(); // Ensure unique copy before allowing modification
        // Bounds checking
        if (index >= m_data->items.size()) {
             throw std::out_of_range("Index out of range");
        }
        // If using an optional name map, modification here might require updating the map
        // m_data->update_map_for_item(index); // Example
        return m_data->items[index];
    }

    // operator[](const std::string& name) - const version 
    // Returns reference to the *first* T found with the given name.
    // Complexity: O(N) without optimization, potentially O(log N) or O(1) with map.
    const T& operator[](const std::string& name) const {
        // Use std::find_if for searching within the vector of pairs
        auto it = std::find_if(m_data->items.cbegin(), m_data->items.cend(),
                               [&name](const value_type& element) {
                                   return element.second == name; // Compare name part of pair
                               });

        if (it == m_data->items.cend()) {
            throw std::invalid_argument("Name not found in MyVector: " + name);
        }
        return it->first; // Return const reference to the T part of the pair
        // Optional: If using name_to_first_index map:
        // auto map_it = m_data->name_to_first_index.find(name);
        // if (map_it == m_data->name_to_first_index.end()) { ... throw ... }
        // return m_data->items[map_it->second].first; // O(log N) or O(1) access
    }

    // operator[](const std::string& name) - non-const version (Requirement 4)
    // Must trigger CoW before returning a modifiable reference.
    T& operator[](const std::string& name) {
        detach(); // Ensure unique copy before potential modification

        // Find the element (similar to const version)
        auto it = std::find_if(m_data->items.begin(), m_data->items.end(),
                               [&name](const value_type& element) {
                                   return element.second == name;
                               });

        if (it == m_data->items.end()) {
            throw std::invalid_argument("Name not found in MyVector: " + name);
        }
        // If using an optional name map, modification might require map update
        // m_data->update_map_for_item(std::distance(m_data->items.begin(), it));
        return it->first; // Return reference to the T part
        // Optional: If using name_to_first_index map:
        // auto map_it = m_data->name_to_first_index.find(name);
        // if (map_it == m_data->name_to_first_index.end()) { ... throw ... }
        // return m_data->items[map_it->second].first;
    }


    // --- Modifiers ---

    // Add an element with its name
    void push_back(const T& obj, const std::string& name) {
        detach(); // Ensure unique copy before modification
        m_data->items.emplace_back(obj, name); // Use emplace_back for efficiency
        // Optional: Update name map if used
        // m_data->update_map_after_push_back(name);
    }

    // Add an element using a pair
     void push_back(const value_type& value) {
        detach();
        m_data->items.push_back(value);
        // Optional: Update name map
        // m_data->update_map_after_push_back(value.second);
     }

     void push_back(value_type&& value) {
        detach();
        m_data->items.push_back(std::move(value));
        // Optional: Update name map
        // m_data->update_map_after_push_back(m_data->items.back().second); 
     }

    // Clear all elements
    void clear() {
        detach(); // Ensure unique copy (or create new empty one if needed)
        m_data->items.clear();
        // Optional: Clear name map
        // m_data->name_to_first_index.clear();
    }

    // Reserve capacity
    void reserve(size_type new_cap) {
        detach(); // Ensure unique copy before modification
        m_data->items.reserve(new_cap);
    }


    // --- Capacity ---

    // Check if empty
    [[nodiscard]] bool empty() const noexcept {
        // No modification, no detach needed. Thread-safe read via shared_ptr.
        return !m_data || m_data->items.empty();
    }

    // Get size
    size_type size() const noexcept {
        // No modification, no detach needed. Thread-safe read via shared_ptr.
        return m_data ? m_data->items.size() : 0;
    }


    // --- Iterators ---

    // begin() - non-const
    iterator begin() {
        detach(); // Ensure unique copy before allowing modification via iterator
        return iterator(m_data->items.begin());
    }

    // cbegin() - const
    const_iterator cbegin() const noexcept {
        // Const access, no detach needed
        // Note: Returning a const_iterator based on the non-const vector iterator.
        // A potentially better implementation might require a dedicated const vector iterator type.
        return const_iterator(m_data ? m_data->items.cbegin() : typename std::vector<value_type>::const_iterator{});
    }

    // begin() - const version
    const_iterator begin() const noexcept {
        return cbegin();
    }

    // end() - non-const
    iterator end() {
        detach(); // Ensure unique copy
        return iterator(m_data->items.end());
    }

    // cend() - const
    const_iterator cend() const noexcept {
        // Const access, no detach needed
         return const_iterator(m_data ? m_data->items.cend() : typename std::vector<value_type>::const_iterator{});
    }

    // end() - const version
    const_iterator end() const noexcept {
        return cend();
    }

    // --- Swap ---
    void swap(MyVector& other) noexcept {
        // Swapping shared_ptrs is efficient and exception-safe
        using std::swap;
        swap(m_data, other.m_data);
    }
};

// Non-member swap function
template <typename T>
void swap(MyVector<T>& lhs, MyVector<T>& rhs) noexcept {
    lhs.swap(rhs);
}


#endif 
