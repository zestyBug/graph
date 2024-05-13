/**
 * contain staructures that are needed for 'type_id(T)'
 * ,seems like gcc doesnt support type_id(T) whene -fno-rtti
 */

#ifndef TYPE_UTILS
#define TYPE_UTILS 1





#include "basic_utils.hpp"



/**
 * @brief Entity traits.
 *
 * Primary template isn't defined on purpose. All the specializations give a
 * compile-time error unless the template parameter is an accepted entity type.
 */
template<typename>
struct entt_traits;


/**
 * @brief Entity traits for a 16 bits entity identifier.
 *
 * A 16 bits entity identifier
 */
template<>
struct entt_traits<std::uint16_t> {
    /*! @brief Underlying entity type. */
    using entity_type = std::uint16_t;

    /*! @brief Mask to use to get the entity number out of an identifier. */
    static constexpr entity_type entity_mask = 0x7FFF;
};

/**
 * @brief Entity traits for a 32 bits entity identifier.
 *
 * A 32 bits entity identifier
 */
template<>
struct entt_traits<std::uint32_t> {
    /*! @brief Underlying entity type. */
    using entity_type = std::uint32_t;
    /*! @brief Underlying version type. */
    using version_type = std::uint16_t;
    /*! @brief Difference type. */
    using difference_type = std::int64_t;

    /*! @brief Mask to use to get the entity number out of an identifier. */
    static constexpr entity_type entity_mask = 0xFFFFF;
    /*! @brief Mask to use to get the version out of an identifier. */
    static constexpr entity_type version_mask = 0xFFF;
    /*! @brief Extent of the entity number within an identifier. */
    static constexpr std::size_t entity_shift = 20u;
};

/**
 * @brief Entity traits for a 64 bits entity identifier.
 *
 * A 64 bits entity identifier guarantees:
 *
 * * 32 bits for the entity number (an indecently large number).
 * * 32 bit for the version (an indecently large number).
 */
template<>
struct entt_traits<std::uint64_t> {
    /*! @brief Underlying entity type. */
    using entity_type = std::uint64_t;
    /*! @brief Underlying version type. */
    using version_type = std::uint32_t;
    /*! @brief Difference type. */
    using difference_type = std::int64_t;

    /*! @brief Mask to use to get the entity number out of an identifier. */
    static constexpr entity_type entity_mask = 0xFFFFFFFF;
    /*! @brief Mask to use to get the version out of an identifier. */
    static constexpr entity_type version_mask = 0xFFFFFFFF;
    /*! @brief Extent of the entity number within an identifier. */
    static constexpr std::size_t entity_shift = 32u;
};

/**
 * @brief Converts an entity type to its underlying type.
 * @tparam Entity The value type.
 * @param entity The value to convert.
 * @return The integral representation of the given value.
 */
template<typename Entity>
[[nodiscard]] constexpr
auto to_integral(const Entity entity) noexcept {
    return static_cast<typename entt_traits<Entity>::entity_type>(entity);
}


/*! @brief Null object for all entity identifiers.  */
struct null_t
{
    /** @brief Converts the null object to identifiers of any type.*/
    template<typename Entity>
    [[nodiscard]] constexpr operator Entity() const noexcept { return Entity{entt_traits<Entity>::entity_mask}; }

    [[nodiscard]] constexpr bool operator==(const null_t &) const noexcept {return true;}
    [[nodiscard]] constexpr bool operator!=(const null_t &) const noexcept {return false;}
    template<typename Entity>
    [[nodiscard]] constexpr bool operator!=(const Entity &entity) const noexcept { return !(*this == entity); }
    template<typename Entity>
    [[nodiscard]] constexpr bool operator==(const Entity &entity) const noexcept {
        return to_integral(entity) == to_integral(static_cast<Entity>(*this));
    }
};



#endif
