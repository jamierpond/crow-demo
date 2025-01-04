#include "crow.h"
#include <cstdint>

// This is placed ahead of other bare manipulation functions to reduce integer
// promotion weirdness in the constant creation of SWAR and related.
template <int NBits, typename T> constexpr auto leastNBitsMask() {
  // It was integer promotion screwing this up at equal sizes all along, special
  // casing unblocks.
  constexpr auto type_bits = sizeof(T) * 8;
  if constexpr (NBits < type_bits) {
    return (T{1} << NBits) - 1;
  } else if constexpr (type_bits < 32) {
    return (T{1} << NBits) - 1;
  } else if constexpr (type_bits >= 32) {
    return ~T{0};
  }
}

#include <zoo/map/RobinHood.h>

using FromType = uint64_t;
using ToType = std::string;
constexpr auto MapSize = 1 << 15;
constexpr auto PSLBits = 5;
constexpr auto HashBits = 3;

constexpr auto to_hex_string(uint64_t value) {
  std::string result;
  result.reserve(16);
  for (int i = 0; i < 16; i++) {
    auto nibble = (value >> (60 - i * 4)) & 0xF;
    if (nibble < 10) {
      result.push_back('0' + nibble);
    } else {
      result.push_back('a' + nibble - 10);
    }
  }

  // trim leading zeros
  while (result.size() > 1 && result[0] == '0') {
    result.erase(0, 1);
  }

  return result;
}

static_assert(to_hex_string(0x1234567890abcdef) == "1234567890abcdef");
static_assert(to_hex_string(0x0000000000000001) == "1");

constexpr auto from_hex_string(const std::string_view &str) {
  uint64_t result = 0;
  auto size = str.size();
  if (size > 16) {
    throw std::runtime_error("Invalid hex string");
  }

  constexpr auto is_number = [](char c) { return c >= '0' && c <= '9'; };
  constexpr auto is_lower_case_letter = [](char c) {
    return c >= 'a' && c <= 'f';
  };
  constexpr auto is_upper_case_letter = [](char c) {
    return c >= 'A' && c <= 'F';
  };

  for (int i = 0; i < size; i++) {
    auto c = str[i];
    uint64_t nibble;

    if (is_number(c)) {
      nibble = c - '0';
    } else if (is_lower_case_letter(c)) {
      nibble = c - 'a' + 10;
    } else if (is_upper_case_letter(c)) {
      nibble = c - 'A' + 10;
    } else {
      throw std::runtime_error("Invalid hex string");
    }

    result = (result << 4) | nibble;
  }
  return result;
}

static_assert(from_hex_string("1234567890abcdef") == 0x1234567890abcdef);
static_assert(from_hex_string("0000000000000001") == 0x0000000000000001);
static_assert(from_hex_string("1") == 0x0000000000000001);
static_assert(from_hex_string("f2") == 0x00000000000000f2);
static_assert(from_hex_string("F2") == 0x00000000000000f2);

template <typename From, typename To>
using ZooMap =
    zoo::rh::RH_Frontend_WithSkarupkeTail<From, To, MapSize, PSLBits, HashBits>;

constexpr std::string_view OUR_DOMAIN = "http://localhost:8080/";

constexpr auto is_link_valid(const std::string &link) {
  // todo implement a proper link validation, maybe with checking also
  // if the link returns application/html content type, with a 200 status code
  // and less than 2s timeout
  return true;
}

int main() {
  crow::SimpleApp app;
  std::atomic<uint64_t> current_id = 0;
  ZooMap<uint64_t, std::string> links{};
  std::mutex links_mutex;

  auto id = current_id.load();

  std::cout << "Starting server" << std::endl;

  CROW_ROUTE(app, "/insert")
  ([&](const crow::request &req) {
    auto id = current_id.load();
    current_id++;

    if (id >= MapSize) {
      // todo real error handling
      return std::string{"Map is full"};
    }

    auto json = crow::json::load(req.body);
    auto link = std::string{json["link"].s()};

    if (!is_link_valid(link)) {
      // todo real error handling
      return std::string{"Invalid link"};
    }

    std::lock_guard<std::mutex> lock{links_mutex};
    std::cout << "Inserting " << id << " " << link << std::endl;
    links.insert(std::pair{id, link});

    auto current_string = to_hex_string(id);
    auto short_link = std::string{OUR_DOMAIN} + current_string + "\n";

    return short_link;
  });

  CROW_ROUTE(app, "/")
  ([&]() { return "hello"; });

  CROW_ROUTE(app, "/<string>")
  ([&](const crow::request &req, const std::string& hex) {
    auto id = from_hex_string(hex);
    std::lock_guard<std::mutex> lock{links_mutex};
    auto it = links.find(id);

    if (it == links.end()) {
      // todo real error handling
      return std::string{"Link not found"};
    }

    return it->second;
  });

  app.port(8080).run();
}
