
#include <array>
#include <concepts>
#include <cstddef>
#include <functional>
#include <type_traits>
#include <optional>


#if __cplusplus == 202302L
#include <utility>
using std::unreachable;
#else
[[noreturn]] inline void unreachable()
{
#if defined(_MSC_VER) && !defined(__clang__) // MSVC
    __assume(false);
#else // GCC, Clang
    __builtin_unreachable();
#endif
}
#endif


template<typename T>
concept unsigned_int = (std::integral<T> && std::is_unsigned_v<T>);

template<typename T>
concept signed_int = (std::integral<T> && std::is_signed_v<T>);

template<typename T, typename TT>
concept has_validator = requires(TT tt) {
    { T::is_valid(tt) } -> std::convertible_to<bool>;
};

template<signed_int T, typename Derived>
class SafeInPlaceOps {
    private:
        Derived& as_derived() {
            return *static_cast<Derived*>(this);
        }
        T& _x() {
            return as_derived().x;
        }
    protected:
        constexpr SafeInPlaceOps() { static_assert(has_validator<Derived, T> && std::derived_from<Derived, SafeInPlaceOps<T, Derived>>); }
    public:
        Derived& decrement_unsafe(T y) { _x() -= y; return as_derived(); }
        Derived& increment_unsafe(T y) { _x() += y; return as_derived(); }
        Derived& multiply_unsafe(T y) { _x() *= y; return as_derived(); }
        Derived& divide_unsafe(T y) { _x() /= y; return as_derived(); }

        bool try_decrement(T y) {
            T diff = _x() - y;
            if (!Derived::is_valid(diff)) return false;
            
            _x() = diff;
            return true;
        }
        bool try_increment(T y) {
            int sum = _x() + y;
            if (!Derived::is_valid(sum)) return false;

            _x() = sum;
            return true;
        }
        bool try_multiply(T y) {
            T prod = _x() * y;
            if (!Derived::is_valid(prod)) return false;
            
            _x() = prod;
            return true;
        }
        bool try_divide(T y) {
            int quot = _x() / y;
            if (!Derived::is_valid(quot)) return false;

            _x() = quot;
            return true;
        }
};


template<std::integral T>
class N {};

template<signed_int T, T n>
class LessThanEq;

template<signed_int T, T n>
class GreaterThanEq;

template<std::integral T, T n, T m>
class InRange {};

template<signed_int T>
class Range;

template<signed_int T, T n>
class RangeLt;

template<signed_int T, T n>
class RangeGt;

template<signed_int T, T n, T m>
class RangeInterval;



template<std::integral T, T x>
inline static constexpr const InRange<T, x, x> constant = N<T>(x).template assume<InRange<T, x, x>>();

template<signed_int T>
class N<T> {
    public:
        using Self = N<T>;
        constexpr N(T x) : x(x) { static_assert(std::is_trivially_copyable<Self>()); }
        constexpr operator T&() { return x; }
        constexpr operator const T&() const { return x; }
        template<signed_int TT> requires (std::is_same_v<std::common_type_t<T, TT>, TT>)
        constexpr operator N<TT>() const { return N<TT>(x); }

        template<T n>
        constexpr LessThanEq<T, n> assume_lteq() const {
            return LessThanEq<T, n>(*this);
        }
        template<T n>
        std::optional<LessThanEq<T, n>> constrain_lteq() const {
            if (x > n) return std::nullopt;
            return assume_lteq<n>();
        }

        template<has_validator<T> U>
        constexpr U assume() const {
            if (!U::is_valid(x)) unreachable();
            return U(*this);
        }

        template<has_validator<T> U>
        std::optional<U> constrain() const {
            if (!U::is_valid(x)) return std::nullopt;
            return assume<U>();
        }

        template<T n>
        constexpr GreaterThanEq<T, n> assume_gteq() const {
            return GreaterThanEq<T, n>(*this);
        }
        template<T n>
        std::optional<GreaterThanEq<T, n>> constrain_gteq() const {
            if (x < n) return std::nullopt;
            return assume_gteq<n>();
        }

