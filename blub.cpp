#include <concepts>
#include <iostream>

template<typename T, typename TT>
concept has_validator = requires(TT tt) {
    { T::is_valid(tt) } -> std::convertible_to<bool>;
};

template<typename T, has_validator<T> Derived>
struct SafeInPlaceOps {
    void blub(T x) {
        if (Derived::is_valid(x)) std::cout << "valid\n";
        else std::cout << "invalid\n";
    }
};



int main () {
    

}