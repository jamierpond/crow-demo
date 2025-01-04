#include "crow.h"
#include <cstdint>
#include <thread>

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
constexpr auto MapSize = 1 << 8;
constexpr auto PSLBits = 5;
constexpr auto HashBits = 3;

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

  auto thread = std::thread([&] {
    std::cout << "Inserting " << id << " " << "https://www.google.com" << std::endl;
    auto json = crow::json::load("{\"link\":\"https://www.google.com\"}");
    auto link = std::string{json["link"].s()};
    std::lock_guard<std::mutex> lock{links_mutex};
    links.insert(std::pair{id, link});
  });

  thread.join();

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

    auto current_string = std::to_string(id);
    auto short_link = std::string{OUR_DOMAIN} + current_string;

    return short_link;
  });

  CROW_ROUTE(app, "/<uint>")
  ([&](const crow::request &req, uint64_t id) {
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
