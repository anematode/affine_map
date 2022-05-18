/**
 * Abstraction of a simple linear map ax+b and set of linear maps.
 */

#pragma once

#include <array>
#include <stdint.h>
#include <tuple>
#include <type_traits>
#include <utility>

namespace AffineMap {
constexpr int LINEAR_COEFF_MAX = 256;
constexpr int LINEAR_CONST_MAX = 512;

namespace {
    template <int a, int b>
    concept ConstGreaterThanLinear = b >= -a;

    template <int a>
    concept LinearCoefficientInRange = a > 1 && a <= LINEAR_COEFF_MAX;

    template <int b>
    concept ConstantCoefficientInRange = b <= LINEAR_CONST_MAX;
}

template <int a, int b>
concept ValidLinearMapCoefficients = ConstGreaterThanLinear<a, b> && LinearCoefficientInRange<a> && ConstantCoefficientInRange<b>; 

namespace {
    using coefficient_pair = std::pair<int, int>;
}

// a > 1 (to avoid triviality), b >= -a (to avoid underflow), a <= 256, b <= 512
template <int a, int b> requires ValidLinearMapCoefficients<a, b>
struct LinearMap {
    static constexpr coefficient_pair coeffs = { a, b };

    constexpr coefficient_pair get_coeffs() {
        return coeffs;
    }

    inline int64_t apply(int64_t x) {
        return a * x + b;
    }
};

template <class T>
concept ValidLinearMap = requires (T x) {
    { LinearMap{x} } -> std::same_as<T>;
};

template <int n, int cnt>
concept LinearMapIndexInRange = 0 <= n && n < cnt;

template <ValidLinearMap... LinearMaps>
struct LinearMapSet {
    // Number of maps
    static constexpr std::size_t map_count = sizeof...(LinearMaps);
    // Coefficients of the maps themselves
    static constexpr std::array<coefficient_pair, map_count> coeffs = {LinearMaps::coeffs...};

    constexpr auto get_coeffs() {
        return coeffs;
    }

    template<int idx> requires LinearMapIndexInRange<idx, map_count>
    using NthMap = typename std::tuple_element<idx, std::tuple<LinearMaps...> >::type;

    std::array<int64_t, map_count> apply(int64_t x) {
        std::array<int64_t, map_count> a;

        for (int i = 0; i < map_count; ++i) {
            a[i] = coeffs[i][0] * x + coeffs[i][1];
        }
        
        return a;
    }
};
}
