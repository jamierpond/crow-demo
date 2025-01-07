#include <cmath>
#include <iostream>
template<typename T>
struct Constants {
    constexpr static auto a = static_cast<T>(33'962.035);
    constexpr static auto b = static_cast<T>(-30'438.8);
    constexpr static auto c = static_cast<T>(41'563.4);
    constexpr static auto d = static_cast<T>(-24'871.969);
};
using CD = Constants<double>;
using CF = Constants<float>;
template<typename T>
constexpr auto d_of_prod(T a, T b, T c, T d) {
    return (a * b) - (c * d);
}
template<typename T>
constexpr auto fma_d_of_prod(T a, T b, T c, T d) {
    auto cd = c * d;
    auto err = std::fma(c, d, -cd);
    auto dop = std::fma(a, b, -cd);
    return dop + err;
}
int main() {
  auto fma_float = fma_d_of_prod(CF::a, CF::b, CF::c, CF::d);
  auto fma_double = fma_d_of_prod(CD::a, CD::b, CD::c, CD::d);

  auto normal_float = d_of_prod(CF::a, CF::b, CF::c, CF::d);
  auto normal_double = d_of_prod(CD::a, CD::b, CD::c, CD::d);

  std::cout << "fma_float: " << fma_float << std::endl;
  std::cout << "fma_double: " << fma_double << std::endl;
  std::cout << "normal_float: " << normal_float << std::endl;
  std::cout << "normal_double: " << normal_double << std::endl;
}
