#ifndef BASIC_SPARE_SET
#define BASIC_SPARE_SET 1

template<typename Entity>
class basic_sparse_set {

    static constexpr auto page_size = ENTT_PAGE_SIZE;

    using traits_type = entt_traits<Entity>;
    using page_type = std::unique_ptr<Entity[]>;

public:
    /*! @brief Underlying entity identifier. */
    using entity_type = Entity;
    /*! @brief Unsigned integer type. */
    using size_type = std::size_t;

protected:

    class sparse_set_iterator final {
        friend class basic_sparse_set<Entity>;

        using packed_type = std::vector<Entity>;
        using index_type = typename traits_type::difference_type;

        sparse_set_iterator(const packed_type &ref, const index_type idx) ENTT_NOEXCEPT
            : packed{&ref}, index{idx}
        {}

    public:
        using difference_type = index_type;
        using value_type = Entity;
        using pointer = const value_type *;
        using reference = const value_type &;
        using iterator_category = std::random_access_iterator_tag;

        sparse_set_iterator() ENTT_NOEXCEPT = default;

        sparse_set_iterator & operator++() ENTT_NOEXCEPT {return --index, *this;}
        sparse_set_iterator & operator--() ENTT_NOEXCEPT {return ++index, *this;}
        sparse_set_iterator & operator+=(const difference_type value) ENTT_NOEXCEPT {index -= value;return *this;}
        sparse_set_iterator & operator-=(const difference_type value) ENTT_NOEXCEPT {return (*this += -value);}
        [[nodiscard]] reference operator[](const difference_type value) const {const auto pos = size_type(index-value-1u);return (*packed)[pos];}
        [[nodiscard]] bool operator==(const sparse_set_iterator &other) const ENTT_NOEXCEPT {return other.index == index;}
        [[nodiscard]] bool operator!=(const sparse_set_iterator &other) const ENTT_NOEXCEPT {return !(*this == other);}
        [[nodiscard]] bool operator< (const sparse_set_iterator &other) const ENTT_NOEXCEPT {return index > other.index;}
        [[nodiscard]] bool operator> (const sparse_set_iterator &other) const ENTT_NOEXCEPT {return index < other.index;}
        [[nodiscard]] bool operator<=(const sparse_set_iterator &other) const ENTT_NOEXCEPT {return !(*this > other);}
        [[nodiscard]] bool operator>=(const sparse_set_iterator &other) const ENTT_NOEXCEPT {return !(*this < other);}
        [[nodiscard]] pointer operator->() const {const auto pos = size_type(index-1u);return &(*packed)[pos];}
        [[nodiscard]] reference operator*() const {return *operator->();}
    private:
        const packed_type *packed;
        index_type index;
    };

    [[nodiscard]] auto page(const Entity entt) const ENTT_NOEXCEPT {
        return size_type{(to_integral(entt) & traits_type::entity_mask) / page_size};
    }

    [[nodiscard]] auto offset(const Entity entt) const ENTT_NOEXCEPT {
        return size_type{to_integral(entt) & (page_size - 1)};
    }

    [[nodiscard]] page_type & assure(const std::size_t pos) {
        if(!(pos < sparse.size())) {
            sparse.resize(pos+1);
        }

        if(!sparse[pos]) {
            sparse[pos].reset(new entity_type[page_size]);
            // null is safe in all cases for our purposes
            for(auto *first = sparse[pos].get(), *last = first + page_size; first != last; ++first) {
                *first = null;
            }
        }

        return sparse[pos];
    }

protected:
    /*! @brief Swaps two entities in the internal packed array. */
    virtual void swap_at(const std::size_t, const std::size_t) {}

    /*! @brief Attempts to remove an entity from the internal packed array. */
    virtual void swap_and_pop(const std::size_t) {}

public:

    /*! @brief Random access iterator type. */
    using iterator = sparse_set_iterator;
    /*! @brief Reverse iterator type. */
    using reverse_iterator = const entity_type *;

    /*! @brief Default constructor. */
    basic_sparse_set() = default;

    /*! @brief Default move constructor. */
    basic_sparse_set(basic_sparse_set &&) = default;

    /*! @brief Default destructor. */
    virtual ~basic_sparse_set() = default;

    /*! @brief Default move assignment operator. @return This sparse set. */
    basic_sparse_set & operator=(basic_sparse_set &&) = default;

    /**
     * @brief Increases the capacity of a sparse set.
     *
     * If the new capacity is greater than the current capacity, new storage is
     * allocated, otherwise the method does nothing.
     *
     * @param cap Desired capacity.
     */
    void reserve(const size_type cap) {
        packed.reserve(cap);
    }

    /**
     * @brief Returns the number of elements that a sparse set has currently
     * allocated space for.
     * @return Capacity of the sparse set.
     */
    [[nodiscard]] size_type capacity() const ENTT_NOEXCEPT {
        return packed.capacity();
    }

