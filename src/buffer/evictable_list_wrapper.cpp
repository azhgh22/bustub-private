#include "buffer/evictable_list_wrapper.h"


namespace bustub
{

template <typename KeyType>
void EVListWrapper<KeyType>::Print(){
    std::cout<<"LRU size: "<<this->lru_.size()<<" Lru EvictableSize: "<<evictables_.size()<<std::endl;
	for (auto it = this->lru_.rbegin(); it != this->lru_.rend(); ++it) {
		std::cout<<(*it)<<" ";
	}
	std::cout<<std::endl;

	for (auto it = evictables_.begin(); it != evictables_.end(); ++it) {
		std::cout<<(it->first)<<" "<<it->second<<" ";
	}
	std::cout<<std::endl;
}

template <typename KeyType>
size_t EVListWrapper<KeyType>::EvictableSize(){
    return evictables_.size();
}

template <typename KeyType>
void EVListWrapper<KeyType>::Insert(KeyType key, bool ev){
	this->lru_.push_front(key);
    this->map_[key] = this->lru_.begin();
    this->ev_map_[key] = {update_time_,ev};
    if(ev){
        this->evictables_.insert({update_time_,key});
    }
    update_time_++;
}

template <typename KeyType>
void EVListWrapper<KeyType>::Remove(KeyType key){
	if(!this->Contains(key)) return;
	auto it = this->map_[key];
	this->lru_.erase(it);
	this->map_.erase(key);

    // remove if evictable
    evictables_.erase({ev_map_[key].update_time_,key});
    ev_map_.erase(key);
}

template <typename KeyType>
void EVListWrapper<KeyType>::MoveForward(KeyType key){
    if(!this->Contains(key)) return;
    bool is_ev = ev_map_[key].is_ev;
    Remove(key);
    Insert(key,is_ev);
}

template <typename KeyType>
std::optional<KeyType> EVListWrapper<KeyType>::RemoveLru(){
	if(EvictableSize()==0) return std::nullopt;

    std::pair<timestamp,frame_id_t> lru = *evictables_.begin();
    Remove(lru.second);
	return lru.second;
}

template class EVListWrapper<frame_id_t>;

}
