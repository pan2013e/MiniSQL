#ifndef _INDEX_
#define _INDEX_

#include "pch.h"
#include "record.h"
#include "buffer.h"
#include <map>

namespace _INDEX {
    void CreateIndex(String& i_name, String& t_name, String& att_name);
    void Drop(String attributename, int flag);
    void Establish(String attributename, int type, std::vector<String>& key, int flag);
    void Insert(String attributename, String attribute, int flag);
    void Delete(String attributename, String attribute, int flag);
    Table Find(String attributename, String attribute, int flag);
}

#endif