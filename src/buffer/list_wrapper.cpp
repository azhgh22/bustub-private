#include "buffer/list_wrapper.h"

namespace bustub {

// template <typename KeyType>
// void ListWrapper<KeyType>::Print(){
//   std::cout<<"LRU size: "<<lru_.size()<<" Lru EvictableSize: "<<lru_evictable_.size()<<std::endl;
// 	for (auto it = lru_.rbegin(); it != lru_.rend(); ++it) {
// 		std::cout<<(*it)<<" ";
// 	}
// 	std::cout<<std::endl;

// 	for (auto it = lru_evictable_.rbegin(); it != lru_evictable_.rend(); ++it) {
// 		std::cout<<(*it)<<" ";
// 	}
// 	std::cout<<std::endl;
// }

template <typename KeyType>
auto ListWrapper<KeyType>::Contains(KeyType key) -> bool {
  return map_.find(key) != map_.end();
}

template <typename KeyType>
auto ListWrapper<KeyType>::Size() -> size_t {
  return lru_.size();
}

template <typename KeyType>
void ListWrapper<KeyType>::Insert(KeyType key) {
  lru_.push_front(key);
  map_[key] = lru_.begin();
}

template <typename KeyType>
void ListWrapper<KeyType>::Remove(KeyType key) {
  if (!Contains(key)) {
    return;
  }
  auto it = map_[key];
  lru_.erase(it);
  map_.erase(key);
}

template <typename KeyType>
void ListWrapper<KeyType>::MoveForward(KeyType key) {
  Remove(key);
  Insert(key);
}

template <typename KeyType>
auto ListWrapper<KeyType>::RemoveLru() -> std::optional<KeyType> {
  if (Size() > 0) [[likely]] {
    KeyType key = (lru_.back());
    Remove(key);
    return key;
  }

  return std::nullopt;
}

template class ListWrapper<frame_id_t>;

}  // namespace bustub