        template<signed_int TT>
        auto range_to(TT y, GreaterThanEq<TT, 1> step = ::constant<TT, 1>) const {
            return Range<std::common_type_t<T, TT>>(x, y, step);
        }
        
        template<signed_int TT, TT m>
        auto range_to(LessThanEq<TT, m> y, GreaterThanEq<TT, 1> step = ::constant<TT, 1>) const {
            return RangeLt<std::common_type_t<T, TT>, m>(*this, y);
        }

        template<signed_int TT, TT m, TT mm>
        auto range_to(InRange<TT, m, mm> y, GreaterThanEq<TT, 1> step = ::constant<TT, 1>) const {
            return RangeLt<std::common_type_t<T, TT>, mm>(*this, y, step);
        }

    protected:
    
        T x;
};

template<unsigned_int T>
class N<T> {
    public:
        using Self = N<T>;
        constexpr N(T x) : x(x) { static_assert(std::is_trivially_copyable<Self>()); }
        operator T&() { return x; }
        operator T&() const { return x; } 

        template<T n, T m>
        constexpr InRange<T, n, m> assume_in_range() const {
            return InRange<T, n, m>(x);
        }

        template<has_validator<T> U>
        constexpr U assume() const {
            if (!U::is_valid(x)) unreachable();
            return U(*this);
        }

        template<has_validator<T> U>
        std::optional<U> constrain() const {
            if (!U::is_valid(x)) return std::nullopt;
            return assume<U>();
        }
        template<T n, T m>
        std::optional<InRange<T, n, m>> constrain_to_range() const {
            if constexpr (m >= n) {
                if (x < n || x > m) return std::nullopt;
            } else {
                if (x < n && x > m) return std::nullopt;
            }
            return assume_in_range<n, m>();
        }

    protected:
        T x;
};



template<signed_int T, T n>
class LessThanEq : public N<T>, public SafeInPlaceOps<T, LessThanEq<T, n>> {
    private:
        using Self = LessThanEq<T, n>; 
        constexpr LessThanEq(N<T> x) : N<T>(x) { compiler_hint(); static_assert(std::is_trivially_copyable<Self>()); }
        template<std::integral>
        friend class N;
        template<signed_int TT, TT>
        friend class LessThanEq;
        template<signed_int TT, TT>
        friend class GreaterThanEq;
        template<std::integral TT, TT, TT>
        friend class InRange;
        friend class SafeInPlaceOps<T, LessThanEq<T, n>>;
        
        Self operator++(int) { this->x++; return *this; }
        Self& operator++() { this->x++; return *this; }
        
        Self& operator+=(int) { unreachable(); return *this; }
        Self& operator-=(int) { unreachable(); return *this; }
        Self& operator*=(int) { unreachable(); return *this; }
        Self& operator/=(int) { unreachable(); return *this; }
        Self& operator<<=(int) { unreachable(); return *this; }
        Self& operator>>=(int) { unreachable(); return *this; }
        Self& operator|=(int) { unreachable(); return *this; }
        Self& operator&=(int) { unreachable(); return *this; }

    public:
        constexpr LessThanEq() : N<T>(n) { compiler_hint(); static_assert(std::is_trivially_copyable<Self>()); }
        consteval LessThanEq(T x) : N<T>(x) { compiler_hint(); }

        template<T m> requires (m > n)
        constexpr operator LessThanEq<T, m>() const { return LessThanEq<T, m>(*this); };

        template<T m>
        constexpr InRange<T, m, n> assume_gteq() const {
            return InRange<T, m, n>(*this);
        }
        template<T m>
        std::optional<InRange<T, m, n>> constrain_gteq() const {
            if (this->x < m) return std::nullopt;
            return assume_gteq<m>();
        }

        template<T m>
        constexpr LessThanEq<T, m + n> operator+(LessThanEq<T, m> other) const {
            return LessThanEq<T, m + n>(N<T>(this->x + other.x));
        }

