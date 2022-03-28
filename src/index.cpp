#include "index.h"
// #include "BPlusTree.h"

extern BufferPool data_buf;

namespace _INDEX{

void CreateIndex(String attributename, int type, std::vector<String>& key, int flag){
    /*创建一个索引文件，并且将当前的索引值按从小到大的顺序写入索引文件中*/
    if(flag){
        String filename = DATA(attributename);
        int line_size = RECORD::attribute_size(type);
        /*get the index data from file*/
        int size,i;
        /*convert the attribute to char* */
        char* temp = new char[line_size+1];
        char * current = temp;
        /*find the position where the String should be added*/
        size_t current_offset = data_buf.file_size(filename);
        if (current_offset > 0) current_offset--;
        byte* current_data = data_buf.get_data(filename, current_offset);
        int current_size = RECORD::char4_to_int(current_data);
        i = current_size;
        /*insert the data*/
        if(i+line_size<BlockSize){
            for(int j=0;j<line_size;j++){
                current_data[i+j] = temp[j];
            }
            current_size+=line_size;
            data_buf.modify_data(filename, current_offset, current_data);
            data_buf.flush(filename, current_offset);
        } else {
            /*need new buff block*/
            byte data[BlockSize];
            for(int i=0;i<BlockSize;i++) data[i] = 0;
            for(int j=0;j<line_size;j++){
                data[j+4] = temp[j];
            }
            data_buf.modify_data(filename, current_offset + 1, data);
            data_buf.flush(filename, current_offset + 1);
        }
    }
}
void Drop(String attributename, int flag){
    //删除索引文件
    if(flag){
        String file = DATA(attributename);
        data_buf.reset(file);
        remove(file.c_str());
        byte empty[BlockSize];
        for (int i = 0;i < BlockSize;i++) empty[i] = 0;
        data_buf.modify_data(DATA(attributename), 0, empty);
        data_buf.flush(DATA(attributename), 0);
    }
}
void Insert(String attributename, String attribute, int flag){
    //在索引文件中插入新键值
    if(flag){
        // BPTree T = FindTree<int>(attributename);
        // T.Insert(attribute);
    }
}
void Delete(String attributename, String attribute, int flag){
    //删除索引文件中键值
    if(flag){
        // BPTree T = FindTree<int>(attributename);
        // T.Delete(attribute);
    }
}

Table Find(String attributename, String attribute, int flag){
    Table t;
    if(flag){
        // BPTree T = FindTree<int>(attributename);
        // T.Search(attribute , attributename, &t);
    }
    return t;
}

}