//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// count_min_sketch.cpp
//
// Identification: src/primer/count_min_sketch.cpp
//
// Copyright (c) 2015-2025, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "primer/count_min_sketch.h"

#include <stdexcept>
#include <string>

namespace bustub {

/**
 * Constructor for the count-min sketch.
 *
 * @param width The width of the sketch matrix_.
 * @param depth The depth of the sketch matrix_.
 * @throws std::invalid_argument if width or depth are zero.
 */
template <typename KeyType>
CountMinSketch<KeyType>::CountMinSketch(uint32_t width, uint32_t depth) : width_(width), depth_(depth) {
  /** @TODO(student) Implement this function! */
  if (width <= 0 || depth <= 0) {
    throw std::invalid_argument("Width and depth must be greater than zero");
  }

  for (size_t i = 0; i < depth; i++) {
    this->matrix_.emplace_back(std::vector<std::atomic<uint32_t>>(width));
    for (size_t j = 0; j < width; j++) {
      this->matrix_[i][j].store(0, std::memory_order_relaxed);
    }
  }

  /** @spring2026 PLEASE DO NOT MODIFY THE FOLLOWING */
  // Initialize seeded hash functions
  hash_functions_.reserve(depth_);
  for (size_t i = 0; i < depth_; i++) {
    hash_functions_.push_back(this->HashFunction(i));
  }
}

template <typename KeyType>
CountMinSketch<KeyType>::CountMinSketch(CountMinSketch &&other) noexcept : width_(other.width_), depth_(other.depth_) {
  /** @TODO(student) Implement this function! */
  this->matrix_ = std::move(other.matrix_);
  this->hash_functions_ = std::move(other.hash_functions_);
}

template <typename KeyType>
auto CountMinSketch<KeyType>::operator=(CountMinSketch &&other) noexcept -> CountMinSketch & {
  /** @TODO(student) Implement this function! */
  this->width_ = other.width_;
  this->depth_ = other.depth_;
  this->matrix_ = std::move(other.matrix_);
  this->hash_functions_ = std::move(other.hash_functions_);
  return *this;
}

template <typename KeyType>
void CountMinSketch<KeyType>::Insert(const KeyType &item) {
  /** @TODO(student) Implement this function! */
  for (size_t i = 0; i < this->depth_; i++) {
    size_t idx = this->hash_functions_[i](item);
    this->matrix_[i][idx]++;
  }
}

template <typename KeyType>
void CountMinSketch<KeyType>::Merge(const CountMinSketch<KeyType> &other) {
  if (width_ != other.width_ || depth_ != other.depth_) {
    throw std::invalid_argument("Incompatible CountMinSketch dimensions for merge.");
  }
  /** @TODO(student) Implement this function! */

  for (size_t i = 0; i < depth_; i++) {
    for (size_t j = 0; j < width_; j++) {
      uint32_t cur_this = matrix_[i][j].load();
      uint32_t cur_other = other.matrix_[i][j].load();
      matrix_[i][j].store(cur_this + cur_other);
    }
  }
}

template <typename KeyType>
auto CountMinSketch<KeyType>::Count(const KeyType &item) const -> uint32_t {
  uint32_t min_count = UINT32_MAX;
  for (size_t i = 0; i < this->depth_; i++) {
    size_t idx = this->hash_functions_[i](item);
    min_count = std::min(min_count, this->matrix_[i][idx].load());
  }
  return min_count;
}

template <typename KeyType>
void CountMinSketch<KeyType>::Clear() {
  /** @TODO(student) Implement this function! */
  for (size_t i = 0; i < depth_; i++) {
    for (size_t j = 0; j < width_; j++) {
      matrix_[i][j].store(0);
    }
  }
}

template <typename KeyType>
auto CountMinSketch<KeyType>::TopK(uint16_t k, const std::vector<KeyType> &candidates)
    -> std::vector<std::pair<KeyType, uint32_t>> {
  /** @TODO(student) Implement this function! */
  std::vector<std::pair<uint32_t, KeyType>> temp(candidates.size());

  k = std::min(k, static_cast<uint16_t>(candidates.size()));
  std::vector<std::pair<KeyType, uint32_t>> ans;

  for (size_t i = 0; i < temp.size(); i++) {
    temp[i] = {this->Count(candidates[i]), candidates[i]};
  }

  std::sort(temp.begin(), temp.end());

  for (size_t i = 1; i <= k; i++) {
    ans.push_back({temp[candidates.size() - i].second, temp[candidates.size() - i].first});
  }

  return ans;
}

// Explicit instantiations for all types used in tests
template class CountMinSketch<std::string>;
template class CountMinSketch<int64_t>;  // For int64_t tests
template class CountMinSketch<int>;      // This covers both int and int32_t
}  // namespace bustub