        template<T m>
        constexpr LessThanEq<T, n - m> operator-(GreaterThanEq<T, m> other) const {
            return LessThanEq<T, n - m>(N<T>(this->x - other.x));
        }

        
        constexpr Self operator--(int) { this->x--; return *this; }
        constexpr Self& operator--() { this->x--; return *this; }

        template<signed_int TT, TT nn> requires(nn >= 0)
        constexpr Self& operator-=(GreaterThanEq<TT, nn> other) { this->x -= other; return *this; }

        template<signed_int TT, TT nn, TT mm> requires(nn >= 0)
        constexpr Self& operator-=(InRange<TT, nn, mm> other) { this->x -= other; return *this; }

        template<signed_int TT, TT nn> requires(nn <= 0)
        constexpr Self& operator+=(LessThanEq<TT, nn> other) { this->x += other; return *this; }

        template<signed_int TT, TT nn, TT mm> requires(mm <= 0)
        constexpr Self& operator+=(InRange<TT, nn, mm> other) { this->x += other; return *this; }
        
        template<signed_int TT, TT nn> requires(nn > 0)
        constexpr Self& operator*=(GreaterThanEq<TT, nn> other) { this->x *= other; return *this; }

        template<signed_int TT, TT nn, TT mm> requires(nn >= 0)
        constexpr Self& operator*=(InRange<TT, nn, mm> other) { this->x *= other; return *this; }
        
        template<signed_int TT, signed_int TTT = T>
        auto range_to(TT y, GreaterThanEq<TTT, 1> step = ::constant<TTT, 1>) const {
            return Range<std::common_type_t<T, TT, TTT>>(*this, y, step);
        }

        template<signed_int TT, TT m, signed_int TTT = T>
        auto range_to(LessThanEq<TT, m> y, GreaterThanEq<TTT, 1> step = ::constant<TTT, 1>) const {
            return RangeLt<std::common_type_t<T, TT, TTT>, m>(*this, y, step);
        }

        template<signed_int TT, TT m, TT mm, signed_int TTT = T>
        auto range_to(InRange<TT, m, mm> y, GreaterThanEq<TTT, 1> step = ::constant<TTT, 1>) const {
            return RangeLt<std::common_type_t<T, TT, TTT>, mm>(*this, y, step);
        }

        template<signed_int TT = T>
        auto range_to_this(GreaterThanEq<TT, 1> step = ::constant<TT, 1>) {
            return RangeInterval<std::common_type_t<T, TT>, 0, n>(::constant<std::common_type_t<T, TT>, 0>, *this);
        }

        constexpr static bool is_valid(T x) {
            return x <= n;
        }


        constexpr void compiler_hint() {
            if (!Self::is_valid(this->x)) unreachable();
        }
};


template<signed_int T, T n>
class GreaterThanEq : public N<T>, public SafeInPlaceOps<T, GreaterThanEq<T, n>> {
    private:
        using Self = GreaterThanEq<T, n>;
        constexpr GreaterThanEq(N<T> x) : N<T>(x) { compiler_hint(); static_assert(std::is_trivially_copyable<Self>()); }
        template<std::integral>
        friend class N;
        template<signed_int TT, TT>
        friend class LessThanEq;
        template<signed_int TT, TT>
        friend class GreaterThanEq;
        template<std::integral TT, TT, TT>
        friend class InRange;
        friend class SafeInPlaceOps<T, GreaterThanEq<T, n>>;
        
        Self operator++(int) { this->x++; return *this; }
        Self& operator++() { this->x++; return *this; }
        
        Self& operator+=(int) { unreachable(); return *this; }
        Self& operator-=(int) { unreachable(); return *this; }
        Self& operator*=(int) { unreachable(); return *this; }
        Self& operator/=(int) { unreachable(); return *this; }
        Self& operator<<=(int) { unreachable(); return *this; }
        Self& operator>>=(int) { unreachable(); return *this; }
        Self& operator|=(int) { unreachable(); return *this; }
        Self& operator&=(int) { unreachable(); return *this; }
    public:
        constexpr GreaterThanEq() : N<T>(n) { compiler_hint(); static_assert(std::is_trivially_copyable<Self>()); }
        consteval GreaterThanEq(T x) : N<T>(x) { compiler_hint(); }

