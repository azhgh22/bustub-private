//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// disk_scheduler.cpp
//
// Identification: src/storage/disk/disk_scheduler.cpp
//
// Copyright (c) 2015-2025, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "storage/disk/disk_scheduler.h"
#include <vector>
#include "common/macros.h"
#include "storage/disk/disk_manager.h"

namespace bustub {

DiskScheduler::DiskScheduler(DiskManager *disk_manager) : disk_manager_(disk_manager) {
  // UNIMPLEMENTED("TODO(P1): Add implementation.");
  // Spawn the background thread
  background_thread_.emplace([&] { StartWorkerThread(); });
}

DiskScheduler::~DiskScheduler() {
  // Put a `std::nullopt` in the queue to signal to exit the loop
  request_queue_.Put(std::nullopt);
  if (background_thread_.has_value()) {
    background_thread_->join();
  }
}

/**
 * TODO(P1): Add implementation
 *
 * @brief Schedules a request for the DiskManager to execute.
 *
 * @param requests The requests to be scheduled.
 */
void DiskScheduler::Schedule(std::vector<DiskRequest> &requests) {
  for(DiskRequest& req : requests){
    request_queue_.Put(std::move(req));
  }
}


void DiskScheduler::HandleRead(page_id_t page_id,char* data, std::promise<bool> callback){
  disk_manager_->ReadPage(page_id,data);
  callback.set_value(true);
}

void DiskScheduler::HandleWrite(page_id_t page_id,char* data, std::promise<bool> callback){
  disk_manager_->WritePage(page_id,data);
  callback.set_value(true);
}

/**
 * TODO(P1): Add implementation
 *
 * @brief Background worker thread function that processes scheduled requests.
 *
 * The background thread needs to process requests while the DiskScheduler exists, i.e., this function should not
 * return until ~DiskScheduler() is called. At that point you need to make sure that the function does return.
 */
void DiskScheduler::StartWorkerThread() {
  while (true){
    std::optional<DiskRequest> req = request_queue_.Get();
    if(!req.has_value()) break;

    if(req->is_write_){
      HandleWrite(req->page_id_,req->data_,std::move(req->callback_));
    }else{
      HandleRead(req->page_id_,req->data_,std::move(req->callback_));
    }
    
  }
  
}

}  // namespace bustub
