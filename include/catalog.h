#ifndef _CTLOG_
#define _CTLOG_

#include "pch.h"
#include <cstdio>
namespace CTLOG{
    bool check_table(String&);
    bool check_index(String&);
    void create_table(String&, AttrArr&);
    void create_index(String i_name, String& t_name, String& att_name);
    void drop_table(String&);
    void drop_index(String&);
    bool check_attr_unique(String& t_name, String& att_name);
    bool check_attr_exists(String& t_name, String& att_name);
    void get_property(String& t_name, int unique[], int type[]);
    void get_attr_name(String& t_name, std::vector<String>& attr);
    int get_attr_cnt(String& t_name);
    int get_attr_id(String& t_name, String& attr);
}

#endif