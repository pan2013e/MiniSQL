#include "catalog.h"
#include "buffer.h"

extern BufferPool catalog_buf,data_buf;

namespace CTLOG {
    static bool check_file_exists(String file_name){
        std::ifstream check(file_name,std::ios::binary);
        if (check.is_open()) {
            check.close();
            return true;
        }
        check.close();
        return false;
    }
    bool check_table(String& name) {
        return check_file_exists(T_NAME(name));
    }
    bool check_index(String& i_name) {
        return check_file_exists(I_NAME(i_name));
    }
    bool check_attr_exists(String& t_name, String& att_name) {
        byte data[BlockSize + 1] = { 0 };
        memcpy(data, catalog_buf.get_data(T_NAME(t_name), 0), BlockSize);
        String t_info(data);
        std::stringstream strcin;
        strcin << t_info;
        int cnt = 0;
        strcin >> cnt;
        for (int i = 0;i < cnt;i++) {
            String att[3];
            strcin >> att[0] >> att[1] >> att[2];
            if (att[0] == att_name) return true;
        }
        return false;
    }
    bool check_attr_unique(String& t_name, String& att_name) {
        byte data[BlockSize + 1] = { 0 };
        memcpy(data, catalog_buf.get_data(T_NAME(t_name), 0), BlockSize);
        String t_info(data);
        std::stringstream strcin;
        strcin << t_info;
        int cnt = 0;
        strcin >> cnt;
        for (int i = 0;i < cnt;i++) {
            String att[3];
            strcin >> att[0] >> att[1] >> att[2];
            if (att[0] == att_name) {
                if (atoi(att[1].c_str()) > 1) return true;
            }
        }
        return false;
    }
    void get_property(String& t_name, int unique[], int type[]) {
        if(unique)
            memset(unique, 0, sizeof(int) * 32);
        if(type)
            memset(type, 0, sizeof(int) * 32);
        byte data[BlockSize + 1] = { 0 };
        memcpy(data, catalog_buf.get_data(T_NAME(t_name), 0), BlockSize);
        String t_info(data);
        std::stringstream strcin;
        strcin << t_info;
        int cnt = 0;
        strcin >> cnt;
        assert(cnt < 32);
        for (int i = 0;i < cnt; i++) {
            String att[3];
            strcin >> att[0] >> att[1] >> att[2];
            if(unique)  unique[i] = (atoi(att[1].c_str()) > 1) ? 1 : 0;
            if(type)    type[i] = (att[2] == "int") ? -1 : ((att[2] == "float") ? 0 : atoi(att[2].c_str()));
        }
    }
    void create_table(String& name, AttrArr& att) {
        if (check_table(name)) {
            String err_msg = "Table '" + name + "' already exists";
            throw SQLException(err_msg,CATALOG);
        } else {
            std::stringstream wt;
            wt << att.size() << '\n';
            for(auto it=att.begin();it!=att.end();it++){
                wt << it->first << ' ' << it->second.first << ' ' << it->second.second << '\n';
            }
            assert(wt.str().length() <= BlockSize);
            byte data[BlockSize]={0};
            memcpy(data,wt.str().c_str(),wt.str().length());
            catalog_buf.modify_data(T_NAME(name),0,data);
            catalog_buf.flush(T_NAME(name),0);
        }
    }