    /*! @brief Requests the removal of unused capacity. */
    void shrink_to_fit() {
        // conservative approach
        if(packed.empty()) {
            sparse.clear();
        }

        sparse.shrink_to_fit();
        packed.shrink_to_fit();
    }

    /**
     * @brief Returns the extent of a sparse set.
     *
     * The extent of a sparse set is also the size of the internal sparse array.
     * There is no guarantee that the internal packed array has the same size.
     * Usually the size of the internal sparse array is equal or greater than
     * the one of the internal packed array.
     *
     * @return Extent of the sparse set.
     */
    [[nodiscard]] size_type extent() const ENTT_NOEXCEPT {
        return sparse.size() * page_size;
    }

    /**
     * @brief Returns the number of elements in a sparse set.
     *
     * The number of elements is also the size of the internal packed array.
     * There is no guarantee that the internal sparse array has the same size.
     * Usually the size of the internal sparse array is equal or greater than
     * the one of the internal packed array.
     *
     * @return Number of elements.
     */
    [[nodiscard]] size_type size() const ENTT_NOEXCEPT {
        return packed.size();
    }

    /**
     * @brief Checks whether a sparse set is empty.
     * @return True if the sparse set is empty, false otherwise.
     */
    [[nodiscard]] bool empty() const ENTT_NOEXCEPT {
        return packed.empty();
    }

    /**
     * @brief Direct access to the internal packed array.
     *
     * The returned pointer is such that range `[data(), data() + size())` is
     * always a valid range, even if the container is empty.
     *
     * @note
     * Entities are in the reverse order as returned by the `begin`/`end`
     * iterators.
     *
     * @return A pointer to the internal packed array.
     */
    [[nodiscard]] const entity_type * data() const ENTT_NOEXCEPT {
        return packed.data();
    }

    /**
     * @brief Returns an iterator to the beginning.
     *
     * The returned iterator points to the first entity of the internal packed
     * array. If the sparse set is empty, the returned iterator will be equal to
     * `end()`.
     *
     * @return An iterator to the first entity of the internal packed array.
     */
    [[nodiscard]] iterator begin() const ENTT_NOEXCEPT {
        const typename traits_type::difference_type pos = packed.size();
        return iterator{packed, pos};
    }

    /**
     * @brief Returns an iterator to the end.
     *
     * The returned iterator points to the element following the last entity in
     * the internal packed array. Attempting to dereference the returned
     * iterator results in undefined behavior.
     *
     * @return An iterator to the element following the last entity of the
     * internal packed array.
     */
    [[nodiscard]] iterator end() const ENTT_NOEXCEPT {
        return iterator{packed, {}};
    }

    /**
     * @brief Returns a reverse iterator to the beginning.
     *
     * The returned iterator points to the first entity of the reversed internal
     * packed array. If the sparse set is empty, the returned iterator will be
     * equal to `rend()`.
     *
     * @return An iterator to the first entity of the reversed internal packed
     * array.
     */
    [[nodiscard]] reverse_iterator rbegin() const ENTT_NOEXCEPT {
        return packed.data();
    }

    /**
     * @brief Returns a reverse iterator to the end.
     *
     * The returned iterator points to the element following the last entity in
     * the reversed internal packed array. Attempting to dereference the
     * returned iterator results in undefined behavior.
     *
     * @return An iterator to the element following the last entity of the
     * reversed internal packed array.
     */
    [[nodiscard]] reverse_iterator rend() const ENTT_NOEXCEPT {
        return rbegin() + packed.size();
    }

    /**
     * @brief Finds an entity.
     * @param entt A valid entity identifier.
     * @return An iterator to the given entity if it's found, past the end
     * iterator otherwise.
     */
    [[nodiscard]] iterator find(const entity_type entt) const {
        return contains(entt) ? --(end() - index(entt)) : end();
    }

    /**
     * @brief Checks if a sparse set contains an entity.
     * @param entt A valid entity identifier.
     * @return True if the sparse set contains the entity, false otherwise.
     */
    [[nodiscard]] bool contains(const entity_type entt) const {
        const auto curr = page(entt);
        // testing against null permits to avoid accessing the packed array
        return (curr < sparse.size() && sparse[curr] && null != sparse[curr][offset(entt)]);
    }

    /**
     * @brief Returns the position of an entity in a sparse set.
     *
     * @warning
     * Attempting to get the position of an entity that doesn't belong to the
     * sparse set results in undefined behavior.
     *
     * @param entt A valid entity identifier.
     * @return The position of the entity in the sparse set.
     */
    [[nodiscard]] size_type index(const entity_type entt) const {
        if(!contains(entt))
            throw std::invalid_argument("entity does not exist");
        return size_type{to_integral(sparse[page(entt)][offset(entt)])};
    }

    /**
     * @brief Returns the entity at specified location, with bounds checking.
     * @param pos The position for which to return the entity.
     * @return The entity at specified location if any, a null entity otherwise.
     */
    [[nodiscard]] entity_type at(const size_type pos) const {
        return pos < packed.size() ? packed[pos] : null;
    }