        template<T m> requires (m < n)
        constexpr operator GreaterThanEq<T, m>() const { return GreaterThanEq<T, m>(*this); }

        template<T m>
        constexpr InRange<T, n, m> assume_lteq() const {
            return InRange<T, n, m>(*this);
        }
        template<T m>
        std::optional<InRange<T, n, m>> contrain_lteq() const {
            if (this->x > m) return std::nullopt;
            return assume_lteq<m>();
        }

        template<T m>
        constexpr GreaterThanEq<T, m + n> operator+(GreaterThanEq<T, m> other) const {
            return GreaterThanEq<T, m + n>(N<T>(this->x + other.x));
        }

        template<T m>
        constexpr GreaterThanEq<T, n - m> operator-(LessThanEq<T, m> other) const {
            return GreaterThanEq<T, n - m>(N<T>(this->x - other.x));
        }

        template<signed_int TT, signed_int TTT = T>
        auto range_to(TT y, GreaterThanEq<TTT, 1> step = ::constant<TTT, 1>) const {
            return RangeGt<std::common_type_t<T, TT, TTT>, n>(*this, y);
        }

        template<signed_int TT, TT m, signed_int TTT = T>
        auto range_to(LessThanEq<TT, m> y, GreaterThanEq<TTT, 1> step = ::constant<TTT, 1>) const {
            return RangeInterval<std::common_type_t<T, TT, TTT>, n, m>(*this, y, step);
        }

        template<signed_int TT, TT m, TT mm, signed_int TTT = T>
        auto range_to(InRange<TT, m, mm> y, GreaterThanEq<TTT, 1> step = ::constant<TTT, 1>) const {
            return RangeInterval<std::common_type_t<T, TT, TTT>, n, mm>(*this, y, step);
        }

        template<signed_int TT = T>
        auto range_to_this(GreaterThanEq<TT, 1> step = ::constant<TT, 1>) {
            return RangeGt<std::common_type_t<T, TT>, 0>(::constant<std::common_type_t<T, TT>, 0>, *this);
        }

        constexpr static bool is_valid(T x) {
            return x >= n;
        }

        constexpr void compiler_hint() {
            if (!Self::is_valid(this->x)) unreachable();
        }
        
};

template<signed_int T, T n, T m> requires(n <= m)
class InRange<T, n, m> : public N<T>, public SafeInPlaceOps<T, InRange<T, n, m>> {
    private:
        using Self = InRange<T, n, m>;
        constexpr InRange(N<T> x) : N<T>(x) { compiler_hint(); }
        template<std::integral>
        friend class N;
        template<signed_int TT, TT>
        friend class LessThanEq;
        template<signed_int TT, TT>
        friend class GreaterThanEq;
        template<std::integral TT, TT, TT>
        friend class InRange;
        template<std::integral TT, TT>
        friend class __;
        friend class SafeInPlaceOps<T, InRange<T, n, m>>;

        
        Self operator++(int) { this->x++; return *this; }
        Self& operator++() { this->x++; return *this; }
        
        Self& operator+=(int) { unreachable(); return *this; }
        Self& operator-=(int) { unreachable(); return *this; }
        Self& operator*=(int) { unreachable(); return *this; }
        Self& operator/=(int) { unreachable(); return *this; }
        Self& operator<<=(int) { unreachable(); return *this; }
        Self& operator>>=(int) { unreachable(); return *this; }
        Self& operator|=(int) { unreachable(); return *this; }
        Self& operator&=(int) { unreachable(); return *this; }
    public:
        constexpr InRange() : N<T>(n) { compiler_hint(); static_assert(std::is_trivially_copyable<Self>()); }
        consteval InRange(T x) : N<T>(x) { compiler_hint(); }

        template<unsigned_int TT>
        constexpr operator InRange<TT, n, m>() const { return InRange<TT, n, m>(*this); }

