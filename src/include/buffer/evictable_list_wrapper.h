#ifndef EV_LISTWRAPPER_AZ
#define EV_LISTWRAPPER_AZ

#include <iostream>
#include <list>
#include <optional>
#include <set>
#include <unordered_map>
#include <utility>

#include "common/config.h"
#include "common/macros.h"

#include "buffer/list_wrapper.h"

namespace bustub {

#define timestamp size_t

struct KeyStatus {
  timestamp update_time_;
  bool is_ev_;
};

template <typename KeyType>
struct EVListWrapper : public ListWrapper<KeyType> {
 private:
  std::set<std::pair<timestamp, frame_id_t>> evictables_;
  // std::unordered_map<KeyType,size_t> ev_map_;
  std::unordered_map<KeyType, KeyStatus> ev_map_;
  timestamp update_time_{0};

 public:
  void Insert(KeyType) = delete;
  void Print();
  auto EvictableSize() -> size_t;
  void Insert(KeyType key, bool ev);
  void Remove(KeyType key);  // removes elem from list
  void MoveForward(KeyType key);
  auto RemoveLru() -> std::optional<KeyType>;
};

}  // namespace bustub

#endif
