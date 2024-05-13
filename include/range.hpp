#ifndef __RANGE_HPP__
#define __RANGE_HPP__ 1

#include <cstdio>
#include <cstdint>

template<const unsigned int FROM=0,const unsigned int TO=4,const unsigned int STEP=1>
struct range final
{
    using value = unsigned int;
    struct Iterator final {
        value VALUE;
        Iterator(const value v):VALUE{v} {}
        Iterator(const Iterator& obj) = default;
        Iterator& operator = (const Iterator& obj) = default;
        constexpr bool operator != (const Iterator &obj) const {
            return VALUE < obj.VALUE;
        }
        void operator ++ () {
            VALUE+=STEP;
        }
        Iterator& operator += (const value b) {
            VALUE+=b;
            return *this;
        }
        constexpr value operator * () const {
            return this->VALUE;
        }
        constexpr auto operator [] (const value i) const {
            constexpr auto result = i*STEP+FROM;
            static_assert(result < TO,"Invalid Index");
            return result;
        }
    };
    range() = default;
    constexpr Iterator begin(){
        return Iterator{FROM};
    }
    constexpr Iterator end(){
        return Iterator{TO};
    }
};

#endif