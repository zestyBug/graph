#ifndef BASIC_REGISTER
#define BASIC_REGISTER 1



#include "type_utils.hpp"


/**
 * @brief Fast and reliable entity-component system.
 *
 * The registry is the core class of the entity-component framework.<br/>
 * It stores entities and arranges pools of components on a per request basis.
 * By means of a registry, users can manage entities and components, then create
 * views or groups to iterate them.
 *
 * @tparam Entity A valid entity type (see entt_traits for more details).
 */
template<typename Entity,typename Component>
class basic_registry
{
    static_assert(sizeof(Component) > 0, "Exclusion-only views are not supported");
    static_assert(!std::is_const_v<Component>, "Invalid const type");


    using traits_type = entt_traits<Entity>;
    using storage_type = storage_traits_type<Entity, Component>;

public:

    /*! @brief Underlying entity identifier. */
    using entity_type = Entity;
    using version_type = typename traits_type::version_type;
    /*! @brief Underlying entity identifier. */
    using value_type = Component;
    /*! @brief Unsigned integer type. */
    using size_type = std::size_t;

private:

    [[nodiscard]]
    const storage_type & pool() const
    {
        return pool_pool;
    }

    [[nodiscard]]
    storage_type & pool()
    {
        return pool_pool;
    }

    entity_type generate_identifier()
    {
        // traits_type::entity_mask is reserved to allow for null identifiers
        if(static_cast<typename traits_type::entity_type>(entities.size()) >= traits_type::entity_mask)
            throw std::runtime_error("run out of entity number");
        return entities.emplace_back(entity_type{static_cast<typename traits_type::entity_type>(entities.size())});
    }

    entity_type recycle_identifier()
    {
        const auto curr = to_integral(available);
        const auto version = to_integral(entities[curr]) & (traits_type::version_mask << traits_type::entity_shift);
        available = entity_type{to_integral(entities[curr]) & traits_type::entity_mask};
        return entities[curr] = entity_type{curr | version};
    }

    void release_entity(const Entity entity)
    {
        const auto entt = to_integral(entity) & traits_type::entity_mask;
        entities[entt] = entity_type{to_integral(available) | (typename traits_type::entity_type{static_cast<typename traits_type::version_type>(version(entity) + 1u)} << traits_type::entity_shift)};
        available = entity_type{entt};
    }

public:

    /**
     * @brief Returns the entity identifier without the version.
     * @param entity An entity identifier, either valid or not.
     * @return The entity identifier without the version.
     */
    [[nodiscard]] static entity_type entity(const entity_type entity) ENTT_NOEXCEPT {
        return entity_type{to_integral(entity) & traits_type::entity_mask};
    }

    /**
     * @brief Returns the version stored along with an entity identifier.
     * @param entity An entity identifier, either valid or not.
     * @return The version stored along with the given entity identifier.
     */
    [[nodiscard]] static version_type version(const entity_type entity) ENTT_NOEXCEPT {
        return version_type(to_integral(entity) >> traits_type::entity_shift);
    }

    /*! @brief Default constructor. */
    basic_registry():pool_pool(){
    };

    /*! @brief Default move constructor. */
    basic_registry(const basic_registry &) = delete;

    /*! @brief Default move constructor. */
    basic_registry(basic_registry &&) = default;

    /*! @brief Default move assignment operator. @return This registry. */
    basic_registry & operator=(basic_registry &&) = default;


    [[nodiscard]] size_type pool_size() const {
        return pool().size();
    }

    /**
     * @brief Returns the number of entities created so far.
     * @return Number of entities created so far.
     */
    [[nodiscard]] size_t size() const ENTT_NOEXCEPT {
        return entities.size();
    }

    /**
     * @brief Returns the number of entities that a pool has currently
     * allocated space for.
     * @return Capacity of the pool.
     */
    [[nodiscard]] size_type pool_capacity() const {
        return pool().size();
    }

    /**
     * @brief Returns the number of entities that a registry has currently
     * allocated space for.
     * @return Capacity of the registry.
     */
    [[nodiscard]] size_type capacity() const ENTT_NOEXCEPT {
        return entities.capacity();
    }

    /**
     * @brief Creates a new entity and returns it.
     *
     * There are two kinds of possible entity identifiers:
     *
     * * Newly created ones in case no entities have been previously destroyed.
     * * Recycled ones with updated versions.
     *
     * @return A valid entity identifier.
     */
    entity_type create()
        {return null == available ? generate_identifier() : recycle_identifier();}