        template<signed_int TT, TT nn, TT mm> requires (nn <= n && mm >= m)
        constexpr operator InRange<TT, nn, mm>() const { return InRange<TT, nn, mm>(*this); }

        template<signed_int TT, TT mm> requires (mm >= m)
        constexpr operator LessThanEq<TT, mm>() const { return LessThanEq<TT, mm>(*this); }
        template<signed_int TT, TT nn> requires (nn <= n)
        constexpr operator GreaterThanEq<TT, nn>() const { return GreaterThanEq<TT, nn>(*this); }

        template<signed_int TT, TT nn, TT mm>
        constexpr InRange<std::common_type_t<T, TT>, n + nn, m + mm> operator+(InRange<TT, nn, mm> other) const {
            return InRange<std::common_type_t<T, TT>, n + nn, m + mm>(N<T>(this->x + other.x));
        }

        template<signed_int TT, TT nn, TT mm>
        constexpr auto operator-(InRange<TT, nn, mm> other) const {
            return *this + InRange<TT, -mm, -nn>(N<TT>(-other.x));
        }

        template<signed_int TT, T nn>
        constexpr GreaterThanEq<std::common_type_t<T, TT>, nn + n> operator+(GreaterThanEq<TT, nn> other) const {
            return GreaterThanEq<std::common_type_t<T, TT>, n>(*this) + other;
        }

        template<signed_int TT, TT mm>
        constexpr LessThanEq<std::common_type_t<T, TT>, mm + m> operator+(LessThanEq<TT, mm> other) const {
            return LessThanEq<std::common_type_t<T, TT>, m>(*this) + other;
        }

        template<signed_int TT, TT nn>
        constexpr auto operator-(GreaterThanEq<TT, nn> other) const {
            return LessThanEq<T, m>(*this) - other;
        }

        template<signed_int TT, TT mm>
        constexpr auto operator-(LessThanEq<TT, mm> other) const {
            return GreaterThanEq<T, n>(*this) - other;
        }

        template<T mm>
        constexpr InRange<T, n, mm> assume_lteq() const {
            return InRange<T, n, mm>(*this);
        }
        template<T mm>
        std::optional<InRange<T, n, mm>> constrain_lteq() const {
            if (this->x > n) return std::nullopt;
            return assume_lteq<n>();
        }

        template<T nn>
        constexpr InRange<T, nn, m> assume_gteq() const {
            return InRange<T, nn, m>(*this);
        }
        template<T nn>
        std::optional<InRange<T, nn, m>> constrain_gteq() const {
            if (this->x < n) return std::nullopt;
            return assume_gteq<n>();
        }

        template<signed_int TT, signed_int TTT = T>
        auto range_to(TT y, GreaterThanEq<TTT, 1> step = ::constant<TTT, 1>) const {
            return RangeGt<std::common_type_t<T, TT, TTT>, n>(*this, y, step);
        }

        template<signed_int TT, TT mm, signed_int TTT = T>
        auto range_to(LessThanEq<TT, mm> y, GreaterThanEq<TTT, 1> step = ::constant<TTT, 1>) const {
            return RangeInterval<std::common_type_t<T, TT, TTT>, n, mm>(*this, y, step);
        }

        template<signed_int TT, TT nn, TT mm, signed_int TTT = T>
        auto range_to(InRange<TT, nn, mm> y, GreaterThanEq<TTT, 1> step = ::constant<TTT, 1>) const {
            return RangeInterval<std::common_type_t<T, TT, TTT>, n, mm>(*this, y, step);
        }   
        
        template<signed_int TT = T>
        auto range_to_this(GreaterThanEq<TT, 1> step = ::constant<TT, 1>) {
            return RangeInterval<std::common_type_t<T, TT>, 0, m>(::constant<std::common_type_t<T, TT>, 0>, *this);
        }

        constexpr static bool is_valid(T x) {
            return x >= n && x <= m;
        }

        constexpr void compiler_hint() {
            if (!Self::is_valid(this->x)) unreachable();
        }
}; 

