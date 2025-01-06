#include "./home.hpp"
#include "crow.h"
#include <cstdint>
#include <regex>
#include <shared_mutex>
#include <string>

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

// TODO ADD SERIALIZE/DESERIALIZE OF MAP

using FromType = uint32_t;
using ToType = std::string;
constexpr auto MapSize = 1 << 23;
constexpr auto PSLBits = 5;
constexpr auto HashBits = 3;
using Json = crow::json::wvalue;

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

template <typename Encoding>
constexpr auto apply_encoding(uint64_t value) {
  std::string result{};
  result.reserve(8);

  if (value == 0) {
    return std::string{"0"};
  }

  auto multiple_of_base = 1;

  // todo make associative iteration ?
  while (value > 0) {
    auto index = value % Encoding::size();
    auto value_now = value * multiple_of_base;

    result.push_back(Encoding::chars[index]);

    value /= Encoding::size();
    multiple_of_base *= Encoding::size();
  }

  return result;
}

template <typename Encoding>
constexpr auto reverse_encoding(const std::string_view& str) {
  auto multiple_of_base = 1;
  auto result = 0;
  auto base = Encoding::size();

  for (int i = 0; i < str.size(); i++) {
    auto c = str[i];
    auto index = Encoding::chars.find(c);
    if (index == std::string::npos) {
      throw std::runtime_error("Invalid character in string");
    }

    result += index * multiple_of_base;
    multiple_of_base *= base;
  }

  return result;
}

static_assert(apply_encoding<Base62Encoding>(0x0000) == "0");
static_assert(apply_encoding<Base62Encoding>(0x0001) == "1");
static_assert(apply_encoding<Base62Encoding>(0x000A) == "A");
static_assert(apply_encoding<Base62Encoding>(0x000f) == "F");
static_assert(reverse_encoding<Base62Encoding>("F") == 0x000f);
static_assert(apply_encoding<Base62Encoding>(Base62Encoding::size()) == "01");
static_assert(apply_encoding<Base62Encoding>(Base62Encoding::size() + 1) == "11");
static_assert(apply_encoding<Base62Encoding>(Base62Encoding::size() + 15) == "F1");
static_assert(reverse_encoding<Base62Encoding>("01") == Base62Encoding::size());
static_assert(reverse_encoding<Base62Encoding>("F1") == Base62Encoding::size() + 15);


template <typename From, typename To>
using ZooMap =
    zoo::rh::RH_Frontend_WithSkarupkeTail<From, To, MapSize, PSLBits, HashBits>;

constexpr std::string_view OUR_DOMAIN = "https://s.pond.audio/";

auto is_link_valid(const std::string &link) {
  static const std::regex url_regex(R"(^(https?:\/\/)?)" // Protocol (optional)
                                    R"([\w\-]+(\.[\w\-]+)+)" // Domain
                                    R"([^\s]*$)");           // Rest of URL

  return std::regex_match(link, url_regex);
}

int main() {
  crow::SimpleApp app{};
  std::atomic<uint64_t> current_id = 0;
  using Map = ZooMap<FromType, ToType>;

  auto links = std::unique_ptr<Map>{new Map{}};
  auto links_mutex = std::shared_mutex{};
  auto id = current_id.load();

  CROW_ROUTE(app, "/insert")
      .methods("POST"_method)([&](const crow::request &req) {
        auto id = current_id.load();
        current_id++;

        if (id >= MapSize) {
          return crow::response{500, "Server is full"};
        }

        auto json = crow::json::load(req.body);
        if (!json) {
          return crow::response{400, "Invalid JSON"};
        }
        if (!json.has("link")) {
          return crow::response{400, "Missing link"};
        }
        auto link = std::string{json["link"].s()};

        if (!is_link_valid(link)) {
          return crow::response{400, "Invalid link"};
        }

        std::lock_guard<std::shared_mutex> lock{links_mutex};
        links->insert(std::pair{id, link});

        auto current_string = apply_encoding<Base62Encoding>(id);
        auto short_link = std::string{OUR_DOMAIN} + current_string;

        return crow::response{Json{{"short_link", short_link}}};
      });

  CROW_ROUTE(app, "/")
  ([&]() { return HOME_TEMPLATE_HTML; });

  CROW_ROUTE(app, "/<string>")
  ([&](const crow::request &req, const std::string &hex) {
    try {
      auto id = reverse_encoding<Base62Encoding>(hex);
      std::shared_lock<std::shared_mutex> lock{links_mutex};
      auto it = links->find(id);

      if (it == links->end()) {
        return crow::response{404, "Not found"};
      }

      auto link = it->second;

      // redirect to the original link
      auto res = crow::response{};
      res.redirect(link);
      return res;
    } catch (const std::runtime_error &e) {
      return crow::response{500, e.what()};
    }
  });

  app.port(8080).run();
}