    static void check_index_and_delete(String t_name, String i_name) {
        String fname(i_name),rd_t_name;
        std::ifstream rd(I_NAME(i_name), std::ios::binary);
        rd >> rd_t_name;
        rd.close();
        if (rd_t_name == t_name) {
            String rm_i_name = I_NAME(i_name);
            catalog_buf.reset(rm_i_name);
            ::remove(rm_i_name.c_str());
        }
    }
    void drop_table(String& name) {
        if (check_table(name)) {
            String data = DATA(name);
            String file = T_NAME(name);
            catalog_buf.reset(file);
            data_buf.reset(data);
            if (::remove(file.c_str())) throw SQLException("Unable to drop table due to system issues", ACCESS);
            if (::remove(data.c_str())) throw SQLException("Unable to drop table due to system issues", ACCESS);
            #ifdef __APPLE__
            struct dirent* dirp;
            String temp = I_DIR;
            DIR* dir = opendir(temp.c_str());
            while ((dirp = readdir(dir)) != nullptr) {
                if (dirp->d_type == DT_REG) {
                    String fname(dirp->d_name);
                    check_index_and_delete(name, fname);
                }
            }
            closedir(dir);
            #else
            const char* to_search=".meta\\index\\*";
            long handle;
            struct _finddata_t fileinfo;
            handle = _findfirst(to_search, &fileinfo);
            if (handle == -1)  return;
            check_index_and_delete(name, fileinfo.name);
            while (!_findnext(handle, &fileinfo)){
                check_index_and_delete(name, fileinfo.name);
            }
            _findclose(handle);  
            #endif
        } else {
            String err_msg = "Table '" + name + "' does not exist";
            throw SQLException(err_msg,CATALOG);
        }
    }
    void drop_index(String& name) {
        if (check_index(name)) {
            String file = I_NAME(name);
            catalog_buf.reset(file);
            if (::remove(file.c_str())) throw SQLException("Unable to drop table due to system issues", ACCESS);
        } else {
            String err_msg = "Index '" + name + "' does not exist";
            throw SQLException(err_msg,CATALOG);
        }
    }
    void create_index(String i_name, String& t_name, String& att_name) {
        if (!check_table(t_name)) {
            String err_msg = "Table '" + t_name + "' does not exist";
            throw SQLException(err_msg,CATALOG);
        }
        if (check_index(i_name)) {
            String err_msg = "Index '" + i_name + "' already exists";
            throw SQLException(err_msg,CATALOG);
        } else {
            if (!check_attr_exists(t_name, att_name)) {
                String err_msg = "Attribute '" + att_name + "' does not exist";
                throw SQLException(err_msg, CATALOG);
            }
            if (!check_attr_unique(t_name, att_name)) {
                String err_msg = "Attribute '" + att_name + "' is not single-valued";
                throw SQLException(err_msg, CATALOG);
            }
            std::stringstream wt;
            wt << t_name << ' ' << att_name << '\n';
            byte data[BlockSize]={0};
            memcpy(data, wt.str().c_str(), wt.str().length());
            srand(time(NULL));
            for (int i = wt.str().length();i < 4096;i++) {
                data[i] = rand() % 255;
            }
            catalog_buf.modify_data(I_NAME(i_name), 0, data);
            catalog_buf.flush(I_NAME(i_name),0);
        }
    }
    int get_attr_cnt(String& t_name) {
        byte data[BlockSize + 1] = { 0 };
        memcpy(data, catalog_buf.get_data(T_NAME(t_name), 0), BlockSize);
        String t_info(data);
        std::stringstream strcin;
        strcin << t_info;
        int cnt = 0;
        strcin >> cnt;
        assert(cnt < 32);
        return cnt;
    }
    void get_attr_name(String& t_name, std::vector<String>& attr) {
        byte data[BlockSize + 1] = { 0 };
        memcpy(data, catalog_buf.get_data(T_NAME(t_name), 0), BlockSize);
        String t_info(data);
        std::stringstream strcin;
        strcin << t_info;
        int cnt = 0;
        strcin >> cnt;
        assert(cnt < 32);
        for (int i = 0;i < cnt; i++) {
            String att[3];
            strcin >> att[0] >> att[1] >> att[2];
            attr.emplace_back(att[0]);
        }
    }
    int get_attr_id(String& t_name, String& attr) {
        byte data[BlockSize + 1] = { 0 };
        memcpy(data, catalog_buf.get_data(T_NAME(t_name), 0), BlockSize);
        String t_info(data);
        std::stringstream strcin;
        strcin << t_info;
        int cnt = 0;
        strcin >> cnt;
        assert(cnt < 32);
        for (int i = 0;i < cnt; i++) {
            String att[3];
            strcin >> att[0] >> att[1] >> att[2];
            if (att[0] == attr) return i;
        }
        return -1;
    }
}