template<unsigned_int T, T n, T m> requires (n - 1 == m) // if n-1 is m, then there are effectively no constraints
class InRange<T, n, m> : public N<T> {
    public:
        using N<T>::N;
};


template<unsigned_int T, T n, T m> requires (n - 1 != m)
class InRange<T, n, m> : public N<T> {
    private:
        using Self = InRange<T, n, m>;
        constexpr InRange(N<T> x) : N<T>(x) { compiler_hint(); static_assert(std::is_trivially_copyable<Self>()); }
        template<std::integral>
        friend class N;
        template<signed_int TT, TT>
        friend class LessThanEq;
        template<signed_int TT, TT>
        friend class GreaterThanEq;
        template<std::integral TT, TT, TT>
        friend class InRange;
        template<std::integral TT, TT>
        friend class __;
        
        Self operator++(int) { this->x++; return *this; }
        Self& operator++() { this->x++; return *this; }
        
        Self& operator+=(T x) { this->x += x; return *this; }
        Self& operator-=(T x) { this->x -= x; return *this; }
        Self& operator*=(T x) { this->x += x; return *this; }
        Self& operator/=(T x) { this->x += x; return *this; }
        Self& operator<<=(T x) { this->x += x; return *this; }
        Self& operator>>=(T x) { this->x += x; return *this; }
        Self& operator|=(T x) { this->x += x; return *this; }
        Self& operator&=(T x) { this->x += x; return *this; }

    public:
        constexpr InRange() : N<T>(n) { compiler_hint(); static_assert(std::is_trivially_copyable<Self>()); }
        consteval InRange(T x) : N<T>(x) { compiler_hint(); }

        template<signed_int TT> requires (n <= m)
        operator InRange<TT, n, m>() const { return InRange<TT, n, m>(*this); }

        template<unsigned_int TT, TT nn, TT mm> requires ((nn <= mm && nn <= n && mm >= m) ||  (nn > mm)) // todo fix constraint
        operator InRange<TT, nn, mm>() const { return InRange<TT, nn, mm>(*this); }

        template<unsigned_int TT, TT nn, TT mm>
        InRange<std::common_type_t<TT, T>, n + nn, m + mm> operator+(InRange<TT, nn, mm> other) const {
            using CT = std::common_type_t<T, TT>;
            if constexpr ((mm - nn) + (m - n) < (mm - nn)) return N<CT>(N<T>(this->x + other.x));
            return InRange<CT, n + nn, m + mm>(N<T>(this->x + other.x));
        }

        template<unsigned_int TT, TT nn, TT mm>
        auto operator-(InRange<TT, nn, mm> other) const {
            return *this + InRange<TT, -mm, -nn>(N<TT>(-other.x));
        }

        
        Self& decrement_unsafe(T x) { return (*this)-= x; }
        Self& increment_unsafe(T x) { return (*this)+= x; }

        template<T nn, T mm>
        std::optional<InRange<T, nn, mm>> constrain_to_range() const {
            if constexpr (mm >= nn) {
                if (this->x < nn || this->x > mm) return std::nullopt;
            } else {
                if (this->x < nn && this->x > mm) return std::nullopt;
            }
            return static_cast<N<T>&>(*this).template assume_in_range<nn, mm>();
        }

        constexpr void compiler_hint() {
            if (!Self::is_valid(this->x)) unreachable();
        }

        constexpr static bool is_valid(T x) {
            if constexpr (m >= n) {
                return (x >= n && x <= m);
            } else {
                return (x >= n || x <= m);
            }
        }

};


struct CantExist {
    CantExist() = delete;
    template<typename T>
    operator T() const { return *reinterpret_cast<T*>(0); }
};




template<signed_int T>
class Range {
    public:
        Range(T first, T sentinel, GreaterThanEq<T, 1> step = constant<T, 1>) : first(first), sentinel(sentinel), step(step) {}
        void for_each(std::function<void(T)> f) {
            for(T i = first; i < sentinel; i += step) {
                f(i);
            }
        }

        T begin() {
            return first;
        }

        T end() {
            return sentinel;
        }
    private:
        T first;
        T sentinel;
        GreaterThanEq<T, 1> step;
};

