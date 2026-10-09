//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// buffer_pool_manager_test.cpp
//
// Identification: test/buffer/buffer_pool_manager_test.cpp
//
// Copyright (c) 2015-2025, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include <atomic>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <mutex>   // NOLINT
#include <thread>  // NOLINT
#include <vector>

#include "buffer/buffer_pool_manager.h"
#include "gtest/gtest.h"
#include "storage/page/page_guard.h"

namespace bustub {

static std::filesystem::path db_fname("test.bustub");

// The number of frames we give to the buffer pool.
const size_t FRAMES = 10;

void CopyString(char *dest, const std::string &src) {
  BUSTUB_ENSURE(src.length() + 1 <= BUSTUB_PAGE_SIZE, "CopyString src too long");
  snprintf(dest, BUSTUB_PAGE_SIZE, "%s", src.c_str());
}

TEST(BufferPoolManagerTest, VeryBasicTest) {
  // A very basic test.

  auto disk_manager = std::make_shared<DiskManager>(db_fname);
  auto bpm = std::make_shared<BufferPoolManager>(FRAMES, disk_manager.get());

  const page_id_t pid = bpm->NewPage();
  const std::string str = "Hello, world!";

  // Check `WritePageGuard` basic functionality.
  {
    auto guard = bpm->WritePage(pid);
    CopyString(guard.GetDataMut(), str);
    EXPECT_STREQ(guard.GetData(), str.c_str());
  }

  // Check `ReadPageGuard` basic functionality.
  {
    const auto guard = bpm->ReadPage(pid);
    EXPECT_STREQ(guard.GetData(), str.c_str());
  }

  // Check `ReadPageGuard` basic functionality (again).
  {
    const auto guard = bpm->ReadPage(pid);
    EXPECT_STREQ(guard.GetData(), str.c_str());
  }

  ASSERT_TRUE(bpm->DeletePage(pid));
}

TEST(BufferPoolManagerTest, PagePinEasyTest) {
  auto disk_manager = std::make_shared<DiskManager>(db_fname);
  auto bpm = std::make_shared<BufferPoolManager>(2, disk_manager.get());

  const page_id_t pageid0 = bpm->NewPage();
  const page_id_t pageid1 = bpm->NewPage();

  const std::string str0 = "page0";
  const std::string str1 = "page1";
  const std::string str0updated = "page0updated";
  const std::string str1updated = "page1updated";

  {
    auto page0_write_opt = bpm->CheckedWritePage(pageid0);
    ASSERT_TRUE(page0_write_opt.has_value());
    auto page0_write = std::move(page0_write_opt.value());  // NOLINT
    CopyString(page0_write.GetDataMut(), str0);

    auto page1_write_opt = bpm->CheckedWritePage(pageid1);
    ASSERT_TRUE(page1_write_opt.has_value());
    auto page1_write = std::move(page1_write_opt.value());  // NOLINT
    CopyString(page1_write.GetDataMut(), str1);

    ASSERT_EQ(1, bpm->GetPinCount(pageid0));
    ASSERT_EQ(1, bpm->GetPinCount(pageid1));

    const auto temp_page_id1 = bpm->NewPage();
    const auto temp_page1_opt = bpm->CheckedReadPage(temp_page_id1);
    ASSERT_FALSE(temp_page1_opt.has_value());

    const auto temp_page_id2 = bpm->NewPage();
    const auto temp_page2_opt = bpm->CheckedWritePage(temp_page_id2);
    ASSERT_FALSE(temp_page2_opt.has_value());

    ASSERT_EQ(1, bpm->GetPinCount(pageid0));
    page0_write.Drop();
    ASSERT_EQ(0, bpm->GetPinCount(pageid0));

    ASSERT_EQ(1, bpm->GetPinCount(pageid1));
    page1_write.Drop();
    ASSERT_EQ(0, bpm->GetPinCount(pageid1));
  }

  {
    const auto temp_page_id1 = bpm->NewPage();
    const auto temp_page1_opt = bpm->CheckedReadPage(temp_page_id1);
    ASSERT_TRUE(temp_page1_opt.has_value());

    const auto temp_page_id2 = bpm->NewPage();
    const auto temp_page2_opt = bpm->CheckedWritePage(temp_page_id2);
    ASSERT_TRUE(temp_page2_opt.has_value());

    ASSERT_FALSE(bpm->GetPinCount(pageid0).has_value());
    ASSERT_FALSE(bpm->GetPinCount(pageid1).has_value());
  }

  {
    auto page0_write_opt = bpm->CheckedWritePage(pageid0);
    ASSERT_TRUE(page0_write_opt.has_value());
    auto page0_write = std::move(page0_write_opt.value());  // NOLINT
    EXPECT_STREQ(page0_write.GetData(), str0.c_str());
    CopyString(page0_write.GetDataMut(), str0updated);

    auto page1_write_opt = bpm->CheckedWritePage(pageid1);
    ASSERT_TRUE(page1_write_opt.has_value());
    auto page1_write = std::move(page1_write_opt.value());  // NOLINT
    EXPECT_STREQ(page1_write.GetData(), str1.c_str());
    CopyString(page1_write.GetDataMut(), str1updated);

    ASSERT_EQ(1, bpm->GetPinCount(pageid0));
    ASSERT_EQ(1, bpm->GetPinCount(pageid1));
  }

  ASSERT_EQ(0, bpm->GetPinCount(pageid0));
  ASSERT_EQ(0, bpm->GetPinCount(pageid1));

  {
    auto page0_read_opt = bpm->CheckedReadPage(pageid0);
    ASSERT_TRUE(page0_read_opt.has_value());
    const auto page0_read = std::move(page0_read_opt.value());  // NOLINT
    EXPECT_STREQ(page0_read.GetData(), str0updated.c_str());

    auto page1_read_opt = bpm->CheckedReadPage(pageid1);
    ASSERT_TRUE(page1_read_opt.has_value());
    const auto page1_read = std::move(page1_read_opt.value());  // NOLINT
    EXPECT_STREQ(page1_read.GetData(), str1updated.c_str());

    ASSERT_EQ(1, bpm->GetPinCount(pageid0));
    ASSERT_EQ(1, bpm->GetPinCount(pageid1));
  }

  ASSERT_EQ(0, bpm->GetPinCount(pageid0));
  ASSERT_EQ(0, bpm->GetPinCount(pageid1));

  remove(db_fname);
  remove(disk_manager->GetLogFileName());
}

TEST(BufferPoolManagerTest, PagePinMediumTest) {
  auto disk_manager = std::make_shared<DiskManager>(db_fname);
  auto bpm = std::make_shared<BufferPoolManager>(FRAMES, disk_manager.get());

  // Scenario: The buffer pool is empty. We should be able to create a new page.
  const auto pid0 = bpm->NewPage();
  auto page0 = bpm->WritePage(pid0);

  // Scenario: Once we have a page, we should be able to read and write content.
  const std::string hello = "Hello";
  CopyString(page0.GetDataMut(), hello);
  EXPECT_STREQ(page0.GetData(), hello.c_str());

  page0.Drop();

  // Create a vector of unique pointers to page guards, which prevents the guards from getting destructed.
  std::vector<WritePageGuard> pages;

  // Scenario: We should be able to create new pages until we fill up the buffer pool.
  for (size_t i = 0; i < FRAMES; i++) {
    const auto pid = bpm->NewPage();
    auto page = bpm->WritePage(pid);
    pages.push_back(std::move(page));
  }

  // Scenario: All of the pin counts should be 1.
  for (const auto &page : pages) {
    const auto pid = page.GetPageId();
    EXPECT_EQ(1, bpm->GetPinCount(pid));
  }

  // Scenario: Once the buffer pool is full, we should not be able to create any new pages.
  for (size_t i = 0; i < FRAMES; i++) {
    const auto pid = bpm->NewPage();
    const auto fail = bpm->CheckedWritePage(pid);
    ASSERT_FALSE(fail.has_value());
  }

  // Scenario: Drop the first 5 pages to unpin them.
  for (size_t i = 0; i < FRAMES / 2; i++) {
    const auto pid = pages[0].GetPageId();
    EXPECT_EQ(1, bpm->GetPinCount(pid));
    pages.erase(pages.begin());
    EXPECT_EQ(0, bpm->GetPinCount(pid));
  }

  // Scenario: All of the pin counts of the pages we haven't dropped yet should still be 1.
  for (const auto &page : pages) {
    const auto pid = page.GetPageId();
    EXPECT_EQ(1, bpm->GetPinCount(pid));
  }

  // Scenario: After unpinning pages {1, 2, 3, 4, 5}, we should be able to create 4 new pages and bring them into
  // memory. Bringing those 4 pages into memory should evict the first 4 pages {1, 2, 3, 4} because of LRU.
  for (size_t i = 0; i < ((FRAMES / 2) - 1); i++) {
    const auto pid = bpm->NewPage();
    auto page = bpm->WritePage(pid);
    pages.push_back(std::move(page));
  }

  // Scenario: There should be one frame available, and we should be able to fetch the data we wrote a while ago.
  {
    const auto original_page = bpm->ReadPage(pid0);
    EXPECT_STREQ(original_page.GetData(), hello.c_str());
  }

  // Scenario: Once we unpin page 0 and then make a new page, all the buffer pages should now be pinned. Fetching page 0
  // again should fail.
  const auto last_pid = bpm->NewPage();
  const auto last_page = bpm->ReadPage(last_pid);

  const auto fail = bpm->CheckedReadPage(pid0);
  ASSERT_FALSE(fail.has_value());

  // Shutdown the disk manager and remove the temporary file we created.
  disk_manager->ShutDown();
  remove(db_fname);
}

TEST(BufferPoolManagerTest, PageAccessTest) {
  const size_t rounds = 50;

  auto disk_manager = std::make_shared<DiskManager>(db_fname);
  auto bpm = std::make_shared<BufferPoolManager>(1, disk_manager.get());

  const auto pid = bpm->NewPage();
  char buf[BUSTUB_PAGE_SIZE];

  auto thread = std::thread([&]() {
    // The writer can keep writing to the same page.
    for (size_t i = 0; i < rounds; i++) {
      std::this_thread::sleep_for(std::chrono::milliseconds(5));
      auto guard = bpm->WritePage(pid);
      CopyString(guard.GetDataMut(), std::to_string(i));
    }
  });

  for (size_t i = 0; i < rounds; i++) {
    // Wait for a bit before taking the latch, allowing the writer to write some stuff.
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    // While we are reading, nobody should be able to modify the data.
    const auto guard = bpm->ReadPage(pid);

    // Save the data we observe.
    memcpy(buf, guard.GetData(), BUSTUB_PAGE_SIZE);

    // Sleep for a bit. If latching is working properly, nothing should be writing to the page.
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    // Check that the data is unmodified.
    EXPECT_STREQ(guard.GetData(), buf);
  }

  thread.join();
}

TEST(BufferPoolManagerTest, ContentionTest) {
  auto disk_manager = std::make_shared<DiskManager>(db_fname);
  auto bpm = std::make_shared<BufferPoolManager>(FRAMES, disk_manager.get());

  const size_t rounds = 100000;

  const auto pid = bpm->NewPage();

  auto thread1 = std::thread([&]() {
    for (size_t i = 0; i < rounds; i++) {
      auto guard = bpm->WritePage(pid);
      CopyString(guard.GetDataMut(), std::to_string(i));
    }
  });

  auto thread2 = std::thread([&]() {
    for (size_t i = 0; i < rounds; i++) {
      auto guard = bpm->WritePage(pid);
      CopyString(guard.GetDataMut(), std::to_string(i));
    }
  });

  auto thread3 = std::thread([&]() {
    for (size_t i = 0; i < rounds; i++) {
      auto guard = bpm->WritePage(pid);
      CopyString(guard.GetDataMut(), std::to_string(i));
    }
  });

  auto thread4 = std::thread([&]() {
    for (size_t i = 0; i < rounds; i++) {
      auto guard = bpm->WritePage(pid);
      CopyString(guard.GetDataMut(), std::to_string(i));
    }
  });

  thread3.join();
  thread2.join();
  thread4.join();
  thread1.join();
}

TEST(BufferPoolManagerTest, DeadlockTest) {
  auto disk_manager = std::make_shared<DiskManager>(db_fname);
  auto bpm = std::make_shared<BufferPoolManager>(FRAMES, disk_manager.get());

  const auto pid0 = bpm->NewPage();
  const auto pid1 = bpm->NewPage();

  auto guard0 = bpm->WritePage(pid0);

  // A crude way of synchronizing threads, but works for this small case.
  std::atomic<bool> start = false;

  auto child = std::thread([&]() {
    // Acknowledge that we can begin the test.
    start.store(true);

    // Attempt to write to page 0.
    const auto guard0 = bpm->WritePage(pid0);
  });

  // Wait for the other thread to begin before we start the test.
  while (!start.load()) {
  }

  // Make the other thread wait for a bit.
  // This mimics the main thread doing some work while holding the write latch on page 0.
  std::this_thread::sleep_for(std::chrono::milliseconds(1000));

  // If your latching mechanism is incorrect, the next line of code will deadlock.
  // Think about what might happen if you hold a certain "all-encompassing" latch for too long...

  // While holding page 0, take the latch on page 1.
  const auto guard1 = bpm->WritePage(pid1);

  // Let the child thread have the page 0 since we're done with it.
  guard0.Drop();

  child.join();
}

TEST(BufferPoolManagerTest, EvictableTest) {
  // Test if the evictable status of a frame is always correct.
  const size_t rounds = 1000;
  const size_t num_readers = 8;

  auto disk_manager = std::make_shared<DiskManager>(db_fname);
  // Only allocate one frame of memory to the buffer pool manager.
  auto bpm = std::make_shared<BufferPoolManager>(1, disk_manager.get());

  for (size_t i = 0; i < rounds; i++) {
    std::mutex mutex;
    std::condition_variable cv;

    // This signal tells the readers that they can start reading after the main thread has already taken the read latch.
    bool signal = false;

    // This page will be loaded into the only available frame.
    const auto winner_pid = bpm->NewPage();
    // We will attempt to load this page into the occupied frame, and it should fail every time.
    const auto loser_pid = bpm->NewPage();

    std::vector<std::thread> readers;
    for (size_t j = 0; j < num_readers; j++) {
      readers.emplace_back([&]() {
        std::unique_lock<std::mutex> lock(mutex);

        // Wait until the main thread has taken a read latch on the page.
        while (!signal) {
          cv.wait(lock);
        }

        // Read the page in shared mode.
        const auto read_guard = bpm->ReadPage(winner_pid);

        // Since the only frame is pinned, no thread should be able to bring in a new page.
        ASSERT_FALSE(bpm->CheckedReadPage(loser_pid).has_value());
      });
    }

    std::unique_lock<std::mutex> lock(mutex);

    if (i % 2 == 0) {
      // Take the read latch on the page and pin it.
      auto read_guard = bpm->ReadPage(winner_pid);

      // Wake up all of the readers.
      signal = true;
      cv.notify_all();
      lock.unlock();

      // Allow other threads to read.
      read_guard.Drop();
    } else {
      // Take the read latch on the page and pin it.
      auto write_guard = bpm->WritePage(winner_pid);

      // Wake up all of the readers.
      signal = true;
      cv.notify_all();
      lock.unlock();

      // Allow other threads to read.
      write_guard.Drop();
    }

    for (size_t i = 0; i < num_readers; i++) {
      readers[i].join();
    }
  }
}

// ##########################################################
// DeletePage tests
// ##########################################################

TEST(BufferPoolManagerTest, DeletePagePinnedFailsTest) {
  // DeletePage must refuse to remove a page that is currently pinned, and must leave everything untouched when it
  // does.
  auto disk_manager = std::make_shared<DiskManager>(db_fname);
  auto bpm = std::make_shared<BufferPoolManager>(FRAMES, disk_manager.get());

  const auto pid = bpm->NewPage();
  const std::string data = "pinned page contents";

  {
    auto guard = bpm->WritePage(pid);
    CopyString(guard.GetDataMut(), data);
  }

  // Pin the page with a read guard, then try (and fail) to delete it.
  {
    const auto guard = bpm->ReadPage(pid);
    ASSERT_EQ(1, bpm->GetPinCount(pid));

    ASSERT_FALSE(bpm->DeletePage(pid));

    // Nothing should have changed: still pinned, still readable.
    ASSERT_EQ(1, bpm->GetPinCount(pid));
    EXPECT_STREQ(guard.GetData(), data.c_str());
  }

  // Once unpinned, deletion should succeed.
  ASSERT_EQ(0, bpm->GetPinCount(pid));
  ASSERT_TRUE(bpm->DeletePage(pid));
  ASSERT_FALSE(bpm->GetPinCount(pid).has_value());

  disk_manager->ShutDown();
  remove(db_fname);
}

TEST(BufferPoolManagerTest, DeletePageFreesFrameForReuseTest) {
  // DeletePage must actually return the frame to the free list (not merely drop it from the replacer), so a
  // subsequent page can be brought in without needing to evict anything.
  auto disk_manager = std::make_shared<DiskManager>(db_fname);
  // A single frame makes it obvious whether the frame was really freed.
  auto bpm = std::make_shared<BufferPoolManager>(1, disk_manager.get());

  const auto pid0 = bpm->NewPage();
  {
    auto guard = bpm->WritePage(pid0);
    CopyString(guard.GetDataMut(), "page0");
  }
  ASSERT_EQ(0, bpm->GetPinCount(pid0));

  ASSERT_TRUE(bpm->DeletePage(pid0));
  ASSERT_FALSE(bpm->GetPinCount(pid0).has_value());

  // The only frame has no page in it and nothing left in the replacer to evict, so this can only succeed if
  // DeletePage pushed the frame onto the free list.
  const auto pid1 = bpm->NewPage();
  auto guard1_opt = bpm->CheckedWritePage(pid1);
  ASSERT_TRUE(guard1_opt.has_value());

  disk_manager->ShutDown();
  remove(db_fname);
}

TEST(BufferPoolManagerTest, DeletePageConcurrentPinRaceTest) {
  // A concurrent DeletePage attempt must see the page as pinned for as long as the owning guard is alive, and must
  // succeed only after the guard is dropped.
  auto disk_manager = std::make_shared<DiskManager>(db_fname);
  auto bpm = std::make_shared<BufferPoolManager>(FRAMES, disk_manager.get());

  const auto pid = bpm->NewPage();
  {
    auto guard = bpm->WritePage(pid);
    CopyString(guard.GetDataMut(), "concurrent delete target");
  }

  auto guard = bpm->ReadPage(pid);

  std::atomic<bool> attempted{false};
  std::atomic<bool> delete_while_pinned_result{true};

  std::thread deleter([&]() {
    delete_while_pinned_result.store(bpm->DeletePage(pid));
    attempted.store(true);
  });

  // Wait for the child thread to actually attempt the delete while we still hold the guard.
  while (!attempted.load()) {
  }

  // The page was pinned for the entire attempt, so it must have failed.
  ASSERT_FALSE(delete_while_pinned_result.load());
  ASSERT_EQ(1, bpm->GetPinCount(pid));

  deleter.join();

  // Now that we drop the pin, deletion should succeed.
  guard.Drop();
  ASSERT_TRUE(bpm->DeletePage(pid));
  ASSERT_FALSE(bpm->GetPinCount(pid).has_value());

  disk_manager->ShutDown();
  remove(db_fname);
}

TEST(BufferPoolManagerTest, DeletePageConcurrentMultipleDeletersTest) {
  // Concurrently deleting several distinct, unpinned pages must not corrupt the buffer pool's shared bookkeeping
  // (page table / free list / replacer): every frame must come back exactly once.
  auto disk_manager = std::make_shared<DiskManager>(db_fname);
  auto bpm = std::make_shared<BufferPoolManager>(FRAMES, disk_manager.get());

  std::vector<page_id_t> pids;
  for (size_t i = 0; i < FRAMES; i++) {
    const auto pid = bpm->NewPage();
    {
      auto guard = bpm->WritePage(pid);
      CopyString(guard.GetDataMut(), std::to_string(i));
    }
    pids.push_back(pid);
  }

  std::vector<std::thread> deleters;
  std::vector<std::atomic<bool>> results(FRAMES);
  for (size_t i = 0; i < FRAMES; i++) {
    deleters.emplace_back([&, i]() { results[i].store(bpm->DeletePage(pids[i])); });
  }
  for (auto &t : deleters) {
    t.join();
  }

  for (size_t i = 0; i < FRAMES; i++) {
    ASSERT_TRUE(results[i].load());
    ASSERT_FALSE(bpm->GetPinCount(pids[i]).has_value());
  }

  // Every frame must have been returned to the free list exactly once: we should be able to bring in exactly
  // FRAMES brand new pages simultaneously without any failing.
  std::vector<WritePageGuard> guards;
  for (size_t i = 0; i < FRAMES; i++) {
    const auto pid = bpm->NewPage();
    auto guard_opt = bpm->CheckedWritePage(pid);
    ASSERT_TRUE(guard_opt.has_value());
    guards.push_back(std::move(guard_opt).value());
  }

  disk_manager->ShutDown();
  remove(db_fname);
}

// ##########################################################
// FlushPageUnsafe / FlushAllPagesUnsafe tests
// ##########################################################

TEST(BufferPoolManagerTest, FlushPageUnsafeBasicTest) {
  auto disk_manager = std::make_shared<DiskManager>(db_fname);
  auto bpm = std::make_shared<BufferPoolManager>(FRAMES, disk_manager.get());

  // A page that was never brought into memory must fail immediately.
  const auto never_loaded_pid = bpm->NewPage();
  ASSERT_FALSE(bpm->FlushPageUnsafe(never_loaded_pid));

  const auto pid = bpm->NewPage();
  const std::string data = "flush me";
  {
    auto guard = bpm->WritePage(pid);
    CopyString(guard.GetDataMut(), data);
  }  // Dropped: Drop() no longer flushes, so the page stays resident, unpinned, AND dirty.

  {
    const auto guard = bpm->ReadPage(pid);
    ASSERT_TRUE(guard.IsDirty());
  }

  ASSERT_TRUE(bpm->FlushPageUnsafe(pid));

  {
    const auto guard = bpm->ReadPage(pid);
    ASSERT_FALSE(guard.IsDirty());
    EXPECT_STREQ(guard.GetData(), data.c_str());
  }

  // Verify independently of the buffer pool that the bytes actually made it to disk.
  char buf[BUSTUB_PAGE_SIZE];
  disk_manager->ReadPage(pid, buf);
  EXPECT_STREQ(buf, data.c_str());

  // Flushing an already-clean, resident page is a safe no-op.
  ASSERT_TRUE(bpm->FlushPageUnsafe(pid));

  disk_manager->ShutDown();
  remove(db_fname);
}

TEST(BufferPoolManagerTest, FlushAllPagesUnsafeBasicTest) {
  auto disk_manager = std::make_shared<DiskManager>(db_fname);
  auto bpm = std::make_shared<BufferPoolManager>(FRAMES, disk_manager.get());

  std::vector<page_id_t> pids;
  for (size_t i = 0; i < FRAMES; i++) {
    const auto pid = bpm->NewPage();
    {
      auto guard = bpm->WritePage(pid);
      CopyString(guard.GetDataMut(), std::to_string(i));
    }  // Dropped, unpinned, left dirty.
    pids.push_back(pid);
  }

  for (size_t i = 0; i < FRAMES; i++) {
    const auto guard = bpm->ReadPage(pids[i]);
    ASSERT_TRUE(guard.IsDirty());
  }

  bpm->FlushAllPagesUnsafe();

  for (size_t i = 0; i < FRAMES; i++) {
    {
      const auto guard = bpm->ReadPage(pids[i]);
      ASSERT_FALSE(guard.IsDirty());
      EXPECT_STREQ(guard.GetData(), std::to_string(i).c_str());
    }

    char buf[BUSTUB_PAGE_SIZE];
    disk_manager->ReadPage(pids[i], buf);
    EXPECT_STREQ(buf, std::to_string(i).c_str());
  }

  disk_manager->ShutDown();
  remove(db_fname);
}

// ##########################################################
// FlushPage / FlushAllPages tests
//
// Unlike FlushPageUnsafe / FlushAllPagesUnsafe, these are documented to take a lock on the page
// before flushing it, specifically so they're safe to call while other threads are concurrently
// reading/writing the same pages. So, unlike the Unsafe tests above, concurrency here is fair game.
// ##########################################################

TEST(BufferPoolManagerTest, FlushPageBasicTest) {
  auto disk_manager = std::make_shared<DiskManager>(db_fname);
  auto bpm = std::make_shared<BufferPoolManager>(FRAMES, disk_manager.get());

  // A page that was never brought into memory must fail immediately.
  const auto never_loaded_pid = bpm->NewPage();
  ASSERT_FALSE(bpm->FlushPage(never_loaded_pid));

  const auto pid = bpm->NewPage();
  const std::string data = "flush me safely";
  {
    auto guard = bpm->WritePage(pid);
    CopyString(guard.GetDataMut(), data);
  }  // Dropped: stays resident, unpinned, and dirty.

  {
    const auto guard = bpm->ReadPage(pid);
    ASSERT_TRUE(guard.IsDirty());
  }

  ASSERT_TRUE(bpm->FlushPage(pid));

  {
    const auto guard = bpm->ReadPage(pid);
    ASSERT_FALSE(guard.IsDirty());
    EXPECT_STREQ(guard.GetData(), data.c_str());
  }

  // Verify independently of the buffer pool that the bytes actually made it to disk.
  char buf[BUSTUB_PAGE_SIZE];
  disk_manager->ReadPage(pid, buf);
  EXPECT_STREQ(buf, data.c_str());

  // Flushing an already-clean, resident page is a safe no-op.
  ASSERT_TRUE(bpm->FlushPage(pid));

  disk_manager->ShutDown();
  remove(db_fname);
}

TEST(BufferPoolManagerTest, FlushAllPagesBasicTest) {
  auto disk_manager = std::make_shared<DiskManager>(db_fname);
  auto bpm = std::make_shared<BufferPoolManager>(FRAMES, disk_manager.get());

  std::vector<page_id_t> pids;
  for (size_t i = 0; i < FRAMES; i++) {
    const auto pid = bpm->NewPage();
    {
      auto guard = bpm->WritePage(pid);
      CopyString(guard.GetDataMut(), std::to_string(i));
    }  // Dropped, unpinned, left dirty.
    pids.push_back(pid);
  }

  for (size_t i = 0; i < FRAMES; i++) {
    const auto guard = bpm->ReadPage(pids[i]);
    ASSERT_TRUE(guard.IsDirty());
  }

  bpm->FlushAllPages();

  for (size_t i = 0; i < FRAMES; i++) {
    {
      const auto guard = bpm->ReadPage(pids[i]);
      ASSERT_FALSE(guard.IsDirty());
      EXPECT_STREQ(guard.GetData(), std::to_string(i).c_str());
    }

    char buf[BUSTUB_PAGE_SIZE];
    disk_manager->ReadPage(pids[i], buf);
    EXPECT_STREQ(buf, std::to_string(i).c_str());
  }

  disk_manager->ShutDown();
  remove(db_fname);
}

TEST(BufferPoolManagerTest, FlushPageConcurrentNoTornWriteHackTest) {
  // The whole point of FlushPage taking a lock on the page is that, unlike FlushPageUnsafe, it must
  // NEVER observe a torn write: every flushed snapshot has to be either the complete old value or
  // the complete new value, never a mix. Try to hack it by hammering a writer (which repeatedly
  // pins, overwrites the whole page with a uniform byte, and unpins) against a concurrent flusher,
  // and fail loudly the instant a torn page is observed on disk.
  auto disk_manager = std::make_shared<DiskManager>(db_fname);
  auto bpm = std::make_shared<BufferPoolManager>(FRAMES, disk_manager.get());

  const auto pid = bpm->NewPage();
  {
    auto guard = bpm->WritePage(pid);
    std::memset(guard.GetDataMut(), 'A', BUSTUB_PAGE_SIZE);
  }

  constexpr int kWriterIterations = 5000;
  std::atomic<bool> torn_detected{false};
  std::atomic<bool> writer_done{false};

  std::thread writer([&]() {
    char value = 'A';
    for (int i = 0; i < kWriterIterations; i++) {
      auto guard = bpm->WritePage(pid);
      std::memset(guard.GetDataMut(), value, BUSTUB_PAGE_SIZE);
      value = (value == 'A') ? 'B' : 'A';
    }
    writer_done.store(true);
  });

  // Everything below must unconditionally reach writer.join(): an exception escaping before that
  // (e.g. FlushPage still throwing UNIMPLEMENTED) would destroy a still-joinable std::thread while
  // unwinding, and that calls std::terminate() -- turning a clean test failure into a hard process
  // abort. Catch, record, and join first; assert only after the background thread is guaranteed done.
  std::string flush_error;
  char buf[BUSTUB_PAGE_SIZE];
  try {
    while (!writer_done.load()) {
      if (!bpm->FlushPage(pid)) {
        flush_error = "FlushPage returned false for a resident page";
        break;
      }
      disk_manager->ReadPage(pid, buf);

      bool all_a = true;
      bool all_b = true;
      for (char c : buf) {
        all_a &= (c == 'A');
        all_b &= (c == 'B');
      }
      if (!all_a && !all_b) {
        torn_detected.store(true);
        break;
      }
    }
  } catch (const std::exception &e) {
    flush_error = e.what();
  }

  writer.join();

  ASSERT_TRUE(flush_error.empty()) << "FlushPage failed: " << flush_error;
  ASSERT_FALSE(torn_detected.load()) << "FlushPage observed a torn (partially-written) page on "
                                         "disk -- it must take the frame's latch before reading "
                                         "the buffer it flushes";

  disk_manager->ShutDown();
  remove(db_fname);
}

TEST(BufferPoolManagerTest, FlushAllPagesConcurrentChurnHackTest) {
  // FlushPage/FlushAllPages are documented to take a lock, so -- unlike the Unsafe versions -- this
  // must survive heavy concurrent page churn (NewPage + WritePage + DeletePage from several threads)
  // without crashing, deadlocking, or corrupting page_table_/frames_. If this hangs, that's a strong
  // signal of a lock-ordering/deadlock bug; if it crashes under ASan, that's a missed lock somewhere.
  auto disk_manager = std::make_shared<DiskManager>(db_fname);
  auto bpm = std::make_shared<BufferPoolManager>(4, disk_manager.get());

  std::atomic<bool> stop{false};
  std::mutex error_mutex;
  std::string first_error;
  auto record_error = [&](const std::string &what) {
    std::lock_guard<std::mutex> lock(error_mutex);
    if (first_error.empty()) {
      first_error = what;
    }
    stop.store(true);
  };

  // An exception escaping a std::thread's entry function (not just an assertion failing) calls
  // std::terminate() immediately -- so every thread body here must catch, record, and return
  // cleanly instead of letting anything (e.g. FlushPage/FlushAllPages still throwing UNIMPLEMENTED)
  // propagate out and hard-abort the process.
  std::thread flusher([&]() {
    try {
      while (!stop.load(std::memory_order_relaxed)) {
        bpm->FlushPage(0);
        bpm->FlushAllPages();
      }
    } catch (const std::exception &e) {
      record_error(e.what());
    }
  });

  std::vector<std::thread> churners;
  for (int t = 0; t < 4; t++) {
    churners.emplace_back([&]() {
      try {
        for (int i = 0; i < 5000 && !stop.load(std::memory_order_relaxed); i++) {
          const auto pid = bpm->NewPage();
          {
            auto guard = bpm->WritePage(pid);
            CopyString(guard.GetDataMut(), "churn");
          }
          bpm->DeletePage(pid);
        }
      } catch (const std::exception &e) {
        record_error(e.what());
      }
    });
  }

  for (auto &t : churners) {
    t.join();
  }
  stop.store(true);
  flusher.join();

  ASSERT_TRUE(first_error.empty()) << "concurrent churn against FlushPage/FlushAllPages failed: " << first_error;

  disk_manager->ShutDown();
  remove(db_fname);
}

}  // namespace bustub