    /**
     * @brief Assigns the given component to an entity.
     *
     * A new instance of the given component is created and initialized with the
     * arguments provided (the component must have a proper constructor or be of
     * aggregate type). Then the component is assigned to the given entity.
     *
     * @warning
     * Attempting to use an invalid entity or to assign a component to an entity
     * that already owns it results in undefined behavior.
     *
     * @tparam Args Types of arguments to use to construct the component.
     * @param entity A valid entity identifier.
     * @param args Parameters to use to initialize the component.
     * @return A reference to the newly created component.
     */
    template<typename... Args>
    decltype(auto) emplace(const entity_type entity, Args &&... args)
    {
        if(!valid(entity))
            throw std::invalid_argument("invalid entity");
        return pool().emplace(entity, std::forward<Args>(args)...);
    }


    /**
     * @brief Destroys an entity.
     *
     * When an entity is destroyed, its version is updated and the identifier
     * can be recycled at any time.
     *
     * @sa remove_all
     *
     * @param entity A valid entity identifier.
     */
    void destroy(const entity_type entity) {
        remove(entity);
        release_entity(entity);
    }

    /**
     * @brief Checks if an entity identifier refers to a valid entity.
     * @param entity An entity identifier, either valid or not.
     * @return True if the identifier is valid, false otherwise.
     */
    [[nodiscard]]
    bool valid(const entity_type entity) const
    {
        const auto pos = size_type(to_integral(entity) & traits_type::entity_mask);
        return (pos < entities.size() && entities[pos] == entity);
    }

    /**
     * @brief Removes component from an entity.
     */
    void remove(const entity_type entity) {
        if(!valid(entity))
            throw std::invalid_argument("invalid entity");
        if(pool_pool.contains(entity))
            pool_pool.remove(entity);
    }

    /**
     * @brief Checks if an entity has alocated component.
     * @param entt A valid entity identifier.
     * @return True if entity has component, false otherwise.
     */
    [[nodiscard]] bool contains(const entity_type entt) const {
        return pool_pool.contains(entt);
    }

    /**
     * @brief Returns references to the given components for an entity.
     *
     * @warning
     * Attempting to use an invalid entity or to get a component from an entity
     * that doesn't own it results in undefined behavior.
     *
     * @param entity A valid entity identifier.
     * @return References to the components owned by the entity.
     */
    [[nodiscard]] decltype(auto) get([[maybe_unused]] const entity_type entity) const {
        if(!valid(entity))
            throw std::invalid_argument("invalid entity");
        return pool().get(entity);
    }

    /*! @copydoc get */
    [[nodiscard]] decltype(auto) get([[maybe_unused]] const entity_type entity) {
        if(!valid(entity))
            throw std::invalid_argument("invalid entity");
        return const_cast<Component &>(pool().get(entity));
    }


    /**
     * @brief Returns a view.
     *
     * This kind of objects are created on the fly and share with the registry
     * its internal data structures.<br/>
     * Feel free to discard a view after the use. Creating and destroying a view
     * is an incredibly cheap operation because they do not require any type of
     * initialization.<br/>
     * As a rule of thumb, storing a view should never be an option.
     *
     * Views do their best to iterate the smallest set of candidate entities.
     * In particular:
     *
     * * Single component views are incredibly fast and iterate a packed array
     *   of entities, all of which has the given component.
     * * Multi component views look at the number of entities available for each
     *   component and pick up a reference to the smallest set of candidates to
     *   test for the given components.
     *
     * Views in no way affect the functionalities of the registry nor those of
     * the underlying pools.
     *
     * @note
     * Multi component views are pretty fast. However their performance tend to
     * degenerate when the number of components to iterate grows up and the most
     * of the entities have all the given components.<br/>
     * To get a performance boost, consider using a group instead.
     *
     * @return A newly created view.
     */
    [[nodiscard]] const basic_view<Entity,Component>
    view() const 
    {
        return { pool() };
    }

    [[nodiscard]] basic_view<Entity,Component>
    view() 
    {
        return { pool() };
    }

private:
    mutable storage_type pool_pool;
    std::vector<entity_type> entities{};
    entity_type available{null};
};

#endif