template<std::integral T, typename U> requires(std::is_convertible_v<U, T>)
class IteratorWrapper {
    private:
        using Self = IteratorWrapper<T, U>;
        template<signed_int TT, TT>
        friend class RangeGt;
        template<signed_int TT, TT>
        friend class RangeLt;
        template<signed_int TT, TT, TT>
        friend class RangeInterval;
        template<std::integral, typename, typename>
        friend class MakeIterable;
        IteratorWrapper(T value, T step) : value(value), step(step) {}

    public:
        Self operator++() {
            value++;
            return *this;
        }
        U operator*() {
            return N<T>(value).template assume<U>();
        }
        bool operator!=(Self other) {
            return value != other.value;
        }
    private:
        T value;
        T step;
};

template<std::integral T, typename Derived, typename Constrained_T>
class MakeIterable {
    private:
        Derived& as_derived() {
            return *static_cast<Derived*>(this);
        }
    protected:
        MakeIterable() { static_assert(true); } // todo put in requirements
    public:
        IteratorWrapper<T, Constrained_T> begin() {
            return IteratorWrapper<T, Constrained_T>(as_derived().first, as_derived().step);
        }
        IteratorWrapper<T, Constrained_T> end() {
            return IteratorWrapper<T, Constrained_T>(as_derived().sentinel, as_derived().step);
        }
};

template<signed_int T, T n>
struct RangeLt : MakeIterable<T, RangeLt<T, n>, LessThanEq<T, n-1>>{
    public:
        RangeLt(T first, LessThanEq<T, n> sentinel, GreaterThanEq<T, 1> step = constant<T, 1>) : first(first), sentinel(sentinel), step(step) {}
        void for_each(std::function<void(LessThanEq<T, n-1>)> f) {
            for(LessThanEq<T, n-1> i = first.template assume_lteq<n-1>(); i < sentinel; i.increment_unsafe(step)) {
                i.compiler_hint();
                f(i);
            }
        }
    private:
        T first;
        LessThanEq<T, n> sentinel;
        GreaterThanEq<T, 1> step;
};


template<signed_int T, T n>
struct RangeGt : MakeIterable<T, RangeGt<T, n>, GreaterThanEq<T, n>> {
    public:
        friend class MakeIterable<T, RangeGt<T, n>, GreaterThanEq<T, n>>;
        RangeGt(GreaterThanEq<T, n> first, T sentinel, GreaterThanEq<T, 1> step = constant<T, 1>) : first(first), sentinel(sentinel), step(step) {}
        void for_each(std::function<void(GreaterThanEq<T, n>)> f) {
            for(GreaterThanEq<T, n> i = first; i < sentinel; i += step) {
                i.compiler_hint();
                f(i);
            }
        }

    private:
        GreaterThanEq<T, n> first;
        T sentinel;
        GreaterThanEq<T, 1> step;
};

template<signed_int T, T n, T m>
struct RangeInterval : MakeIterable<T, RangeInterval<T, n, m>, InRange<T, n, m-1>> {
    public:
        friend MakeIterable<T, RangeInterval<T, n, m>, InRange<T, n, m-1>>;
        RangeInterval(GreaterThanEq<T, n> first, LessThanEq<T, m> sentinel, GreaterThanEq<T, 1> step = constant<T, 1>) : first(first), sentinel(sentinel), step(step) {}
        void for_each(std::function<void(InRange<T, n, m-1>)> f) {
            for(InRange<T, n, m-1> i = first.template assume_lteq<m-1>(); i < sentinel; i.increment_unsafe(step)) {
                i.compiler_hint();
                f(i);
            }
        }
    private:
        GreaterThanEq<T, n> first;
        LessThanEq<T, m> sentinel;
        GreaterThanEq<T, 1> step;
};

