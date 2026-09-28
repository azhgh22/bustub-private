#ifndef EV_LISTWRAPPER_AZ
#define EV_LISTWRAPPER_AZ

#include <list>
#include <optional>
#include <unordered_map>
#include <iostream>
#include <set>

#include "common/config.h"
#include "common/macros.h"

#include "buffer/list_wrapper.h"

namespace bustub
{

#define timestamp size_t

struct KeyStatus
{
  timestamp update_time_;
  bool is_ev;
};


template <typename KeyType>
struct EVListWrapper : public ListWrapper<KeyType>{
private:
  std::set<std::pair<timestamp,frame_id_t>> evictables_;
  // std::unordered_map<KeyType,size_t> ev_map_;
  std::unordered_map<KeyType,KeyStatus> ev_map_;
  timestamp update_time_{0};

public:
  void Insert(KeyType) = delete;
  void Print();
  size_t EvictableSize();
  void Insert(KeyType key, bool ev);
  void Remove(KeyType key); // removes elem from list
  void MoveForward(KeyType key);
  std::optional<KeyType> RemoveLru();
};

}


#endif