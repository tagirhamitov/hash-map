#include <algorithm>
#include <cstddef>
#include <functional>
#include <initializer_list>
#include <stdexcept>
#include <vector>

/* Hash map with open addressing.
 * When size is more than 3/4 of capacity, it doubles.
 * All elements are located in linked list for iterating over them. */
template<class KeyType, class ValueType, class Hash = std::hash<KeyType> >
class HashMap {
 private:
    /* All elements are located in linked list.
     * Item - class that contains element of linked list. */
    class Item;

 public:
    // Constructs vector from hasher.
    HashMap(const Hash& hasher = Hash()) : hasher_(hasher) {
        init();
    }

    // Constructs vector from 2 iterators and hasher.
    template<typename Iter>
    HashMap(Iter first, Iter last, const Hash& hasher = Hash()) : hasher_(hasher) {
        init();

        for (auto it = first; it != last; ++it) {
            insert(*it);
        }
    }

    // Constructs vector from initializer_list.
    HashMap(std::initializer_list<std::pair<KeyType, ValueType> > initList,
            const Hash& hasher = Hash()) : hasher_(hasher) {

        init();

        for (const auto& keyValue : initList) {
            insert(keyValue);
        }
    }

    // Copy constructor.
    HashMap(const HashMap& other, const Hash& hasher = Hash()) : hasher_(hasher) {
        init();

        for (auto it = other.begin(); it != other.end(); ++it) {
            insert(*it);
        }
    }

    // Assignment operator.
    HashMap& operator=(const HashMap& other) {
        if (&other != this) {
            delete_all();
            for (auto it = other.begin(); it != other.end(); ++it) {
                insert(*it);
            }
        }
        return *this;
    }

    // Returns number of elements in hash map.
    size_t size() const {
        return size_;
    }

    /* If hash map contains one or more elements, returns 'true'.
     * Else returns 'false' */
    bool empty() const {
        return size_ == 0;
    }

    // Return hasher object.
    Hash hash_function() const {
        return hasher_;
    }

    // Inserts element by O(1) amortized
    void insert(const std::pair<KeyType, ValueType>& keyValue) {
        const KeyType& key = keyValue.first;
        const ValueType& value = keyValue.second;

        size_t pos = find_pos(key);

        if (storage[pos] == nullptr) {
            storage[pos] = create_item(key, value);
            storage[pos]->pos = pos;
            ++size_;

            resize_if_need();
        }
    }

    // Deletes element by O(1) amortized
    void erase(const KeyType& key) {
        size_t pos = find_pos(key);

        if (storage[pos] != nullptr) {
            delete_item(storage[pos]);
            storage[pos] = nullptr;
            --size_;

            size_t nextPos = find_next(pos);
            while (storage[nextPos] != nullptr) {
                storage[pos] = storage[nextPos];
                storage[nextPos] = nullptr;
                storage[pos]->pos = pos;

                pos = nextPos;
                nextPos = find_next(pos);
            }

            resize_if_need();
        }
    }

    // Required class 'iterator'. Contains pointer to current element.
    class iterator {
     private:
        Item* item;

     public:
        iterator() = default;

        explicit iterator(Item* item) : item(item) {
        }

        std::pair<const KeyType, ValueType>* operator->() {
            return &item->keyValue;
        }

        std::pair<const KeyType, ValueType>& operator*() {
            return item->keyValue;
        }

        iterator operator++() {
            item = item->next;
            return *this;
        }

        iterator operator++(int) {
            iterator prev = *this;
            item = item->next;
            return prev;
        }

        bool operator==(const iterator& other) {
            return item == other.item;
        }

        bool operator!=(const iterator& other) {
            return item != other.item;
        }
    };

    // Required class 'const_iterator'. Contains pointer to current element.
    class const_iterator {
     private:
        Item* item;

     public:
        const_iterator() = default;

        explicit const_iterator(Item* item) : item(item) {
        }

        const std::pair<const KeyType, ValueType>* operator->() {
            return &item->keyValue;
        }

        const std::pair<const KeyType, ValueType>& operator*() {
            return item->keyValue;
        }

        const_iterator operator++() {
            item = item->next;
            return *this;
        }

        const_iterator operator++(int) {
            const_iterator prev = *this;
            item = item->next;
            return prev;
        }

        bool operator==(const const_iterator& other) {
            return item == other.item;
        }

        bool operator!=(const const_iterator& other) {
            return item != other.item;
        }
    };

    // Returns iterator of first element.
    iterator begin() {
        return iterator(_begin);
    }

    // Returns iterator of element after last element.
    iterator end() {
        return iterator(_end);
    }

    // Returns const_iterator of first element.
    const_iterator begin() const {
        return const_iterator(_begin);
    }

    // Returns const_iterator of element after last element.
    const_iterator end() const {
        return const_iterator(_end);
    }

    /* If 'key' is in hash map - returns its iterator,
     * else returns end().
     * */
    iterator find(const KeyType& key) {
        size_t pos = find_pos(key);

        if (storage[pos] == nullptr) {
            return iterator(_end);
        }

        return iterator(storage[pos]);
    }

    // Constant version of find().
    const_iterator find(const KeyType& key) const {
        size_t pos = find_pos(key);

        if (storage[pos] == nullptr) {
            return const_iterator(_end);
        }

        return const_iterator(storage[pos]);
    }

    /* If 'key' is in hash map - returns it's reference,
     * else creates new element with key 'key' and returns it's reference.
     * */
    ValueType& operator[](const KeyType& key) {
        size_t pos = find_pos(key);

        if (storage[pos] == nullptr) {
            storage[pos] = create_item(key, ValueType());
            storage[pos]->pos = pos;
            ++size_;

            resize_if_need();

            pos = find_pos(key);
        }

        return storage[pos]->keyValue.second;
    }

