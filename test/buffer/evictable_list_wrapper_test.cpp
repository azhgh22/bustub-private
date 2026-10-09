#include "buffer/evictable_list_wrapper.h"

#include "gtest/gtest.h"

namespace bustub {

TEST(EvListWrapperTest, SizeTest) {
  EVListWrapper<frame_id_t> mru;
  ASSERT_EQ(0, mru.Size());
  ASSERT_FALSE(mru.Contains(0));
}

TEST(EvListWrapperTest, InsertTest1) {
  EVListWrapper<frame_id_t> mru;
  ASSERT_EQ(0, mru.Size());
  ASSERT_FALSE(mru.Contains(1));
  ASSERT_EQ(0, mru.EvictableSize());

  mru.Insert(1, false);
  ASSERT_TRUE(mru.Contains(1));

  mru.Insert(2, false);
  ASSERT_TRUE(mru.Contains(1));
  ASSERT_TRUE(mru.Contains(2));
  ASSERT_EQ(2, mru.Size());
  ASSERT_EQ(0, mru.EvictableSize());
}

TEST(EvListWrapperTest, InsertTest2) {
  EVListWrapper<frame_id_t> mru;
  ASSERT_EQ(0, mru.Size());
  ASSERT_FALSE(mru.Contains(1));
  ASSERT_EQ(0, mru.EvictableSize());

  mru.Insert(1, false);
  ASSERT_TRUE(mru.Contains(1));

  mru.Insert(2, true);
  ASSERT_TRUE(mru.Contains(1));
  ASSERT_TRUE(mru.Contains(2));
  ASSERT_EQ(2, mru.Size());
  ASSERT_EQ(1, mru.EvictableSize());
}

TEST(EvListWrapperTest, RemoveTest1) {
  EVListWrapper<frame_id_t> mru;
  ASSERT_EQ(0, mru.Size());
  ASSERT_FALSE(mru.Contains(1));

  mru.Insert(1, false);
  ASSERT_TRUE(mru.Contains(1));

  mru.Insert(2, false);
  ASSERT_TRUE(mru.Contains(1));
  ASSERT_TRUE(mru.Contains(2));
  ASSERT_EQ(2, mru.Size());

  // REMOVE 2
  mru.Remove(2);
  ASSERT_FALSE(mru.Contains(2));
  ASSERT_EQ(1, mru.Size());
}

TEST(EvListWrapperTest, RemoveTest2) {
  EVListWrapper<frame_id_t> mru;
  ASSERT_EQ(0, mru.Size());
  ASSERT_FALSE(mru.Contains(1));

  mru.Insert(1, false);
  ASSERT_TRUE(mru.Contains(1));

  mru.Insert(2, true);
  ASSERT_TRUE(mru.Contains(1));
  ASSERT_TRUE(mru.Contains(2));
  ASSERT_EQ(2, mru.Size());

  // REMOVE 2
  mru.Remove(2);
  ASSERT_FALSE(mru.Contains(2));
  ASSERT_EQ(1, mru.Size());
}

TEST(EvListWrapperTest, RemoveLru1) {
  EVListWrapper<frame_id_t> mru;
  ASSERT_EQ(0, mru.Size());
  ASSERT_FALSE(mru.Contains(1));

  std::optional<frame_id_t> lru = mru.RemoveLru();
  ASSERT_EQ(std::nullopt, lru);

  mru.Insert(1, false);
  ASSERT_TRUE(mru.Contains(1));

  // RemoveLRU 1
  lru = mru.RemoveLru();
  ASSERT_EQ(std::nullopt, lru);

  mru.Insert(2, true);
  ASSERT_TRUE(mru.Contains(1));
  ASSERT_TRUE(mru.Contains(2));
  ASSERT_EQ(2, mru.Size());
  ASSERT_EQ(1, mru.EvictableSize());

  // RemoveLRU 2
  lru = mru.RemoveLru();
  ASSERT_EQ(2, lru.value());
  ASSERT_EQ(1, mru.Size());
  ASSERT_EQ(0, mru.EvictableSize());
}

TEST(EvListWrapperTest, MoveForward1) {
  EVListWrapper<frame_id_t> mru;
  ASSERT_EQ(0, mru.Size());
  ASSERT_FALSE(mru.Contains(1));

  mru.Insert(1, true);
  ASSERT_TRUE(mru.Contains(1));

  mru.Insert(2, false);
  ASSERT_TRUE(mru.Contains(1));
  ASSERT_TRUE(mru.Contains(2));
  ASSERT_EQ(2, mru.Size());

  // Move Forward 1
  mru.MoveForward(1);

  // RemoveLRU 2
  auto lru = mru.RemoveLru();
  ASSERT_EQ(1, lru.value());
}

TEST(EvListWrapperTest, MoveForward2) {
  EVListWrapper<frame_id_t> mru;
  ASSERT_EQ(0, mru.Size());
  ASSERT_FALSE(mru.Contains(1));

  mru.Insert(1, true);
  ASSERT_TRUE(mru.Contains(1));

  mru.Insert(2, true);
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
