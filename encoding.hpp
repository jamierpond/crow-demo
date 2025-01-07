#include <array>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>

struct Base62Encoding {
  constexpr static std::string_view chars = "0123456789"
                                            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                            "abcdefghijklmnopqrstuvwxyz";
  constexpr static auto size() { return chars.size(); }
};

static_assert(Base62Encoding::size() == 62);
static_assert(0b00000000'00111111 == 0x3f);
static_assert(0b00000000'00111111 == 63);
static_assert(0b00000000'00000000 == 0);

template <typename CharacterSet> constexpr auto apply_encoding(uint64_t value) {
  std::string result{};
  result.reserve(8);

  if (value == 0) {
    return std::string{"0"};
  }

  auto multiple_of_base = 1;

  // todo make associative iteration ?
  while (value > 0) {
    auto index = value % CharacterSet::size();
    auto value_now = value * multiple_of_base;

    result.push_back(CharacterSet::chars[index]);

    value /= CharacterSet::size();
    multiple_of_base *= CharacterSet::size();
  }

  return result;
}

template <typename CharacterSet>
constexpr auto reverse_encoding(const std::string_view &str) {
  auto multiple_of_base = 1;
  auto result = 0;
  auto base = CharacterSet::size();

  for (int i = 0; i < str.size(); i++) {
    auto c = str[i];
    auto index = CharacterSet::chars.find(c);
    if (index == std::string::npos) {
      throw std::runtime_error("Invalid character in string");
    }

    result += index * multiple_of_base;
    multiple_of_base *= base;
  }
  return result;
}

template <typename CharacterSet> struct Encoding {
  constexpr static auto apply(uint64_t value) {
    return apply_encoding<CharacterSet>(value);
  }
  constexpr static auto reverse(const std::string_view &str) {
    return reverse_encoding<CharacterSet>(str);
  }
};

typedef Encoding<Base62Encoding> Base62;

static_assert([] {
  constexpr auto random_numbers =
      std::array<uint64_t, 11>{0, 1, 10, 15, 62, 63, 64, 65, 100, 0xFFFFFFF + 0xFFFFFFFF};
  for (auto n : random_numbers) {
    if (Base62::reverse(Base62::apply(n)) != n) {
      return false;
    }
  }
  return true;
}());

static_assert(Base62::apply(0x0000) == "0");
static_assert(Base62::apply(0x0001) == "1");
static_assert(Base62::apply(0x000A) == "A");
static_assert(Base62::apply(0x000f) == "F");
static_assert(Base62::reverse("F") == 0x000f);
static_assert(Base62::apply(Base62Encoding::size()) == "01");
static_assert(Base62::apply(Base62Encoding::size() + 1) == "11");
static_assert(Base62::apply(Base62Encoding::size() + 15) == "F1");
static_assert(reverse_encoding<Base62Encoding>("01") == Base62Encoding::size());
static_assert(reverse_encoding<Base62Encoding>("F1") ==
              Base62Encoding::size() + 15);
