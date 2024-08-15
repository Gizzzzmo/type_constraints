#include <cstddef>
#include <iostream>
#include <vector>
#include "int.hpp"


void test1(LessThanEq<int, 2> x) {
    return;
}

void test2(GreaterThanEq<int, -7> x) {
    return;
}

constexpr int _fib(int x) {
    if (x == 0 || x == 1) return 1;
    int prev = 1;
    int prevprev = 1;
    for (int i = 0; i < x-1; i++) {
        int next = prev + prevprev;
        prevprev = prev;
        prev = next;
    }
    return prev;
}

template<int n, int m> requires (n >= 0 && m < 46)
auto fib(InRange<int, n, m> x) {
    return N<int>(_fib(x)).assume_lteq<_fib(m)>().template assume_gteq<_fib(n)>();
}


bool api(N<int> x) {
    auto y = x.constrain<InRange<int, 0, 45>>();
    if (!y) return false;

    auto z = y.value();


    auto f = fib(z);
    for (auto i : z.range_to(f)) {
        test2(i);
    }
    test2(f);
    return true;
}

int main() {
    std::cout << "\nEnter an integer between 0 and 10" << std::endl;

    int x;
    InRange<int, 0, 11> result;
    InRange<unsigned int, 0, 11> result_u;
    while (true) {
        std::cin >> x;
        if (std::cin.eof())
            return 1;

        auto input = N<int>(x).constrain_gteq<0>()->contrain_lteq<10>();

        if (input){
            result = input.value();
            result_u = result;
            break;
        }
        else
            std::cout << "what did I say?" << std::endl;
    }

    std::cout << result << std::endl;
    auto calculate = result - constant<int, 9>;
    test2(result);
    test1(calculate);

    result
        .range_to(constant<int, 20>)
        .for_each([](InRange<int, 0, 19> i) {
            std::cout << fib(i) << ", ";
    });
    std::cout << std::endl;

    LessThanEq<int, 10> xxx;
    GreaterThanEq<int, 0> yyy;
    InRange<int, 0, 20> zzz;

    xxx -= yyy;
    xxx -= zzz;

    constant<int, 10>.range_to(constant<int, 0>).for_each([](SafeInPlaceOps<int, int>) {
        std::cout << "hi" << std::endl;
    });

    for (auto i : yyy.range_to(xxx)) {

    };

    for (auto i : xxx.range_to_this()) {

    }

    InRange<int, 0, 10> xxxx(9);
    auto blub = constant<int, 5> - xxxx;

    InRange<int, 0, 10> yyyy = N<int>(10).assume_gteq<0>().assume_lteq<10>();
    yyyy.compiler_hint();
    if (yyyy> 10)
        return 1;


    safe_array<int, 11> arr;

    size_t i = 10;
    arr[i] = 0;
    InRange<size_t, 0, 10> ii(9);
    arr[ii] = 0;
    // test1(result);
    safe_ptr<int, 0, 10> ptr = arr;

    auto ptr2 = ptr + constant<std::ptrdiff_t, 3>;
    
}