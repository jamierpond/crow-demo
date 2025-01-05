
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

constexpr auto HOME_TEMPLATE_HTML = R"(
<!DOCTYPE html>
<html>
  <head>
    <title>Shorten your link</title>
  </head>
  <body>
    <h1>Shorten your link</h1>
    <form id="shortenForm">
      <label for="link">Link:</label>
      <input type="text" id="link" name="link" required>
      <input type="submit" value="Submit">
    </form>
    <div id="result" hidden>
      <h3>Your shortened link:</h3>
      <a id="shortenedLink"></a>
    </div>

    <script>
      document.getElementById('shortenForm').addEventListener('submit', async (event) => {
        event.preventDefault();

        try {
          const response = await fetch('/insert', {
            method: 'POST',
            headers: {'Content-Type': 'application/json'},
            body: JSON.stringify({
              "link": document.getElementById('link').value
            })
          });

          if (!response.ok) {
            console.error('Error:', response);
            throw new Error(response.data);
          }
          const data = await response.json();

          document.getElementById('shortenedLink').textContent = data.short_link;
          document.getElementById('result').hidden = false;
        } catch (error) {
          console.error('Error:', error);
          alert('An error occurred while shortening the link' + error.message);
        }
      });
    </script>
  </body>
</html>
)";

#include <zoo/map/RobinHood.h>

using FromType = uint32_t;
using ToType = std::string;
constexpr auto MapSize = 1 << 23;
constexpr auto PSLBits = 5;
constexpr auto HashBits = 3;
using Json = crow::json::wvalue;

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
    throw std::runtime_error("Invalid hex string: " + std::string{str});
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

        auto current_string = to_hex_string(id);
        auto short_link = std::string{OUR_DOMAIN} + current_string;

        return crow::response{Json{{"short_link", short_link}}};
      });

  CROW_ROUTE(app, "/")
  ([&]() { return HOME_TEMPLATE_HTML; });

  CROW_ROUTE(app, "/<string>")
  ([&](const crow::request &req, const std::string &hex) {
    try {
      auto id = from_hex_string(hex);
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
