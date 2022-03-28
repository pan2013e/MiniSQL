#ifndef _REPLACER_H
#define _REPLACER_H

#include "pch.h"

// abstract class for different replacement policies
class Replacer {
public:
    using block_id_t = int;
    Replacer() = default;
    virtual ~Replacer() = default;
    virtual void clear() = 0;
    virtual void visit(block_id_t block_id) = 0;
    virtual void add(block_id_t block_id) = 0;
    virtual void pin(block_id_t block_id) = 0;
    virtual bool evict(block_id_t& block_id) = 0;
    virtual size_t size() const = 0;
    virtual bool is_full() const = 0;
};

// least recently used replacement
class LRU : public Replacer {
public:
    explicit LRU(int sz) : max_sz(sz){};
    ~LRU() override = default;
    void clear() override { blocks_inuse.clear(); }
    void visit(block_id_t block_id) override;
    void add(block_id_t block_id) override;
    void pin(block_id_t block_id) override { visit(block_id); }
    bool evict(block_id_t& block_id) override;
    inline size_t size() const override { return blocks_inuse.size(); }
    inline bool is_full() const override { return size()==max_sz; }

private:
    void evict();
    size_t max_sz;
    std::list<block_id_t> blocks_inuse;
};


#endif //_REPLACER_H
