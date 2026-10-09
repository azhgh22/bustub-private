#ifndef LISTWRAPPER_AZ
#define LISTWRAPPER_AZ

#include <iostream>
#include <list>
#include <optional>
#include <set>
#include <unordered_map>

#include "common/config.h"
#include "common/macros.h"

namespace bustub {

#define timestamp size_t

template <typename KeyType>
struct ListWrapper {
 protected:
  std::list<KeyType> lru_;
  std::unordered_map<KeyType, typename std::list<KeyType>::iterator> map_;
  // std::set<std::pair<timestamp,frame_id_t>> evictables_;

 public:
  void Print();
  auto Contains(KeyType key) -> bool;
  auto Size() -> size_t;
  void Insert(KeyType);
  void Remove(KeyType key);  // removes elem from list
  void MoveForward(KeyType key);
  auto RemoveLru() -> std::optional<KeyType>;
};

}  // namespace bustub

#endif
