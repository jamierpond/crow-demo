#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>


template <auto Strings>
struct CharacterSet {
  constexpr static std::string_view chars = Strings();
  constexpr static auto size() { return chars.size(); }
  constexpr static auto contains(char c) {
    return chars.find(c) != std::string::npos;
  }
  constexpr static auto find(char c) { return chars.find(c); }
};


namespace encodings {
constexpr auto Hex = [] { return "0123456789ABCDEF"; };
constexpr auto Decimal = [] { return "0123456789"; };
constexpr auto Binary = [] { return "01"; };
constexpr auto Base62 = [] { return "0123456789"
                                     "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                     "abcdefghijklmnopqrstuvwxyz"; };
} // namespace encodings

typedef CharacterSet<encodings::Base62> Base62Encoding;
typedef CharacterSet<encodings::Hex> HexEncoding;
typedef CharacterSet<encodings::Decimal> DecimalEncoding;
typedef CharacterSet<encodings::Binary> BinaryEncoding;

template <typename CharacterSet, typename T = uint64_t>
constexpr auto apply_encoding(T value) {
  std::string result{};
  result.reserve(8);

  if (value == 0) {
    return std::string{"0"};
  }

  auto multiple_of_base = 1;

  while (value > 0) {
    auto index = value % CharacterSet::size();
    result.insert(result.begin(), CharacterSet::chars[index]);
    value /= CharacterSet::size();
    multiple_of_base *= CharacterSet::size();
  }

  return result;
}

template <typename CharacterSet, typename T = uint64_t>
constexpr auto reverse_encoding(const std::string_view &s) {
  auto multiple_of_base = T{1};
  auto result = T{0};
  auto base = CharacterSet::size();
  auto start = s.begin();

  auto str = std::string_view{start, s.end()};
  if (str.empty()) {
    return T{0};
  }

  for (auto i = str.size(); i-- > 0;) {
    auto c = str[i];
    auto index = CharacterSet::find(c);

    if (index == std::string::npos) {
      throw std::runtime_error("Invalid character in string");
    }

    result += index * multiple_of_base;
    multiple_of_base *= base;
  }
  return result;
}

template <typename CharacterSet, typename T = uint64_t> struct Encoding {
  constexpr static auto apply(T value) { return apply_encoding<CharacterSet>(value); }

  constexpr static auto reverse(const std::string_view &str) {
    return reverse_encoding<CharacterSet, T>(str);
  }
};

typedef Encoding<Base62Encoding> Base62;
typedef Encoding<HexEncoding> Hex;
typedef Encoding<BinaryEncoding> Binary;
typedef Encoding<DecimalEncoding> Decimal;

template <typename Encoding>
constexpr auto test(uint64_t value) {
  return Encoding::reverse(Encoding::apply(value)) == value;
}

static_assert(Decimal::apply(10) == "10");
static_assert(Decimal::apply(789789) == "789789");

static_assert(Decimal::reverse("0000010") == 10);

static_assert(Base62::apply(0x0000) == "0");
static_assert(Base62::apply(0x0001) == "1");
static_assert(Base62::apply(0x000A) == "A");
static_assert(Base62::apply(0x000f) == "F");
static_assert(Base62::reverse("F") == 0x000f);
static_assert(Base62::apply(Base62Encoding::size() + 1) == "11");
static_assert(Base62::apply(Base62Encoding::size() + 15) == "1F");

static_assert(Hex::apply(0xEF11) == "EF11");
static_assert(Hex::apply(0xdeadbeef) == "DEADBEEF");
static_assert(Hex::apply(0xadded) == "ADDED");
static_assert(Hex::apply(0xcadde) == "CADDE");

static_assert(Hex::apply(0xFFFFFF) == "FFFFFF");
static_assert(Hex::apply(0xFFFFFFFF) == "FFFFFFFF");
static_assert(Hex::apply(0xFFFFFFFFFF) == "FFFFFFFFFF");
static_assert(Hex::apply(0xFFFFFFFFFFFF) == "FFFFFFFFFFFF");
static_assert(Hex::apply(0xFFFFFFFFFFFFFF) == "FFFFFFFFFFFFFF");
static_assert(Hex::reverse("FFFFFFFFFF") == 0xFFFFFFFFFF);
static_assert(Hex::reverse("4A7BDF") == 0x4a7bdf);
static_assert(Decimal::reverse("6789") == 6789);

static_assert(test<Hex>(0xFFFFFFFFFFFFFFFF));
static_assert(test<Base62>(0xFFFFFFFFFFFFFFFF));
static_assert(test<Binary>(0b1010));
static_assert(Binary::apply(0b1010) == "1010");

