#include "buffer.h"

#ifndef _WIN32
const char digit[] = {
    '0','1','2','3',
    '4','5','6','7',
    '8','9','A','B',
    'C','D','E','F',
};
void itoa(int src, char* dst, radix_type radix) {
    String res, temp;
    if (!src) res = res + '0';
    while (src) {
        temp = digit[src % radix];
        res = temp + res;
        src /= radix;
    }
    assert(res.length() <= 20);
    memcpy(dst, res.c_str(), res.length());
}
#endif

size_t FILEIO::get_size(String& file) {
    static struct stat buf;
    int rc = ::stat(file.c_str(), &buf);
    return rc == 0 ? buf.st_size : -1;
}

void FILEIO::write_to_file(String file, size_t page_offset, const byte* data) {
    std::fstream wt(file, std::ios::binary | std::ios::in | std::ios::out);
    if(!wt.is_open()){
        wt.clear();
        wt.open(file, std::ios::binary | std::ios::trunc | std::ios::in | std::ios::out);
        wt.close();
        wt.open(file, std::ios::binary | std::ios::in | std::ios::out);
    }
    size_t byte_offset = page_offset * BlockSize;
    wt.seekp(byte_offset, std::ios::beg);
    wt.write(data, BlockSize);
    wt.flush();
}

void FILEIO::read_from_file(String file, size_t page_offset, byte* data) {
    std::fstream rd(file, std::ios::binary | std::ios::in | std::ios::out);
    if (!rd.is_open()) {
        data = nullptr;
        return;
    }
    size_t byte_offset = page_offset * BlockSize;
    if (byte_offset > get_size(file)) {
        data = nullptr;
        return;
    }else{
        rd.seekg(byte_offset, std::ios::beg);
        rd.read(data, BlockSize);
        size_t cnt=rd.gcount();
        if (cnt < BlockSize) { // file ends before a block size
            memset(data + cnt, 0, BlockSize - cnt);
        }
    }
}

String BufferPool::name_mapping(String& file, size_t page_offset) {
    char offst[20] = { 0 };
    itoa(page_offset, offst, DEC);
    return file + offst;
}

byte* BufferPool::get_data(String file, size_t page_offset, block_id_t* id) {
    String temp = name_mapping(file, page_offset);
    // data in buffer
    if(file_to_buf.find(temp)!=file_to_buf.end()){
        auto fetched_id = file_to_buf[temp];
        _replacer->visit(fetched_id);
        if (id) *id = fetched_id;
        return buf[fetched_id].content();
    }
    // need to fetch from memory
    // search in free list first
    if (!block_unused.empty()) {
        // printf("..in free list...\n");
        auto fetched_id = block_unused.front();
        block_unused.pop_front();
        _replacer->add(fetched_id);
        file_to_buf[temp] = fetched_id; // add to hash table
        buf[fetched_id].file = file;
        buf[fetched_id].page_offset = page_offset;
        io.read_from_file(file, page_offset, buf[fetched_id].data);
        if (id) *id = fetched_id;
        return buf[fetched_id].content();
    }
    // search in replacer
    block_id_t fetched_id;
    _replacer->evict(fetched_id);
    for (auto it = file_to_buf.begin();it != file_to_buf.end();it++) {
        if (it->second == fetched_id) {
            file_to_buf.erase(it);
            break;
        }
    }
    if (buf[fetched_id].dirty()) {
        io.write_to_file(buf[fetched_id].file, buf[fetched_id].page_offset, buf[fetched_id].content());
        buf[fetched_id].isDirty = false;
    }
    file_to_buf[temp] = fetched_id;
    buf[fetched_id].file = file;
    buf[fetched_id].page_offset = page_offset;
    _replacer->add(fetched_id);
    io.read_from_file(file, page_offset, buf[fetched_id].data);
    if (id) *id = fetched_id;
    return buf[fetched_id].content();
}

void BufferPool::modify_data(String file, size_t page_offset, byte* data) {
    if (!data) return;
    block_id_t to_mod=INVALID_ID;
    get_data(file, page_offset, &to_mod);
    assert(to_mod != INVALID_ID);
    buf[to_mod].isDirty = true;
    memcpy(buf[to_mod].data, data, BlockSize);
}

void BufferPool::flush(String file, size_t page_offset){
    String temp=name_mapping(file,page_offset);
    if(file_to_buf.find(temp)!=file_to_buf.end()){
        auto fetched_id = file_to_buf[temp];
        if (buf[fetched_id].dirty()) {
            io.write_to_file(buf[fetched_id].file, buf[fetched_id].page_offset, buf[fetched_id].content());
            buf[fetched_id].isDirty = false;
        }
    }
}

void BufferPool::flush_all() {
    for (auto it = file_to_buf.begin();it != file_to_buf.end();it++) {
        if (buf[it->second].dirty()) {
            io.write_to_file(buf[it->second].file, buf[it->second].page_offset, buf[it->second].content());
            buf[it->second].isDirty = false;
        }
    }
}

void BufferPool::reset(String file, size_t page_offset) {
    String temp = name_mapping(file, page_offset);
    if(file_to_buf.find(temp)!=file_to_buf.end()){
        auto fetched_id = file_to_buf[temp];
        buf[fetched_id].reset();
        file_to_buf.erase(file_to_buf.find(temp));
    }
}

void BufferPool::reset(String file) {
    int sz = file_size(file);
    for (int i = 0;i < sz;i++) {
        reset(file, i);
    }
}