template<signed_int T, T n, T m> requires (n >= m)
struct RangeInterval<T, n, m> {
    public:
        RangeInterval(GreaterThanEq<T, n> first, LessThanEq<T, m> sentinel, GreaterThanEq<T, 1> step = constant<T, 1>) : first(first), sentinel(sentinel), step(step) {}
        void for_each(std::function<void(CantExist)> f) {
            return;
        }
    private:
        GreaterThanEq<T, n> first;
        LessThanEq<T, m> sentinel;
        GreaterThanEq<T, 1> step;
};

template<typename T, size_t n>
class safe_array;

template<typename T, std::ptrdiff_t n = 0, std::ptrdiff_t m = 1>
class safe_ptr {
    private:
        template<typename, std::ptrdiff_t, std::ptrdiff_t>
        friend class safe_ptr;
        template<typename, size_t>
        friend class safe_array;

        using Self = safe_ptr<T, n, m>;
        constexpr safe_ptr(T* pointer) : pointer(pointer) { static_assert(std::is_trivially_copyable<safe_ptr<T, n, m>>()); }

    public:
        template<std::ptrdiff_t nn, std::ptrdiff_t mm> requires(nn <= n && mm >= m && !(nn == n && mm == m))
        constexpr safe_ptr(safe_ptr<T, nn, mm> other) : pointer(other.pointer) { static_assert(std::is_trivially_copyable<safe_ptr<T, n, m>>()); }
        constexpr safe_ptr(T& obj) requires(n == 0 && m == 1) : pointer(&obj) { static_assert(std::is_trivially_copyable<safe_ptr<T, n, m>>()); }
        template<typename TT> requires(n == 0 && m == 1 && std::convertible_to<TT*, T*>)
        constexpr safe_ptr(safe_ptr<TT> other) : pointer(other.pointer) { static_assert(std::is_trivially_copyable<safe_ptr<T, n, m>>()); }

        template<std::ptrdiff_t nn = 0, std::ptrdiff_t mm = 0> requires(nn >= n && mm < m)
        safe_ptr<T> to_singleton(InRange<std::ptrdiff_t, nn, mm> i = constant<std::ptrdiff_t, 0>) const {
            return safe_ptr<T>(&pointer[i]);
        }

        constexpr T& operator*() const requires(n <= 0 && m > 0) { return *pointer; }
        constexpr T* operator->() const requires(n <= 0 && m > 0) { return pointer; }

        template<std::ptrdiff_t nn, std::ptrdiff_t mm> requires(n <= nn && m > mm)
        constexpr T& operator[](InRange<std::ptrdiff_t, nn, mm> i) const { return pointer[i]; }
        template<std::ptrdiff_t l, std::ptrdiff_t u>
        constexpr safe_ptr<T, n - l, m - u> operator+(InRange<std::ptrdiff_t, l, u> x) {
            return safe_ptr<T, n - l, m - u>(pointer);
        }
    private:
        T* pointer;
};


template<typename T>
safe_ptr<T, 0, 1> safe_ptr_to(T& x) {
    return safe_ptr<T, 0, 1>(x);
}

template<typename T, size_t n>
class safe_array : public std::array<T, n> {
    public:
        using std::array<T, n>::array;
    private:
        T& operator[](size_t i) { return (*static_cast<std::array<T, n>*>(this))[i]; }
        const T& operator[](size_t i) const { return (*static_cast<const std::array<T, n>*>(this))[i]; }
    public:
        template<size_t l, size_t u> requires(l >= 0 && u < n)
        T& operator[](InRange<size_t, l, u> i) { return (*this)[static_cast<int>(i)]; }
        template<size_t l, size_t u> requires(l >= 0 && u < n)
        const T& operator[](InRange<size_t, l, u> i) const { return (*this)[static_cast<int>(i)]; }
        template<std::ptrdiff_t l, std::ptrdiff_t u> requires(l >= 0 && u <= n)
        operator safe_ptr<T, l, u>() {
            return safe_ptr<T, l, u>(&this->front());
        }
        template<std::ptrdiff_t l, std::ptrdiff_t u> requires(l >= 0 && u <= n)
        operator safe_ptr<const T, l, u>() const {
            return safe_ptr<const T, l, u>(&this->front());
        }

};