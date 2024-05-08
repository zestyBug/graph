/**
 * contain independent simple classes that are dependecy of other classes
 */

#ifndef BASIC_UTILS
#define BASIC_UTILS 1





/**
 * @brief A class to use to push around lists of constant values, nothing more.
 * @tparam Value Values provided by the value list.
 */
template<auto... Value>
struct value_list {
    /*! @brief Value list type. */
    using type = value_list;
    /*! @brief Compile-time number of elements in the value list. */
    static constexpr auto size = sizeof...(Value);
};
/**
 * @brief A class to use to push around lists of types, nothing more.
 * @tparam Type Types provided by the type list.
 */
template<typename... Type>
struct type_list {
    /*! @brief Type list type. */
    using type = type_list;
    /*! @brief Compile-time number of elements in the type list. */
    static constexpr auto size = sizeof...(Type);
};



/*! @brief Primary template isn't defined on purpose. */
template<std::size_t, typename>
struct value_list_element;

/**
 * @brief Provides compile-time indexed access to the values of a value list.
 * @tparam Index Index of the value to return.
 * @tparam Value First value provided by the value list.
 * @tparam Other Other values provided by the value list.
 */
template<std::size_t Index, auto Value, auto... Other>
struct value_list_element<Index, value_list<Value, Other...>>
    : value_list_element<Index - 1u, value_list<Other...>>
{};

/**
 * @brief Provides compile-time indexed access to the types of a type list.
 * @tparam Value First value provided by the value list.
 * @tparam Other Other values provided by the value list.
 */
template<auto Value, auto... Other>
struct value_list_element<0u, value_list<Value, Other...>> {
    /*! @brief Searched value. */
    static constexpr auto value = Value;
};
template<std::size_t Index, typename List>
inline constexpr auto value_list_element_v = value_list_element<Index, List>::value;








/**
 * @brief Transcribes the constness of a type to another type.
 * @tparam To The type to which to transcribe the constness.
 * @tparam From The type from which to transcribe the constness.
 */
template<typename To, typename From>
struct constness_as {
    /*! @brief The type resulting from the transcription of the constness. */
    using type = std::remove_const_t<To>;
};
/*! @copydoc constness_as */
template<typename To, typename From>
struct constness_as<To, const From> {
    /*! @brief The type resulting from the transcription of the constness. */
    using type = std::add_const_t<To>;
};
/**
 * @brief Alias template to facilitate the transcription of the constness.
 * @tparam To The type to which to transcribe the constness.
 * @tparam From The type from which to transcribe the constness.
 */
template<typename To, typename From>
using constness_as_t = typename constness_as<To, From>::type;





template<std::size_t N>
struct choice_t : choice_t<N-1>
{};

template<>
struct choice_t<0> {};

template<std::size_t N>
inline constexpr choice_t<N> choice{};






namespace internal {
    template<typename>
    [[nodiscard]] constexpr bool is_equality_comparable(...) { return false; }

    template<typename Type>
    [[nodiscard]] constexpr auto is_equality_comparable(choice_t<0>)
    -> decltype(std::declval<Type>() == std::declval<Type>()) { return true; }

    template<typename Type>
    [[nodiscard]] constexpr auto is_equality_comparable(choice_t<1>)
    -> decltype(std::declval<typename Type::value_type>(), std::declval<Type>() == std::declval<Type>()) {
        return is_equality_comparable<typename Type::value_type>(choice<2>);
    }

    template<typename Type>
    [[nodiscard]] constexpr auto is_equality_comparable(choice_t<2>)
    -> decltype(std::declval<typename Type::mapped_type>(), std::declval<Type>() == std::declval<Type>()) {
        return is_equality_comparable<typename Type::key_type>(choice<2>) && is_equality_comparable<typename Type::mapped_type>(choice<2>);
    }
}

/**
 * @brief Provides the member constant `value` to true if a given type is
 * equality comparable, false otherwise.
 * @tparam Type Potentially equality comparable type.
 */
template<typename Type, typename = void>
struct is_equality_comparable: std::bool_constant<internal::is_equality_comparable<Type>(choice<2>)> {};

template<class Type>
inline constexpr auto is_equality_comparable_v = is_equality_comparable<Type>::value;

#endif
