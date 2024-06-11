/**
 * Switch/Case strings using constexpr Fnv1-a.
 * @comment References: <https://tools.ietf.org/html/draft-eastlake-fnv-16>
 * @comment References: <https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function>
 * @maintainer xavier dot roche at algolia.com
 */

#pragma once

#include <cstdint>
#include <cstring>
#include <string>
#include <type_traits>

// Traits for FNV1a
template<size_t Bits>
struct fnv1a_traits
{
    static constexpr bool Supported = false;
};

// Traits for 32-bit FNV1a
template<>
struct fnv1a_traits<32>
{
    static constexpr bool Supported = true;
    using Type = uint32_t;

    static constexpr Type Prime = 0x1000193;
    static constexpr Type Offset = 0x811c9dc5;
};

// Traits for 64-bit FNV1a
template<>
struct fnv1a_traits<64>
{
    static constexpr bool Supported = true;
    using Type = uint64_t;

    static constexpr Type Prime = 0x100000001b3;
    static constexpr Type Offset = 0xcbf29ce484222325;
};

static constexpr __uint128_t Pack128(uint64_t high, uint64_t low)
{
    return ((__uint128_t)high << 64) + (__uint128_t)low;
}

// Traits for 128-bit FNV1a
template<>
struct fnv1a_traits<128>
{
    static constexpr bool Supported = true;
    using Type = __uint128_t;

    static constexpr Type Prime = Pack128(0x1000000, 0x000000000000013b);
    static constexpr Type Offset = Pack128(0x6c62272e07bb0142, 0x62b821756295c58d);
};

// Generic FNV1a implementation
template<size_t Bits>
struct fnv1a
{
    static_assert(fnv1a_traits<Bits>::Supported);
    using Type = typename fnv1a_traits<Bits>::Type;

    /**
     * Compute the Fowler–Noll–Vo hash
     * @comment stop An optional stop character
     * @param s The string
     * @param l The string size
     * @return The fnv-1a hash
     */
    template<typename T, uint8_t stop = 0, typename L = decltype(nullptr)>
    static constexpr Type hash_container(T s,
                                         const std::size_t l,
                                         L stopLen = nullptr,
                                         Type hash = fnv1a_traits<Bits>::Offset)
    {
        // See <https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function>
        std::size_t j = 0;
        for (; j < l; j++) {
            const uint8_t byte = s[j];
            if constexpr (stop != 0) {
                if (byte == stop) {
                    if constexpr (!std::is_same<L, decltype(nullptr)>::value) {
                        *stopLen = j + 1;
                    }
                    break;
                }
            }
            hash ^= byte;
            hash *= fnv1a_traits<Bits>::Prime;
        }

        return hash;
    }

    /**
     * Compute the Fowler–Noll–Vo hash
     * @comment stop An optional stop character
     * @param s The string
     * @param l The string size
     * @return The fnv-1a hash
     */
    template<typename C, C stop = 0, typename L = decltype(nullptr)>
    static constexpr Type hash(const C* s,
                               const std::size_t l,
                               L stopLen = nullptr,
                               Type hash = fnv1a_traits<Bits>::Offset)
    {
        // Accept [ unsigned | signed ] char
        static_assert(std::is_integral<C>::value);
        static_assert(sizeof(C) == 1);
        return hash_container<const C*, stop, L>(s, l, stopLen, hash);
    }

    /**
     * Compute the Fowler–Noll–Vo hash
     * @param s The string
     * @return The fnv-1a hash
     */
    template<const std::size_t l>
    static constexpr Type hash(const char (&s)[l])
    {
        return hash(&s[0], l - 1);
    }

    // Do not infer length for char arrays
    template<const std::size_t l>
    static constexpr Type hash(char (&s)[l])
    {
        return hash(&s[0]);
    }

    /**
     * Compute the Fowler–Noll–Vo hash
     * @param s The string
     * @return The fnv-1a hash
     */
    static constexpr Type hash(const char* s) { return hash(s, __builtin_strlen(s)); }

    /**
     * Compute the Fowler–Noll–Vo hash
     * @param str The string
     * @return The fnv-1a hash
     */
    static constexpr Type hash(const std::string& str) { return hash(str.c_str(), str.size()); }

    /**
     * Compute the Fowler–Noll–Vo hash
     * @param str The string view
     * @return The fnv-1a hash
     */
    template<typename C>
    static constexpr Type hash(const std::basic_string_view<C>& str)
    {
        // Accept [ unsigned | signed ] char
        static_assert(std::is_integral<C>::value);
        static_assert(sizeof(C) == 1);
        return hash(str.data(), str.size());
    }
};

using fnv1a32 = fnv1a<32>;
using fnv1a64 = fnv1a<64>;
using fnv1a128 = fnv1a<128>;

constexpr fnv1a32::Type operator"" _fnv1a32(const char* s, const std::size_t l)
{
    return fnv1a32::hash(s, l);
}
constexpr fnv1a64::Type operator"" _fnv1a64(const char* s, const std::size_t l)
{
    return fnv1a64::hash(s, l);
}
constexpr fnv1a128::Type operator"" _fnv1a128(const char* s, const std::size_t l)
{
    return fnv1a128::hash(s, l);
}

// Static unit tests: <https://fnvhash.github.io/fnv-calculator-online/>
static_assert("hello"_fnv1a32 == 0x4f9f2cab);
static_assert("hello"_fnv1a64 == 0xa430d84680aabd0b);
static_assert("hello"_fnv1a128 == Pack128(0xe3e1efd54283d94f, 0x7081314b599d31b3));

using strhash = fnv1a128;
constexpr strhash::Type operator"" _strhash(const char* s, const std::size_t l)
{
    return strhash::hash(s, l);
}

// Lowercase operator[] wrapper for any operator[]-aware types
namespace strhash_lower {
template<typename T>
class _lowercase_container
{
public:
    constexpr _lowercase_container(const T& container)
      : _container(container)
    {}

    constexpr auto operator[](std::size_t index) const
    {
        const auto c = _container[index];
        static_assert(std::is_integral<decltype(c)>::value);
        static_assert(sizeof(c) == 1);
        return c >= 'A' && c <= 'Z' ? (c + 'a' - 'A') : c;
    }

private:
    const T& _container;
};

/**
 * Hash a generic container, using a lowercase modifier, providing 'size' 8-byte characters through operator[]
 */
template<typename T>
constexpr strhash::Type hash(const T& container, std::size_t size)
{
    return strhash::hash_container(strhash_lower::_lowercase_container(container), size);
}

/**
 * Hash a std::string, using a lowercase modifier
 */
static constexpr strhash::Type hash(const std::string& str)
{
    return hash(str, str.size());
}

/**
 * Hash a std::string, using a lowercase modifier
 */
template<typename C>
static constexpr strhash::Type hash(const std::basic_string_view<C>& str)
{
    // Accept [ unsigned | signed ] char
    static_assert(std::is_integral<C>::value);
    static_assert(sizeof(C) == 1);
    return hash(str.data(), str.size());
}
} // namespace strhash_lower

// Case-insensitive version
constexpr strhash::Type operator"" _strhash_lower(const char* s, const std::size_t l)
{
    return strhash_lower::hash(s, l);
}
