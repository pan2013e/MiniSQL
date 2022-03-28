#ifndef _BUFFER_
#define _BUFFER_

#include "pch.h"
#include "replacer.h"

class Block {
friend class BufferPool;
public:
    Block() { reset();}
    virtual ~Block() = default;
    inline byte* content() { return data; }
    inline bool dirty() { return isDirty; }

private:
    Block(Block&) = default;
    String file;
    size_t page_offset;
    void reset() {
        file = "";
        page_offset = INVALID_ID;
        isDirty = false;
        memset(data, 0, BlockSize);
    }
    bool isDirty = false;
    byte data[BlockSize];
};

class FILEIO {
    friend class BufferPool;
#ifndef DEBUG
    private:
#else
    public:
#endif
    size_t get_size(String& file);
    void write_to_file(String file, size_t page_offset, const byte* data);
    void read_from_file(String file, size_t page_offset, byte* data);
};

class BufferPool {
public:
    explicit BufferPool(size_t size) : buf_size(size) {
        assert(buf_size > 0);
        buf = new Block[buf_size];
        _replacer=new LRU(buf_size);
        for (size_t i = 0;i < buf_size;i++) {
            block_unused.push_back(i);
        }
    }
    virtual ~BufferPool() {
        flush_all();
        delete[] buf;
        delete _replacer;
    }
    byte* get_data(String file, size_t page_offset, block_id_t* id = nullptr);
    void modify_data(String file, size_t page_offset, byte* data);
    inline void pin_block(block_id_t block_id) {
        _replacer->pin(block_id);
    }
    void flush_all();
    void flush(String file, size_t page_offset);
    void reset(String file, size_t page_offset);
    void reset(String file);
    inline size_t file_size(String file) {
        return io.get_size(file)/BlockSize;
    }

private:
    BufferPool() = default;
    BufferPool(BufferPool&) = default;
    FILEIO io;
    Block* buf;
    size_t buf_size;
    Replacer* _replacer;
    std::list<block_id_t> block_unused;
    std::unordered_map<String, block_id_t> file_to_buf;
    String name_mapping(String& file, size_t page_offset);
};

#ifndef _WIN32
void itoa(int src, char* dst, radix_type radix = DEC);
#endif

#endif