    // Similar to operator[] but throws an exception if 'key' isn't in hash map.
    const ValueType& at(const KeyType& key) const {
        size_t pos = find_pos(key);

        if (storage[pos] == nullptr) {
            throw std::out_of_range("No such key in the hash table");
        }

        return storage[pos]->keyValue.second;
    }

    // Deletes all elements from hash map.
    void clear() {
        Item* item = _begin;
        while (item != _end) {
            Item* next = item->next;
            storage[item->pos] = nullptr;
            delete_item(item);
            item = next;
        }
        size_ = 0;
    }

    // Destroys hash map and frees the memory
    ~HashMap() {
        delete_all();
        delete _end;
    }

 private:
    size_t size_;  // size
    size_t capacity_;  // capacity
    Hash hasher_;

    /* All elements are located in linked list
     * Item - class that contains element of linked list. */
    class Item {
     private:
        // Connects neighbors with current element.
        void connect() {
            if (next != nullptr) {
                next->prev = this;
            }

            if (prev != nullptr) {
                prev->next = this;
            }
        }

        // Cuts current element from list and connects its neighbors.
        void disconnect() {
            if (next != nullptr) {
                next->prev = prev;
            }

            if (prev != nullptr) {
                prev->next = next;
            }
        }

     public:
        std::pair<const KeyType, ValueType> keyValue;  // Pair of key and value
        Item* prev;  // Pointer to the previous element
        Item* next;  // Pointer to the next element
        size_t pos;  // Position of element in 'storage' vector

        // Constructs Item from key, value and pointers to it's neighbors.
        Item(const KeyType& key, const ValueType& value, Item* prev, Item* next) :
                keyValue({key, value}),
                prev(prev),
                next(next) {

            connect();
        }

        // Construct Item from it's neighbors, key and value don't matter.
        Item(Item* prev, Item* next) :
                prev(prev),
                next(next) {

            connect();
        }

        // Destroys item and removes it from linked list.
        ~Item() {
            disconnect();
        }
    };

    Item* _begin;  // First element in linked list
    Item* _end;  // Last element in linked list

    // Pointers to linked list elements and the main storage array for hash map.
    std::vector<Item*> storage;

    // Initialize properties for empty hash map by O(1).
    void init() {
        size_ = 0;
        capacity_ = 1;

        storage.assign(1, nullptr);

        _end = new Item(nullptr, nullptr);
        _begin = _end;
    }

    // Changes capacity to newCapacity by O(n) where n is number of elements.
    void resize(size_t newCapacity) {
        storage.assign(newCapacity, nullptr);

        capacity_ = newCapacity;
        size_ = 0;

        for (Item* item = _begin; item != _end; item = item->next) {
            insert_item(item);
        }
    }

    // Get hash modulo capacity.
    size_t get_hash(const KeyType& key) const {
        return hasher_(key) % capacity_;
    }

    // If size is more than 3/4 of capacity, increases it.
    void resize_if_need() {
        if (size_ * 4 >= capacity_ * 3) {  // if size_/capacity_ >= 3/4
            resize(capacity_ * 2);
        }
    }

    // Increases variable i by 1 modulo capacity.
    size_t cyclic_inc(size_t i) const {
        ++i;
        if (i == capacity_) {
            i = 0;
        }
        return i;
    }

    // Checks if i is between 'from' and 'to' in cyclic array.
    bool is_in_range(size_t i, size_t from, size_t to) {
        if (from <= to) {
            return from <= i && i <= to;
        } else {
            return i <= to || from <= i;
        }
    }

    /* If 'key' is in storage, returns its position.
     * If 'key' is not in storage, returns first free position in storage. */
    size_t find_pos(const KeyType& key) const {
        size_t hash = get_hash(key);

        for (size_t i = hash;; i = cyclic_inc(i)) {
            if (storage[i] == nullptr ||
                storage[i]->keyValue.first == key) {

                return i;
            }
        }
    }

    /* Returns first free position in storage after 'pos'.
     * This function is used for deleting elements from hash map. */
    size_t find_next(size_t pos) {
        for (size_t i = cyclic_inc(pos);; i = cyclic_inc(i)) {
            if (storage[i] == nullptr) {
                return i;
            }

            if (!is_in_range(get_hash(storage[i]->keyValue.first), cyclic_inc(pos), i)) {
                return i;
            }
        }
    }

    // Inserts item in hash map.
    void insert_item(Item* item) {
        size_t pos = find_pos(item->keyValue.first);

        if (storage[pos] == nullptr) {
            storage[pos] = item;
            storage[pos]->pos = pos;
            ++size_;
        }
    }

    // Creates new Item object and changes '_begin' property if need.
    Item* create_item(const KeyType& key, const ValueType& value) {
        Item* item = new Item(key, value, _end->prev, _end);
        if (item->prev == nullptr) {
            _begin = item;
        }
        return item;
    }

    // Deletes item and changes '_begin' if need.
    void delete_item(Item* item) {
        if (item == nullptr) {
            return;
        }

        if (item == _begin) {
            _begin = _begin->next;
        }
        delete item;
    }

    // Deletes all elements from hash map.
    void delete_all() {
        for (size_t i = 0; i < storage.size(); ++i) {
            delete_item(storage[i]);
        }

        storage.assign(1, nullptr);

        _begin = _end;

        capacity_ = 1;
        size_ = 0;
    }
};
