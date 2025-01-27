#include <limits>
#include <string>
#define CATCH_CONFIG_MAIN
#include <catch2/catch_all.hpp>
#include "../encoding.hpp"

template <typename Encoding>
constexpr auto test(uint64_t value) {
  return Encoding::reverse(Encoding::apply(value)) == value;
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

static_assert(Binary::apply(177349680) == "1010100100100010010000110000");
static_assert(Binary::apply(1529429974640006688) == "1010100111001101000000111010101101011010000001001011000100000");
static_assert(Binary::reverse("1010100100100010010000110000") == 177349680);
static_assert(Decimal::apply(10) == "10");
static_assert(Decimal::apply(789789) == "789789");
static_assert(Decimal::reverse("0000010") == 10);
static_assert(Decimal::reverse("6789") == 6789);
static_assert(Base62::apply(0x0000) == "0");
static_assert(Base62::apply(0x0001) == "1");
static_assert(Base62::apply(0x000A) == "A");
static_assert(Base62::apply(0x000f) == "F");
static_assert(Base62::reverse("F") == 0x000f);
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
static_assert(test<Hex>(0xFFFFFFFFFFFFFFF));
static_assert(test<Base62>(0xFFFFFFFFFFFFFF));
static_assert(test<Binary>(0b1010));
static_assert(Binary::apply(0b1010) == "1010");

static_assert(SmileyFaces::apply(6) == ":)");
static_assert(SmileyFaces::apply(9) == ":3");
static_assert(SmileyFaces::apply(34) == "xD");
static_assert(SmileyFaces::apply(33) == "x3");

static_assert(fuzz_test<Base62>(15));
static_assert(fuzz_test<Base62>(15));
static_assert(fuzz_test<Binary>(15));

static_assert(Binary::reverse("0000000000000000000000000000000000000000000") == 0);
static_assert(Binary::reverse("0000000000000000000000000000000000000000001") == 1);
static_assert(Hex::reverse   ("00000000000000F") == 15);

TEST_CASE("fuzz test", "[fuzz]") {
  REQUIRE(fuzz_test<Base62>(1'000'000));
  REQUIRE(fuzz_test<Hex>(1'000'000));
  REQUIRE(fuzz_test<Decimal>(1'000'000));
  REQUIRE(fuzz_test<Binary>(1'000'000));
  REQUIRE(fuzz_test<SmileyFaces>(1'000'000));
};


