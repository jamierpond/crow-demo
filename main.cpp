#include "./encoding.hpp"
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
