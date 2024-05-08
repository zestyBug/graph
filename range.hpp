#ifndef __RANGE_HPP__
#define __RANGE_HPP__ 1

#include <stdio.h>

template<const ssize_t FROM=0,const ssize_t TO=4,const ssize_t STEP=1>
struct range final
{
    using value = ssize_t;
    struct Iterator final {
        ssize_t VALUE;
        Iterator(const ssize_t v):VALUE{v} {}
        Iterator(const Iterator& obj) = default;
        Iterator& operator = (const Iterator& obj) = default;
        constexpr bool operator != (const Iterator &obj) const {
            return VALUE < obj.VALUE;
        }
        void operator ++ () {
            VALUE+=STEP;
        }
        void operator + (const ssize_t b) {
            VALUE+=b;
        }
        constexpr ssize_t operator * () const {
            return this->VALUE;
        }
        constexpr auto operator [] (const ssize_t i) const {
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