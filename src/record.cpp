#include "record.h"
#include "catalog.h"
#include "buffer.h"
#include "index.h"
#include <stdlib.h>
#include <stdint.h>

extern BufferPool data_buf;

namespace RECORD{

void int_to_4B(int32_t t, char temp[4]) {
    memcpy(temp, &t, sizeof(int32_t));
} 

int32_t _4B_to_int(char temp[4]) {
    int32_t t;
    memcpy(&t, temp, sizeof(int32_t));
    return t;
}

int char4_to_int(char temp[4]){
    int t=0;
    for(int i=0;i<4;i++){
        t=t*10+(temp[i]-'0');
    }
    return t;
}

void resetSize(byte* t, int size){
    t[0] = size/1000 +'0';
    t[1] = size%1000/100 + '0';
    t[2] = size%100/10 + '0';
    t[3] = size%10+'0';
}

void double_to_8B(double d,char temp[8]) {
    memcpy(temp, &d, sizeof(double));
}

double _8B_to_double(char temp[8]) {
    double d;
    memcpy(&d, temp, sizeof(double));
    return d;
}


int attribute_size(int a){
    switch(a){
        case -1: return 4;
        case 0: return 8;
        default: return a;
    }
}

void attribute_code(int type, String attribute, char* temp){
    switch(type){
        case -1: /*int*/ 
            int_to_4B(atoi(attribute.c_str()), temp);
            break;
        case 0: /*double*/
            double_to_8B(strtod(attribute.c_str(),NULL),temp);
            break;
        default:   /*char*/
            if(attribute.size()>type){
                String SqlException = "the length of "+ attribute + " is out of range.";
                throw SQLException(SqlException,REC);
            }
            for(int j=0;j<type;j++){
                if(j<attribute.size()) temp[j]=attribute[j];
                else temp[j] = '#';
            }
            break;
    }
}

String code_attribute(int type, char* temp){
    switch(type){
        case -1: /*int*/ 
            return std::to_string(_4B_to_int(temp));
        case 0: /*double*/
            return std::to_string(_8B_to_double(temp));
        default:   /*char*/
            String c = "";
            for(int j = 0; temp[j]!='#' && j<type; j++){
                c+=temp[j];
            }
            return c;
    }
}

bool IsRedundancy(String tablename, int type, String attribute, int first_position, int line_size){
    size_t block_num = data_buf.file_size( DATA(tablename));
    int current_size;
    int flag;
    byte* data;
    String temp;
    int sz=attribute_size(type);
    char * a = new char[sz+1];
    memset(a,0,sz+1);
    attribute_code(type, attribute,a);
    for(int i=0; i<block_num;i++){
        data = data_buf.get_data(DATA(tablename),i);
        current_size = char4_to_int(data);
        for(int position = first_position; position < current_size; position+=line_size){
            temp = "";
            for(int j=0;j<attribute_size(type);j++){
                temp += data[position+j];
            }
            flag = 0;
            for(int k=0;k<sz;k++) if(temp[k]!=a[k]) flag = 1;
            if(flag==0) return true;
        }
    }
    return false;
}

void Insert (String tablename ,int* type, std::vector<String>& attribute, int * unique){
    int i,size = attribute.size();
    int line_size = 1;
    int first_position=4;
    String c;
    String filename = DATA(tablename);
    /*get the size of one line*/
    for (i = 0; i < size; i++){
        line_size += attribute_size(type[i]);
    }
    /*check redundancy using index*/
    for (i = 0; i < size; i++) {
        if (unique[i] == 1) {
            if(IsRedundancy(tablename,type[i], attribute[i], first_position,line_size)){
                String SqlException = "attribute "+ attribute[i] + " has already been existed.";
                throw SQLException(SqlException,REC);
            }
            _INDEX::Insert(tablename,attribute[i],0);
        }else{
            first_position += attribute_size(type[i]);
        }
    }

    /*convert the attribute to char* */
    char* temp = new char[line_size+1];
    char * current = temp;
    for(i=0;i<size;i++){
        attribute_code(type[i],attribute[i],current);
        current += attribute_size(type[i]);
    }
    temp[line_size-1] = '0';
    /*find the position where the String should be added*/
    size_t current_offset = data_buf.file_size(filename);
    if (current_offset > 0) current_offset--;
    byte* current_data = data_buf.get_data(filename, current_offset);
    int current_size = char4_to_int(current_data);
    i = current_size;

    /*insert the data*/
    if(i+line_size<BlockSize){
        for(int j=0;j<line_size;j++){
            current_data[i+j] = temp[j];
        }
        current_size+=line_size;
        resetSize(current_data,current_size);
        data_buf.modify_data(filename, current_offset, current_data);
        data_buf.flush(filename, current_offset);
    } else {
        /*need new buff block*/
        byte data[BlockSize];
        for(int i=0;i<BlockSize;i++) data[i] = 0;
        for(int j=0;j<line_size;j++){
            data[j+4] = temp[j];
        }
        resetSize(data,4+line_size);
        data_buf.modify_data(filename, current_offset + 1, data);
        data_buf.flush(filename, current_offset + 1);
    }
}

Table SelectWithoutCondition(String tablename, int *type, int attribute_num){
    Table T;
    T.clear();
    char * temp;
    byte * data;
    std::vector<String> attribute;
    //String temp;
    size_t block_num = data_buf.file_size(DATA(tablename));
    int position;
    int line_size = 0;
    int attribute_offset;
    int current_size;
    //get the size of one line
    for (int i = 0; i < attribute_num; i++){
        line_size += attribute_size(type[i]);
    }
    for(int i=0; i<block_num;i++){
        position = 4;
        data = data_buf.get_data(DATA(tablename),i);
        current_size = char4_to_int(data);
        while(position+line_size < current_size ){
            attribute.clear();
            for(int j=0;j<attribute_num;j++){
                switch(type[j]){
                    case -1: attribute_offset = 4; break;
                    case 0: attribute_offset = 8; break;
                    default: attribute_offset = type[j]; break;
                }
                temp = new char[attribute_offset + 1];
                memset(temp, 0, attribute_offset + 1);
                memcpy(temp, data + position, attribute_offset);
                position+=attribute_offset;
                //std::cout << type[i];
                attribute.emplace_back(code_attribute(type[j], temp));
                delete[] temp;
                //attribute.emplace_back()
            }
            T.emplace_back(attribute);
            if (data[position] == '1') T.pop_back();
            position++;
        }
    }
    //std::cout << T.size();
    return T;
}

bool IsSatisfy(std::vector<String> attribute, Condition& C, int* type){
    String temp;
    for(int i=0; i<C.size(); i++){
        temp = attribute[C[i].a_id];
        switch(type[C[i].a_id]){
            case -1: {
                int tmp1,tmp2;
                tmp1 = atoi(temp.c_str());
                tmp2 = atoi(C[i].val.c_str());
                switch(C[i].symbol){
                    case Less: if(tmp1>=tmp2) return false; break;
                    case Less_Equal: if(tmp1>tmp2) return false; break;
                    case Equal: if(tmp1!=tmp2) return false; break;
                    case Greater_Equal: if(tmp1<tmp2) return false; break;
                    case Greater: if(tmp1<=tmp2) return false; break;
                    case Not_Equal: if(tmp1 == tmp2) return false; break;
                }
                break;
            }    
            case 0:{
                double tmp1,tmp2;
                tmp1 = strtod(temp.c_str(),NULL);
                tmp2 = strtod(C[i].val.c_str(),NULL);
                switch(C[i].symbol){
                    case Less: if(tmp1>=tmp2) return false; break;
                    case Less_Equal: if(tmp1>tmp2) return false; break;
                    case Equal: if(tmp1!=tmp2) return false; break;
                    case Greater_Equal: if(tmp1<tmp2) return false; break;
                    case Greater: if(tmp1<=tmp2) return false; break;
                    case Not_Equal: if(tmp1 == tmp2) return false; break;
                }
                break;
            }
            default:{
                String tmp1,tmp2;
                tmp1 = temp;
                tmp2 = C[i].val;
                switch(C[i].symbol){
                    case Less: if(tmp1>=tmp2) return false; break;
                    case Less_Equal: if(tmp1>tmp2) return false; break;
                    case Equal: if(tmp1!=tmp2) return false; break;
                    case Greater_Equal: if(tmp1<tmp2) return false; break;
                    case Greater: if(tmp1<=tmp2) return false; break;
                    case Not_Equal: if(tmp1 == tmp2) return false; break;
                }
                break;
            }

        }
    }
    return true;
}

Table SelectWithCondition(String tablename, int *type, int attribute_num, Condition& C){
    Table T;
    T.clear();
    char * temp;
    byte * data;
    int * unique = new int[attribute_num];
    std::vector<String> attribute;
    //String temp;
    size_t block_num = data_buf.file_size(DATA(tablename));
    int position;
    int line_size = 0;
    int attribute_offset;
    int current_size;
    //get the size of one line
    for (int i = 0; i < attribute_num; i++){
        line_size += attribute_size(type[i]);
    }
    for(int i=0; i<block_num;i++){
        data = data_buf.get_data(DATA(tablename),i);
        position = 4;
        current_size = char4_to_int(data);
        while(position+line_size < current_size ){
            attribute.clear();
            for(int j=0;j<attribute_num;j++){
                switch(type[j]){
                    case -1: attribute_offset = 4; break;
                    case 0: attribute_offset = 8; break;
                    default: attribute_offset = type[j]; break;
                }
                temp = new char[attribute_offset + 1];
                memset(temp, 0, attribute_offset + 1);
                memcpy(temp, data + position, attribute_offset);
                position+=attribute_offset;
                attribute.emplace_back(code_attribute(type[j], temp));
                delete[] temp;
                //attribute.emplace_back()
            }
            if(IsSatisfy(attribute, C, type)){
                T.emplace_back(attribute);
                if (data[position] == '1') T.pop_back();
            }
            position++;
        }
    }
    return T;
}

void Delete(String tablename, int *type, int attribute_num, Condition& C){
    char * temp;
    byte * data;
    std::vector<String> attribute;
    //String temp;
    size_t block_num = data_buf.file_size(DATA(tablename));
    int position;
    int line_size = 0;
    int attribute_offset;
    int current_size;
    //get the size of one line
    for (int i = 0; i < attribute_num; i++){
        line_size += attribute_size(type[i]);
    }
    for(int i=0; i<block_num;i++){
        data = data_buf.get_data(DATA(tablename),i);
        current_size = char4_to_int(data);
        position = 4;
        while(position+line_size < current_size){
            attribute.clear();
            for(int j=0;j<attribute_num;j++){
                switch(type[j]){
                    case -1: attribute_offset = 4; break;
                    case 0: attribute_offset = 8; break;
                    default: attribute_offset = type[j]; break;
                }
                temp = new char[attribute_offset + 1];
                memset(temp, 0, attribute_offset + 1);
                memcpy(temp, data + position, attribute_offset);
                position+=attribute_offset;
                attribute.emplace_back(code_attribute(type[j], temp));
                delete[] temp;
                //attribute.emplace_back()
            }
            if(IsSatisfy(attribute, C,type)){
                _INDEX::Delete(tablename,attribute[i],0);
                data[position] = '1';
            }
            position++;
        }
    }
}

void DeleteWithoutCondition(String tablename) {
    String file = DATA(tablename);
    data_buf.reset(file);
    remove(file.c_str());
    byte empty[BlockSize];
    for (int i = 0;i < BlockSize;i++) empty[i] = 0;
    resetSize(empty,4);
    data_buf.modify_data(DATA(tablename), 0, empty);
    data_buf.flush(DATA(tablename), 0);
    _INDEX::Drop(tablename,0);
}


}




