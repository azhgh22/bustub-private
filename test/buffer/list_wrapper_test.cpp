#include "buffer/list_wrapper.h"

#include "gtest/gtest.h"

namespace bustub {

TEST(ListWrapperTest, SizeTest) {
  ListWrapper<frame_id_t> mru;
  ASSERT_EQ(0, mru.Size());
  ASSERT_FALSE(mru.Contains(0));
}

TEST(ListWrapperTest, InsertTest1) {
  ListWrapper<frame_id_t> mru;
  ASSERT_EQ(0, mru.Size());
  ASSERT_FALSE(mru.Contains(1));

  mru.Insert(1);
  ASSERT_TRUE(mru.Contains(1));

  mru.Insert(2);
  ASSERT_TRUE(mru.Contains(1));
  ASSERT_TRUE(mru.Contains(2));
  ASSERT_EQ(2, mru.Size());
}

TEST(ListWrapperTest, RemoveTest1) {
  ListWrapper<frame_id_t> mru;
  ASSERT_EQ(0, mru.Size());
  ASSERT_FALSE(mru.Contains(1));

  mru.Insert(1);
  ASSERT_TRUE(mru.Contains(1));

  mru.Insert(2);
  ASSERT_TRUE(mru.Contains(1));
  ASSERT_TRUE(mru.Contains(2));
  ASSERT_EQ(2, mru.Size());

  // REMOVE 2
  mru.Remove(2);
  ASSERT_FALSE(mru.Contains(2));
  ASSERT_EQ(1, mru.Size());
}

TEST(ListWrapperTest, RemoveLru1) {
  ListWrapper<frame_id_t> mru;
  ASSERT_EQ(0, mru.Size());
  ASSERT_FALSE(mru.Contains(1));

  std::optional<frame_id_t> lru = mru.RemoveLru();
  ASSERT_EQ(std::nullopt, lru);

  mru.Insert(1);
  ASSERT_TRUE(mru.Contains(1));

  // RemoveLRU 1
  lru = mru.RemoveLru();
  ASSERT_EQ(1, lru.value());

  mru.Insert(2);
  ASSERT_FALSE(mru.Contains(1));
  ASSERT_TRUE(mru.Contains(2));
  ASSERT_EQ(1, mru.Size());

  // RemoveLRU 2
  lru = mru.RemoveLru();
  ASSERT_EQ(2, lru.value());
}

TEST(ListWrapperTest, MoveForward1) {
  ListWrapper<frame_id_t> mru;
  ASSERT_EQ(0, mru.Size());
  ASSERT_FALSE(mru.Contains(1));

  mru.Insert(1);
  ASSERT_TRUE(mru.Contains(1));

  mru.Insert(2);
  ASSERT_TRUE(mru.Contains(1));
  ASSERT_TRUE(mru.Contains(2));
  ASSERT_EQ(2, mru.Size());

  // Move Forward 1
  mru.MoveForward(1);

  // RemoveLRU 2
  auto lru = mru.RemoveLru();
  ASSERT_EQ(2, lru.value());
}

}  // namespace bustub
