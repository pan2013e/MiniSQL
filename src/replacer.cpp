#include "replacer.h"

void LRU::add(Replacer::block_id_t block_id){
    auto it=std::find(blocks_inuse.begin(),blocks_inuse.end(),block_id);
    if(it!=blocks_inuse.end()) return; // exist this block
    if(blocks_inuse.size()==max_sz) evict();
    blocks_inuse.push_front(block_id);
}

bool LRU::evict(Replacer::block_id_t& block_id) {
    if(blocks_inuse.empty()){
        block_id = INVALID_ID;
        return false;
    }
    block_id = blocks_inuse.back();
    blocks_inuse.pop_back();
    return true;
}

void LRU::evict(){
    if(blocks_inuse.empty()){
        return;
    }
    blocks_inuse.pop_back();
}

void LRU::visit(Replacer::block_id_t block_id) {
    auto it=std::find(blocks_inuse.begin(),blocks_inuse.end(),block_id);
    if(it==blocks_inuse.end()) return; // does not exist
    blocks_inuse.erase(it);
    blocks_inuse.push_front(block_id);
}