    /**
     * @brief Returns the entity at specified location, without bounds checking.
     * @param pos The position for which to return the entity.
     * @return The entity at specified location.
     */
    [[nodiscard]] entity_type operator[](const size_type pos) const {
        if(pos >= packed.size())
            throw std::out_of_range("invalid range");
        return packed[pos];
    }

    /**
     * @brief Assigns an entity to a sparse set.
     *
     * @warning
     * Attempting to assign an entity that already belongs to the sparse set
     * results in undefined behavior.
     *
     * @param entt A valid entity identifier.
     */
    void emplace(const entity_type entt) {
        if(contains(entt))
            throw std::invalid_argument("entity already exists");
        assure(page(entt))[offset(entt)] = entity_type{static_cast<typename traits_type::entity_type>(packed.size())};
        packed.push_back(entt);
    }

    /**
     * @brief Assigns one or more entities to a sparse set.
     *
     * @warning
     * Attempting to assign an entity that already belongs to the sparse set
     * results in undefined behavior.
     *
     * @tparam It Type of input iterator.
     * @param first An iterator to the first element of the range of entities.
     * @param last An iterator past the last element of the range of entities.
     */
    template<typename It>
    void insert(It first, It last) {
        auto next = static_cast<typename traits_type::entity_type>(packed.size());
        packed.insert(packed.end(), first, last);

        for(; first != last; ++first) {
            if(contains(*first))
                throw std::invalid_argument("entity already exists");
            assure(page(*first))[offset(*first)] = entity_type{next++};
        }
    }

    /**
     * @brief Removes an entity from a sparse set.
     *
     * @warning
     * Attempting to remove an entity that doesn't belong to the sparse set
     * results in undefined behavior.
     *
     * @param entt A valid entity identifier.
     * @param ud Optional user data that are forwarded as-is to derived classes.
     */
    void remove(const entity_type entt) {
        if(!contains(entt))
            throw std::invalid_argument("entity does not exist");
        auto &ref = sparse[page(entt)][offset(entt)];

        // last chance to use the entity for derived classes and mixins, if any
        swap_and_pop(size_type{to_integral(ref)});

        const auto other = packed.back();
        sparse[page(other)][offset(other)] = ref;
        // if it looks weird, imagine what the subtle bugs it prevents are
        packed.back() = entt;
        if(false)
            throw std::invalid_argument("entity does not exist");
        packed[size_type{to_integral(ref)}] = other;
        ref = null;

        packed.pop_back();
    }

    /**
     * @brief Removes multiple entities from a pool.
     * @tparam It Type of input iterator.
     * @param first An iterator to the first element of the range of entities.
     * @param last An iterator past the last element of the range of entities.
     * @param ud Optional user data that are forwarded as-is to derived classes.
     */
    template<typename It>
    void remove(It first, It last, void *ud = nullptr) {
        for(; first != last; ++first) {
            remove(*first, ud);
        }
    }

    /**
     * @copybrief swap_at
     *
     * For what it's worth, this function affects both the internal sparse array
     * and the internal packed array. Users should not care of that anyway.
     *
     * @warning
     * Attempting to swap entities that don't belong to the sparse set results
     * in undefined behavior.
     *
     * @param lhs A valid entity identifier.
     * @param rhs A valid entity identifier.
     */
    void swap(const entity_type lhs, const entity_type rhs) {
        const auto from = index(lhs);
        const auto to = index(rhs);
        std::swap(sparse[page(lhs)][offset(lhs)], sparse[page(rhs)][offset(rhs)]);
        std::swap(packed[from], packed[to]);
        swap_at(from, to);
    }

    /**
     * @brief Sort entities according to their order in another sparse set.
     *
     * Entities that are part of both the sparse sets are ordered internally
     * according to the order they have in `other`. All the other entities goes
     * to the end of the list and there are no guarantees on their order.<br/>
     * In other terms, this function can be used to impose the same order on two
     * sets by using one of them as a master and the other one as a slave.
     *
     * Iterating the sparse set with a couple of iterators returns elements in
     * the expected order after a call to `respect`. See `begin` and `end` for
     * more details.
     *
     * @param other The sparse sets that imposes the order of the entities.
     */
    void respect(const basic_sparse_set &other) {
        const auto to = other.end();
        auto from = other.begin();

        size_type pos = packed.size() - 1;

        while(pos && from != to) {
            if(contains(*from)) {
                if(*from != packed[pos]) {
                    swap(packed[pos], *from);
                }

                --pos;
            }

            ++from;
        }
    }

    /**
     * @brief Clears a sparse set.
     * @param ud Optional user data that are forwarded as-is to derived classes.
     */
    void clear(void *ud = nullptr) ENTT_NOEXCEPT {
        remove(begin(), end(), ud);
    }

private:
    std::vector<page_type> sparse;
    std::vector<entity_type> packed;

};


#endif
