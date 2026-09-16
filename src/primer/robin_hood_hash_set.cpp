//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// robin_hood_hash_set.cpp
//
// Identification: src/primer/robin_hood_hash_set.cpp
//
// Copyright (c) 2015-2026, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "primer/robin_hood_hash_set.h"

#include <cstdint>
#include <string>

namespace bustub {

template<typename KeyType>
void PrintState(const std::vector<HashTableSlot<KeyType>>& vec){
  for(const HashTableSlot<KeyType>& s: vec){
    std::cout<< s.state_<< " ";
  }
  std::cout<<std::endl;
}

template <typename KeyType, typename Hash, typename KeyEqual>
RobinHoodHashSet<KeyType, Hash, KeyEqual>::RobinHoodHashSet(size_t capacity, Hash hasher, KeyEqual key_equal) {
  /** @TODO(student) Initialize the fixed-capacity bucket array and store the supplied functors. */
  if(capacity<=0){
    throw std::invalid_argument("RobinHoodHashSet: capacity should be >0");
  }
  
  this->capacity_ = capacity;
  this->hash_fn_ = hasher;
  this->key_equal_fn_ = key_equal;
  this->channel_capacity_ = capacity*3;

  this->hash_table_ = std::vector<HashTableSlot<KeyType>>(capacity);
  this->size_.store(0);
  this->free_slots_.store(0);
  this->chanels_ = std::vector<std::recursive_mutex>(channel_capacity_);
}

template <typename KeyType, typename Hash, typename KeyEqual>
RobinHoodHashSet<KeyType, Hash, KeyEqual>::RobinHoodHashSet(RobinHoodHashSet &&other) noexcept {
  /** @TODO(student) Move the table's storage, metadata, functors, and synchronization state safely. */
  // static_cast<void>(other);
  this->capacity_ = other.capacity_;
  this->hash_fn_ = std::move(other.hash_fn_);
  this->key_equal_fn_ = std::move(other.key_equal_fn_);
  this->hash_table_ = std::move(other.hash_table_);
  this->size_ = other.size_.load();
  this->free_slots_ = other.free_slots_.load();
  this->probe_store_ = std::move(other.probe_store_);
  this->chanels_ = std::move(other.chanels_);
  this->channel_capacity_ = other.channel_capacity_;

  other.capacity_ = 0;
  other.size_ = 0;
}

template <typename KeyType, typename Hash, typename KeyEqual>
auto RobinHoodHashSet<KeyType, Hash, KeyEqual>::operator=(RobinHoodHashSet &&other) noexcept -> RobinHoodHashSet & {
  /** @TODO(student) Implement move assignment, including self-assignment protection. */
  // static_cast<void>(other);
  this->capacity_ = other.capacity_;
  this->hash_fn_ = std::move(other.hash_fn_);
  this->key_equal_fn_ = std::move(other.key_equal_fn_);
  this->hash_table_ = std::move(other.hash_table_);
  this->size_ = other.size_.load();
  this->free_slots_ = other.free_slots_.load();
  this->probe_store_ = std::move(other.probe_store_);
  this->chanels_ = std::move(other.chanels_);
  this->channel_capacity_ = other.channel_capacity_;
  other.capacity_ = 0;
  other.size_ = 0;
  return *this;
}

template <typename KeyType, typename Hash, typename KeyEqual>
auto RobinHoodHashSet<KeyType, Hash, KeyEqual>::Insert(const KeyType &key) -> bool {
  /** @TODO(student) Insert with linear probing, Robin Hood displacement, and safe concurrent access. */  

  
  // std::lock_guard loc1k(global_lock_);
  
  size_t chanel_idx = this->hash_fn_(key) % this->channel_capacity_;
  std::lock_guard lock(chanels_[chanel_idx]);
  // std::cout<<"Sf: "<<chanel_idx<<std::endl;
  
  if(Contains(key)) return true;
  // if do not contains what if someone inserts same kay here

/* 
  atomic size++, and compare to capcaicy
*/
  // size_t current_size = size_.load();

  // while (current_size<capacity_)
  // {
  //   if(size_.compare_exchange_strong(current_size,current_size+1)){
  //     break;
  //   }
  // }

  // if(current_size>capacity_) return false;
  
  
  {
    std::lock_guard lock(global_lock_);
    if(size_.load()==capacity_) return false;
    size_++;
  }
  BUSTUB_ENSURE(size_.load()<=capacity_,"size must be less/eq than capacity");

  size_t home_bucket = this->hash_fn_(key) % this->capacity_;
  size_t cur_probe = 0;
  KeyType cur_key = key;

  for(size_t i = home_bucket;i!=home_bucket+capacity_; i++){
    HashTableSlot<KeyType>& slot = (hash_table_[i%capacity_]);
    std::lock_guard<std::mutex> g(slot.lock_);
    if(slot.state_ == SlotState::OCCUPIED){
      if(key_equal_fn_(key,slot.key_)) return true;

      if(cur_probe>slot.probe_){
        // std::multiset<size_t>::iterator it = probe_store_.find(slot.probe_);
        // if(it!=probe_store_.end()){
        //   probe_store_.erase(it);
        // }
        // probe_store_.insert(cur_probe);
        std::swap(cur_probe,slot.probe_);
        std::swap(cur_key,slot.key_);
      }
    }else{
      // probe_store_.insert(cur_probe);
      slot.probe_ = cur_probe;
      slot.key_ = cur_key;
      slot.state_ = SlotState::OCCUPIED;
      // std::swap(cur_probe,slot.probe_);
      // std::swap(cur_key,slot.key_);
      return true;
    }
    cur_probe++;
  }
  std::cout<<"Probe_store size: "<<probe_store_.size()<<std::endl;
  return false;
}

template <typename KeyType, typename Hash, typename KeyEqual>
auto RobinHoodHashSet<KeyType, Hash, KeyEqual>::Remove(const KeyType &key) -> bool {
  /** @TODO(student) Mark an occupied matching slot as a tombstone without breaking concurrent operations. */
  
  // size_t chanel_idx = this->hash_fn_(key) % this->channel_capacity_;
  // std::lock_guard lock(chanels_[chanel_idx]);
  // std::lock_guard lock(global_lock_);
  // size_t b = GetBucket(key);

  
  size_t home_bucket = this->hash_fn_(key) % this->capacity_;
  for(size_t i = home_bucket;i!=home_bucket+capacity_; i++){
    HashTableSlot<KeyType>& slot = (hash_table_[i%capacity_]);
    std::lock_guard<std::mutex> g(slot.lock_);
    if(slot.state_ == SlotState::EMPTY) return false;
    // if(slot.state_ != SlotState::OCCUPIED) continue;
    if(slot.state_ == SlotState::OCCUPIED && key_equal_fn_(slot.key_,key)){
      // std::multiset<size_t>::iterator it = probe_store_.find(slot.probe_);
      // probe_store_.erase(it);
      slot.state_ = SlotState::TOMBSTONE;
      size_--;
      return true;
    }
  }
  return false;
}

template <typename KeyType, typename Hash, typename KeyEqual>
void RobinHoodHashSet<KeyType, Hash, KeyEqual>::Clear() {
  /** @TODO(student) Remove occupied entries and tombstones while preserving capacity. */
  // std::lock_guard lock(global_lock_);
  for(size_t i = 0;i!=capacity_; i++){
    HashTableSlot<KeyType>& slot = (hash_table_[i]);
    std::lock_guard<std::mutex> g(slot.lock_);
    slot.state_ = SlotState::EMPTY;
  }
  probe_store_.clear();
  size_ = 0;
}

template <typename KeyType, typename Hash, typename KeyEqual>
auto RobinHoodHashSet<KeyType, Hash, KeyEqual>::Contains(const KeyType &key) const -> bool {
  /** @TODO(student) Probe through tombstones and stop only where correctness permits. */
  
  size_t b = GetBucket(key);
  // std::cout<<"contains: "<<key<<" "<<b<<std::endl;
  // PrintState(hash_table_);
  return b!=capacity_;
}

template <typename KeyType, typename Hash, typename KeyEqual>
auto RobinHoodHashSet<KeyType, Hash, KeyEqual>::GetBucket(const KeyType &key) const -> size_t {
  /** @TODO(student) Return the physical bucket holding key, or BucketCount() when absent. */
  std::lock_guard lock(global_lock_);
  
  // size_t chanel_idx = this->hash_fn_(key) % this->channel_capacity_;
  // std::lock_guard lock(chanels_[chanel_idx]);
  
  size_t home_bucket = this->hash_fn_(key) % this->capacity_;
  for(size_t i = home_bucket;i!=home_bucket+capacity_; i++){
    const HashTableSlot<KeyType>& slot = (hash_table_[i%capacity_]);
    std::lock_guard<std::mutex> g(slot.lock_);
    if(slot.state_ == SlotState::EMPTY) return capacity_;
    // if(slot.state_ != SlotState::OCCUPIED) continue;
    if(slot.state_ == SlotState::OCCUPIED && key_equal_fn_(slot.key_,key)) return i%capacity_;
  }
  return capacity_;
}

template <typename KeyType, typename Hash, typename KeyEqual>
auto RobinHoodHashSet<KeyType, Hash, KeyEqual>::Size() const -> size_t {
  /** @TODO(student) Return the number of occupied entries safely. */
  return this->size_.load();
}

template <typename KeyType, typename Hash, typename KeyEqual>
auto RobinHoodHashSet<KeyType, Hash, KeyEqual>::Capacity() const -> size_t {
  /** @TODO(student) Return the fixed capacity safely. */
  return capacity_;
}

template <typename KeyType, typename Hash, typename KeyEqual>
auto RobinHoodHashSet<KeyType, Hash, KeyEqual>::LoadFactor() const -> double {
  /** @TODO(student) Return Size() / Capacity(), including moved-from handling. */
  return static_cast<double>(Size())/static_cast<double>(capacity_);
}

template <typename KeyType, typename Hash, typename KeyEqual>
auto RobinHoodHashSet<KeyType, Hash, KeyEqual>::MaxProbeDistance() const -> size_t {
  /** @TODO(student) Return the greatest probe distance among live entries. */
  // std::lock_guard lock(global_lock_);
  // if(probe_store_.empty()) return 0;
  // return *probe_store_.rbegin();
  return 0;
}

template class RobinHoodHashSet<int>;
template class RobinHoodHashSet<int64_t>;
template class RobinHoodHashSet<std::string>;

}  // namespace bustub
