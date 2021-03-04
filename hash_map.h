#include <algorithm>
#include <cstddef>
#include <functional>
#include <initializer_list>
#include <stdexcept>
#include <vector>

template<class KeyType, class ValueType, class Hash = std::hash<KeyType> >
class HashMap {
private:
    size_t sz;  // size
    size_t cp;  // capacity
    Hash hasher;

    class Item {  // linked list for elements
    private:
        void connect() {
            if (next != nullptr) {
                next->prev = this;
            }

            if (prev != nullptr) {
                prev->next = this;
            }
        }

        void disconnect() {
            if (next != nullptr) {
                next->prev = prev;
            }

            if (prev != nullptr) {
                prev->next = next;
            }
        }

    public:
        std::pair<const KeyType, ValueType> keyValue;
        Item *prev;
        Item *next;
        size_t pos;

        Item(const KeyType &key, const ValueType &value, Item *prev, Item *next) :
                keyValue({key, value}),
                prev(prev),
                next(next) {

            connect();
        }

        Item(Item *prev, Item *next) :
                prev(prev),
                next(next) {

            connect();
        }

        ~Item() {
            disconnect();
        }
    };

    Item *_begin;  // first element in linked list
    Item *_end;  // last element in linked list

    std::vector<Item *> storage;

    void init() {
        sz = 0;
        cp = 1;

        storage.assign(1, nullptr);

        _end = new Item(nullptr, nullptr);
        _begin = _end;
    }

    void resize(size_t newCapacity) {
        storage.assign(newCapacity, nullptr);

        cp = newCapacity;
        sz = 0;

        for (Item *item = _begin; item != _end; item = item->next) {
            insert_item(item);
        }
    }

    size_t get_hash(const KeyType &key) const {
        return hasher(key) % cp;
    }

    void resize_if_need() {
        if (sz * 4 >= cp * 3) {  // if sz/cp >= 3/4
            resize(cp * 2);
        }
    }

    size_t cyclic_inc(size_t i) const {
        ++i;
        if (i == cp) {
            i = 0;
        }
        return i;
    }

    bool is_in_range(size_t i, size_t from, size_t to) {
        if (from <= to) {
            return from <= i && i <= to;
        } else {
            return i <= to || from <= i;
        }
    }

    size_t find_pos(const KeyType &key) const {
        size_t hash = get_hash(key);

        for (size_t i = hash;; i = cyclic_inc(i)) {
            if (storage[i] == nullptr ||
                storage[i]->keyValue.first == key) {

                return i;
            }
        }
    }

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

    void insert_item(Item *item) {
        size_t pos = find_pos(item->keyValue.first);

        if (storage[pos] == nullptr) {
            storage[pos] = item;
            storage[pos]->pos = pos;
            ++sz;
        }
    }

    Item *create_item(const KeyType &key, const ValueType &value) {
        Item *item = new Item(key, value, _end->prev, _end);
        if (item->prev == nullptr) {
            _begin = item;
        }
        return item;
    }

    void delete_item(Item *item) {
        if (item == nullptr) {
            return;
        }

        if (item == _begin) {
            _begin = _begin->next;
        }
        delete item;
    }

    void delete_all() {
        for (size_t i = 0; i < storage.size(); ++i) {
            delete_item(storage[i]);
        }

        storage.assign(1, nullptr);

        _begin = _end;

        cp = 1;
        sz = 0;
    }

public:
    HashMap(const Hash &hasher = Hash()) : hasher(hasher) {
        init();
    }

    template<typename Iter>
    HashMap(Iter first, Iter last, const Hash &hasher = Hash()) : hasher(hasher) {
        init();

        for (auto it = first; it != last; ++it) {
            insert(*it);
        }
    }

    HashMap(std::initializer_list<std::pair<KeyType, ValueType> > initList,
            const Hash &hasher = Hash()) : hasher(hasher) {

        init();

        for (const auto &keyValue : initList) {
            insert(keyValue);
        }
    }

    HashMap(const HashMap &other, const Hash &hasher = Hash()) : hasher(hasher) {
        init();

        for (auto it = other.begin(); it != other.end(); ++it) {
            insert(*it);
        }
    }

    HashMap &operator=(const HashMap &other) {
        if (&other != this) {
            delete_all();
            for (auto it = other.begin(); it != other.end(); ++it) {
                insert(*it);
            }
        }
        return *this;
    }

    size_t size() const {
        return sz;
    }

    bool empty() const {
        return sz == 0;
    }

    Hash hash_function() const {
        return hasher;
    }

    void insert(const std::pair<KeyType, ValueType> &keyValue) {
        const KeyType &key = keyValue.first;
        const ValueType &value = keyValue.second;

        size_t pos = find_pos(key);

        if (storage[pos] == nullptr) {
            storage[pos] = create_item(key, value);
            storage[pos]->pos = pos;
            ++sz;

            resize_if_need();
        }
    }

    void erase(const KeyType &key) {
        size_t pos = find_pos(key);

        if (storage[pos] != nullptr) {
            delete_item(storage[pos]);
            storage[pos] = nullptr;
            --sz;

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

    class iterator {
    private:
        Item *item;

    public:
        iterator() = default;

        explicit iterator(Item *item) : item(item) {
        }

        std::pair<const KeyType, ValueType> *operator->() {
            return &item->keyValue;
        }

        std::pair<const KeyType, ValueType> &operator*() {
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

        bool operator==(const iterator &other) {
            return item == other.item;
        }

        bool operator!=(const iterator &other) {
            return item != other.item;
        }
    };

    class const_iterator {
    private:
        Item *item;

    public:
        const_iterator() = default;

        explicit const_iterator(Item *item) : item(item) {
        }

        const std::pair<const KeyType, ValueType> *operator->() {
            return &item->keyValue;
        }

        const std::pair<const KeyType, ValueType> &operator*() {
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

        bool operator==(const const_iterator &other) {
            return item == other.item;
        }

        bool operator!=(const const_iterator &other) {
            return item != other.item;
        }
    };

    iterator begin() {
        return iterator(_begin);
    }

    iterator end() {
        return iterator(_end);
    }

    const_iterator begin() const {
        return const_iterator(_begin);
    }

    const_iterator end() const {
        return const_iterator(_end);
    }

    iterator find(const KeyType &key) {
        size_t pos = find_pos(key);

        if (storage[pos] == nullptr) {
            return iterator(_end);
        }

        return iterator(storage[pos]);
    }

    const_iterator find(const KeyType &key) const {
        size_t pos = find_pos(key);

        if (storage[pos] == nullptr) {
            return const_iterator(_end);
        }

        return const_iterator(storage[pos]);
    }

    ValueType &operator[](const KeyType &key) {
        size_t pos = find_pos(key);

        if (storage[pos] == nullptr) {
            storage[pos] = create_item(key, ValueType());
            storage[pos]->pos = pos;
            ++sz;

            resize_if_need();

            pos = find_pos(key);
        }

        return storage[pos]->keyValue.second;
    }

    const ValueType &at(const KeyType &key) const {
        size_t pos = find_pos(key);

        if (storage[pos] == nullptr) {
            throw std::out_of_range("No such key in the hash table");
        }

        return storage[pos]->keyValue.second;
    }

    void clear() {
        Item *item = _begin;
        while (item != _end) {
            Item *next = item->next;
            storage[item->pos] = nullptr;
            delete_item(item);
            item = next;
        }
        sz = 0;
    }

    ~HashMap() {
        delete_all();
        delete _end;
    }
};
