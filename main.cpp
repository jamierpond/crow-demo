#include <cstdint>
#include <iostream>
#include <thread>
#include <unordered_map>

/**
 * Looks like this has been removed from the lib accidentally.
 * Adding here for visibility and convenience.
 * */
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


enum class ThreadPolicy { MainThread, WorkerThread };

template <typename Map, ThreadPolicy OnThread>
void do_thing() {
  Map links{};
  auto id = 0;

  auto perform_insert = [&] {
    links.insert(std::pair{id, "cool link"});
  };

  switch (OnThread) {
    case ThreadPolicy::MainThread:
      perform_insert();
      break;
    case ThreadPolicy::WorkerThread:
      auto t = std::thread{perform_insert};
      t.join();
      break;
  }

  auto link = links.find(id);
  std::cout << "Link: " << link->second << std::endl;
}

int main() {
  // totally fine!
  do_thing<std::unordered_map<FromType, ToType>, ThreadPolicy::MainThread>();
  do_thing<std::unordered_map<FromType, ToType>, ThreadPolicy::WorkerThread>();
  do_thing<ZooMap<FromType, ToType>, ThreadPolicy::MainThread>();

  // explodes :(
  do_thing<ZooMap<FromType, ToType>, ThreadPolicy::WorkerThread>();
}

