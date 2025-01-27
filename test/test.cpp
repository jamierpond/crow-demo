#include <cstdint>
#define CATCH_CONFIG_MAIN
#include <catch2/catch_all.hpp>
#include "../encoding.hpp"

template <typename Encoding, typename FromType>
constexpr auto test(FromType value) {
  return Encoding::decode(Encoding::template encode<FromType>(value)) == value;
}

template<typename Encoding>
constexpr bool fuzz_test(int num_iterations = 100) {
  using T = typename Encoding::type;
  auto
     value = T{0},
     seed = value;

  for (T i = 0; i < num_iterations; i++) {
    value = seed;
    seed = value * 0xdeadbeef + 0x1337 * value;
    value = value & 0xFFFFFFFF;
    if (!test<Encoding>(value)) {
      return false;
    }
    if (!test<Encoding>(i)) {
      return false;
    }
  }

  return true;
}

static_assert(Binary::encode(177349680ll) == "1010100100100010010000110000");
static_assert(Binary::encode(1529429974640006688) == "1010100111001101000000111010101101011010000001001011000100000");
static_assert(Binary::decode("1010100100100010010000110000") == 177349680);
static_assert(Decimal::encode(10ll) == "10");
static_assert(Decimal::encode(789789ll) == "789789");
static_assert(Decimal::decode("0000010") == 10);
static_assert(Decimal::decode("6789") == 6789);
static_assert(Base62::encode(0x0000ll) == "0");
static_assert(Base62::encode(0x0001ll) == "1");
static_assert(Base62::encode(0x000All) == "A");
static_assert(Base62::encode(0x000fll) == "F");
static_assert(Base62::decode("F") == 0x000f);
static_assert(Hex::encode(0xEF11ll) == "EF11");
static_assert(Hex::encode(0xdeadbeefll) == "DEADBEEF");
static_assert(Hex::encode(0xaddedll) == "ADDED");
static_assert(Hex::encode(0xcaddell) == "CADDE");
static_assert(Hex::encode(0xFFFFFFll) == "FFFFFF");
static_assert(Hex::encode(0xFFFFFFFFll) == "FFFFFFFF");
static_assert(Hex::encode(0xFFFFFFFFFF) == "FFFFFFFFFF");
static_assert(Hex::encode(0xFFFFFFFFFFFF) == "FFFFFFFFFFFF");
static_assert(Hex::encode(0xFFFFFFFFFFFFFF) == "FFFFFFFFFFFFFF");
static_assert(Hex::decode("FFFFFFFFFF") == 0xFFFFFFFFFF);
static_assert(Hex::decode("4A7BDF") == 0x4a7bdf);
static_assert(test<Hex, uint64_t>(0xFFFFFFFFFFFFFFF));
static_assert(Binary::encode(0b1010ll) == "1010");



// static_assert(test<Base62>(0xFFFFFFFFFFFFFF));
// static_assert(test<Binary>(0b1010));

static_assert(SmileyFaces::encode(6ll) == ":)");
static_assert(SmileyFaces::encode(9ll) == ":3");
static_assert(SmileyFaces::encode(34l) == "xD");
static_assert(SmileyFaces::encode(33l) == "x3");

static_assert(fuzz_test<Base62>(15));
static_assert(fuzz_test<Base62>(15));
static_assert(fuzz_test<Binary>(15));

static_assert(Binary::decode("0000000000000000000000000000000000000000000") == 0);
static_assert(Binary::decode("0000000000000000000000000000000000000000001") == 1);
static_assert(Hex::decode   ("00000000000000F") == 15);

TEST_CASE("fuzz test", "[fuzz]") {
  auto n = 1'000;
  REQUIRE(fuzz_test<Base62>(n));
  REQUIRE(fuzz_test<Hex>(n));
  REQUIRE(fuzz_test<Decimal>(n));
  REQUIRE(fuzz_test<Binary>(n));
  REQUIRE(fuzz_test<SmileyFaces>(n));
};

#include <random>
#include <iostream>

template <typename FromType, typename Encoder>
auto test_foo() {
  auto rng = std::mt19937_64{std::random_device{}()};
  auto range_lower = std::numeric_limits<FromType>::min();
  auto range_upper = std::numeric_limits<FromType>::max();
  for (auto i = 0; i < 1'000'000; i++) {
    auto value = std::generate_canonical<FromType, 64>(rng);
    auto encoded = Encoder:: template encode<FromType>(value);
    auto decoded = Encoder:: template decode<FromType>(encoded);
    std::cout << "value: " << value << " encoded: " << encoded << " decoded: " << decoded << std::endl;
    REQUIRE(value == decoded);
  }
}

// maths works!
static_assert(Hex::encode<double>(4.24200000000000017053025658242E1) == "404535C28F5C28F6");

TEST_CASE("types", "[fuzz]") {
  test_foo<float, Encoder<char_sets::HexEncoding, uint32_t>>();
  test_foo<double, Encoder<char_sets::HexEncoding, uint64_t>>();
}
