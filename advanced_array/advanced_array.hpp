#ifndef ENTT_HPP
#define ENTT_HPP 1

#include <cstddef>
#include <cstdint>
#include <utility>
#include <type_traits>
#include <tuple>
#include <vector>
#include <list>
#include <memory>
#include <functional>
#include <stdexcept>



#ifdef ENTT_PAGE_SIZE
static_assert(ENTT_PAGE_SIZE && ((ENTT_PAGE_SIZE & (ENTT_PAGE_SIZE - 1)) == 0), "ENTT_PAGE_SIZE must be a power of two");
#else
#   define ENTT_PAGE_SIZE 4096
#endif

#ifndef ENTT_NOEXCEPT
#   define ENTT_NOEXCEPT noexcept
#endif

namespace advanced_array {

/*! @brief Alias declaration for type identifiers. */
using id_type = std::uint32_t;


#include "basic_utils.hpp"

#include "type_utils.hpp"


/// @brief Compile-time constant for null entities.
inline constexpr null_t null{};

template<std::size_t Len, std::size_t = alignof(typename std::aligned_storage_t<Len + !Len>)>
class basic_any;

#include "basic_any.hpp"

#include "basic_sparse_set.hpp"

template<typename, typename>
class basic_storage;

#include "basic_storage.hpp"

template<typename,typename>
class basic_registry;

/**
 * @brief Defines the component-to-storage conversion.
 * Mixin type to use to add signal support to storage types.
 * it is a shortcut to storage_traits<~>::storage_type
 *
 * Formally:
 *
 * * If the component type is a non-const one, the member typedef type is the
 *   declared storage type.
 * * If the component type is a const one, the member typedef type is the
 *   declared storage type, except it has a const-qualifier added.
 *
 * @tparam Entity A valid entity type (see entt_traits for more details).
 * @tparam Type Type of objects assigned to the entities.
 */
template<typename Entity, typename Type>
struct storage_traits_type final: public basic_storage<Entity, Type>
{
    /* The type of the underlying storage. */
    using SType = basic_storage<Entity, Type>;
public:
    /*! @brief Underlying value type. */
    using value_type = typename SType::value_type;
    /*! @brief Underlying entity identifier. */
    using entity_type = typename SType::entity_type;

    template<typename... Args>
    auto & emplace(const entity_type entity, Args &&... args) {
        SType::emplace(entity, std::forward<Args>(args)...);
        return this->get(entity);
    }
};

/**
 * @brief View.
 *
 * Primary template isn't defined on purpose. All the specializations give a
 * compile-time error, but for a few reasonable cases.
 */
template<typename,typename>
class basic_view;

#include "basic_view.hpp"

#include "basic_register.hpp"

/*! @brief Alias declaration for the most common use case. */
using sparse_set = basic_sparse_set<id_type>;

/**
 * @brief Alias declaration for the most common use case.
 * @tparam Args Other template parameters.
 */
template<typename... Args>
using storage = basic_storage<id_type, Args...>;

/*! @brief Alias declaration for the most common use case. */
template<typename Component>
using registry = basic_registry<id_type,Component>;

/*! @brief Default entity identifier. */
using entity = id_type;

}

#endif
