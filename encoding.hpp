#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_set>

namespace char_sets {

template <auto Strings> struct CharacterSet {
  constexpr static std::string_view chars = Strings();
  constexpr static auto size() { return chars.size(); }
  constexpr static auto zero_char() { return chars[0]; }
  constexpr static auto find(char c) { return chars.find(c); }
  constexpr static auto contains(char c) {
    return chars.find(c) != std::string::npos;
  }

  static_assert(size() > 0, "Character set must not be empty");
  static_assert([](){
      std::unordered_set<char> set{};
      for (auto c : chars) { set.insert(c); }
      return set.size() == chars.size();
  }, "Character set must not contain duplicates");
};

namespace definitions {
constexpr auto Hex = [] { return "0123456789ABCDEF"; };
constexpr auto SmileyFaces = [] { return "):(3Dx"; };
constexpr auto Decimal = [] { return "0123456789"; };
constexpr auto Binary = [] { return "01"; };
constexpr auto Base62 = [] {
  return "0123456789"
         "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
         "abcdefghijklmnopqrstuvwxyz";
};
} // namespace definitions

typedef CharacterSet<definitions::Base62> Base62Encoding;
typedef CharacterSet<definitions::SmileyFaces> SmileyFacesEncoding;
typedef CharacterSet<definitions::Hex> HexEncoding;
typedef CharacterSet<definitions::Decimal> DecimalEncoding;
typedef CharacterSet<definitions::Binary> BinaryEncoding;

} // namespace char_sets

template <typename CharacterSet, typename T = uint64_t>
constexpr auto apply_encoding(T value) {
  std::string result{};
  result.reserve(8);

  constexpr auto zero = CharacterSet::zero_char();
  if (value == 0) { return std::string{zero}; }

  auto multiple_of_base = 1;
  while (value > 0) {
    auto index = value % CharacterSet::size();
    result.insert(result.begin(), CharacterSet::chars[index]);
    value /= CharacterSet::size();
    multiple_of_base *= CharacterSet::size();
  }

  return result;
}

template <bool enabled>
constexpr auto throw_if(auto condition, const std::string &message) {
  if constexpr (enabled) {
    if (condition) { throw std::runtime_error(message); }
  }
}

template <typename CharacterSet, typename T = uint64_t, bool check_overflow = false, bool check_valid_chars = false>
constexpr auto reverse_encoding(const std::string_view &str) {
  if (str.empty()) { return T{0}; }

  auto
     multiple_of_base = T{1},
     result = T{0},
     base = CharacterSet::size();

  for (auto i = str.size() - 1;;) {
    auto index = CharacterSet::find(str[i]);
    throw_if<check_valid_chars>(index == std::string::npos, "Invalid character in string");
    throw_if<check_overflow>(base > std::numeric_limits<T>::max() / multiple_of_base, "Overflow while decoding");
    result += index * multiple_of_base;
    if (i-- <= 0) { break; }
    multiple_of_base *= base;
  }

  return result;
}

template <typename CharacterSet, typename T = uint64_t> struct Encoder {
  typedef T type;
  constexpr static auto apply(T value) {
    return apply_encoding<CharacterSet>(value);
  }
  constexpr static auto reverse(const std::string_view &str) {
    return reverse_encoding<CharacterSet, T>(str);
  }
};

typedef Encoder<char_sets::Base62Encoding> Base62;
typedef Encoder<char_sets::SmileyFacesEncoding> SmileyFaces;
typedef Encoder<char_sets::HexEncoding> Hex;
typedef Encoder<char_sets::DecimalEncoding> Decimal;
typedef Encoder<char_sets::BinaryEncoding> Binary;
