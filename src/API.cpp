#include "API.h"
#include "catalog.h"
#include "interpreter.h"
#include "buffer.h"
#include "record.h"

const char* attr_token="(\\w+) (\\w+)[ ]*(\\([0-9]+\\))?[ ]*(unique)?";
const char* prim_token = "primary key[ ]*(\\([\\w ]+\\))";
const char* ineq_token = "(\\w+)[ ]*([><=]+)[ ]*([.0-9'a-zA-Z]+)";
const char* ins_int = "[ ]*[0-9]+[ ]*";
const char* ins_flt = "[ ]*[0-9]+.?[0-9]*[ ]*";
const char* ins_chr = "[ ]*'([\\w ]+)'[ ]*";

extern BufferPool catalog_buf;
extern BufferPool data_buf;

namespace API {
    using std::regex;
    using std::smatch;
    using std::regex_match;
    using std::regex_search;
    void create_table(TokenArr& tokens) {
        std::map<String,int> property;
        static regex r_attr(attr_token),r_prim(prim_token);
        smatch sm;
        bool has_prim=false;
        String pri_key;
        AttrArr attrs;
        for(int i=1;i<tokens.size();i++){
            if(regex_match(tokens[i],r_prim)){
                if(has_prim) throw SQLException("Creating two or more primary keys is not allowed",CATALOG);
                regex_search(tokens[i],sm,r_prim);
                pri_key=sm[1];
                pri_key.erase(0, pri_key.find_first_not_of('('));
                pri_key = pri_key.substr(0, pri_key.length() - 1);
                trim(pri_key);
                property[pri_key]=3;
                has_prim=true;
            }else if(!regex_match(tokens[i],r_attr))
                throw SQLException("Invalid attribute syntax",PARSE);
        }
        for(int i=1;i<tokens.size();i++){
            if(regex_match(tokens[i],r_attr)){
                regex_search(tokens[i],sm,r_attr);
                if(sm[2]!="int" && sm[2]!="char" && sm[2]!="float"){
                    throw SQLException("Invalid value type",PARSE);
                }
                String temp=sm[3];
                if(!temp.empty()){
                    temp.erase(0, temp.find_first_not_of('('));
                    temp = temp.substr(0, temp.length() - 1);
                    trim(temp);  
                }
                if((sm[2]=="int" || sm[2]=="float") && !temp.empty()){
                    throw SQLException("Integer or float types cannot have customize sizes",PARSE);
                }
                if(property.find(sm[1])==property.end())
                    property[sm[1]]=(sm[4]=="unique")?2:0;
                if(sm[2]=="int" || sm[2]=="float"){
                    attrs.emplace_back(att_pair(sm[1],str_pair(property[sm[1]],sm[2])));
                } else {
                    if (atoi(temp.c_str()) > 255 || atoi(temp.c_str()) <= 0) throw SQLException("Invalid size for char type", CATALOG);
                    temp = std::to_string(atoi(temp.c_str()) + 2);
                    attrs.emplace_back(att_pair(sm[1], str_pair(property[sm[1]], temp)));
                }
            }
        }
        if(!attrs.size()) throw SQLException("Require one or more attributes",CATALOG);
        if(has_prim){
            bool find=false;
            for(auto it=attrs.begin();it != attrs.end();it++){
                if(it->first==pri_key){
                    find=true;
                    break;
                }
            }
            if(!find) {
                String err_msg="Undefined reference to primary key '"+pri_key+"'";
                throw SQLException(err_msg,CATALOG);
            }
        }
        CTLOG::create_table(tokens[0], attrs);
        byte empty[BlockSize] = { 0 };
        empty[0] = empty[1] = empty[2] = '0';
        empty[3] = '4';
        data_buf.modify_data(DATA(tokens[0]), 0, empty);
        data_buf.flush(DATA(tokens[0]), 0);
        if (has_prim) CTLOG::create_index(tokens[0] + "_pri", tokens[0], pri_key);
    }

    void insert(TokenArr& tokens) {
        static String fname;
        static int type[32], unique[32];
        static regex r_int(ins_int), r_flt(ins_flt), r_chr(ins_chr);
        std::vector<String> attr;
        if (!CTLOG::check_table(tokens[0])) {
            String err_msg = "Table '" + tokens[0] + "' does not exist";
            throw SQLException(err_msg, CATALOG);
        }
        if (fname != tokens[0]) {
            CTLOG::get_property(tokens[0], unique, type);
            fname = tokens[0];
        }
        for (int i = 1;i < tokens.size();i++) {
            if (type[i - 1] == -1 && !regex_match(tokens[i], r_int)) {
                String err_msg = "Invalid int value '" + tokens[i] + "'";
                throw SQLException(err_msg, REC);
            }
            if (type[i - 1] == 0 && !regex_match(tokens[i], r_flt)) {
                String err_msg = "Invalid float value '" + tokens[i] + "'";
                throw SQLException(err_msg, REC);
            }
            if (type[i - 1] > 0 && !regex_match(tokens[i], r_chr)) {
                throw SQLException("Invalid char[] value", REC);
            }
            attr.emplace_back(tokens[i]);
        }
        RECORD::Insert(tokens[0], type, attr, unique);
    }

    void print_table(Table& t,std::vector<String>& head){
        if(t.empty()) {
            printf("(NULL)\n");
            return;
        }
        int i,j,k;
        int m = t[0].size();
        int n = t.size();
        int max[m];
        for(j=0;j<m;j++){
            max[j] = head[j].size();
            for(i=0;i<n;i++){
                if(t[i][j].size()>max[j]){
                    max[j]=t[i][j].size();
                }
            }
        }
        int len=0;
        for(j=0;j<m;j++){
            len+=max[j];
        }
        if(t[0].size()<=1){
            len+=2;
        }else{
            len+=3;
        }

        len+=2*(t[0].size());
        int pos=0;
        int q=0;
        for(k=0;k<=len;k++){
            if(k==pos){
                printf("+");
                pos+=3;
                pos+=max[q++];
            }else{
                if(t[0].size()!=1 || k!=len){
                    printf("-");
                }
            }
        }
        printf("\n");
        for(j=0;j<m;j++){
            printf("| %s",head[j].c_str());
            for(k=0;k<max[j]-head[j].length()+1;k++){
                printf(" ");
            }
        }
        printf("|\n");
        for(i=0;i<n;i++){
            pos=0;
            q=0;
            for(k=0;k<=len;k++){
                if(k==pos){
                    printf("+");
                    pos+=3;
                    pos+=max[q++];
                }else{
                    if(t[0].size()!=1||k!=len){
                        printf("-");
                    }
                }
            }
            printf("\n");
            for(j=0;j<m;j++){
                printf("| %s",t[i][j].c_str());
                for(k=0;k<max[j]-t[i][j].length()+1;k++){
                    printf(" ");
                }
            }
            printf("|\n");
        }
        pos=0;
        q=0;
        for(k=0;k<=len;k++){
            if(k==pos){
                printf("+");
                pos+=3;
                pos+=max[q++];
            }else{
                if(t[0].size()!=1||k!=len){
                    printf("-");
                }
            }
        }
        printf("\n");
        std::cout<<t.size()<<" line(s)"<<std::endl;
    }
    
    void select_or_delete(TokenArr& tokens, bool select) {
        if (!CTLOG::check_table(tokens[0])) {
            String err_msg = "Table '" + tokens[0] + "' does not exist";
            throw SQLException(err_msg, CATALOG);
        }
        int type[32], unique[32];
        CTLOG::get_property(tokens[0], NULL, type);
        Table T;
        if (tokens.size() == 1) {
            if (select)
                T = RECORD::SelectWithoutCondition(tokens[0], type, CTLOG::get_attr_cnt(tokens[0]));
            else
                RECORD::DeleteWithoutCondition(tokens[0]);
        }
        else {
            Condition c;
            for (int i = 1;i < tokens.size();i++) {
                WhereClause temp;
                regex ineq(ineq_token);
                if (regex_match(tokens[i], ineq)) {
                    smatch sm;
                    regex_search(tokens[i], sm, ineq);
                    temp.val = sm[3];
                    if (sm[2] == ">") {
                        temp.symbol = Greater;
                    }
                    else if (sm[2] == ">=") {
                        temp.symbol = Greater_Equal;
                    }
                    else if (sm[2] == "<") {
                        temp.symbol = Less;
                    }
                    else if (sm[2] == "<=") {
                        temp.symbol = Less_Equal;
                    }
                    else if (sm[2] == "=") {
                        temp.symbol = Equal;
                    }
                    else if (sm[2] == "<>") {
                        temp.symbol = Not_Equal;
                    } else {
                        throw SQLException("Invalid conditional statement", PARSE);
                    }
                    String att = sm[1].str();
                    if (!CTLOG::check_attr_exists(tokens[0],att)) {
                        String err_msg = "Attribute '" + att + "' does not exist";
                        throw SQLException(err_msg, CATALOG);
                    } else {
                        temp.a_id = CTLOG::get_attr_id(tokens[0], att);
                    }
                    c.emplace_back(temp);
                } else {
                    throw SQLException("Invalid conditional statement", PARSE);
                }
            }
            if (select)
                T = RECORD::SelectWithCondition(tokens[0], type, CTLOG::get_attr_cnt(tokens[0]), c);
            else
                RECORD::Delete(tokens[0], type, CTLOG::get_attr_cnt(tokens[0]), c);
        }
        if (select) {
            std::vector<String> head;
            CTLOG::get_attr_name(tokens[0], head);
            print_table(T, head);
        }
    }
    
    void select(TokenArr& tokens) {
        select_or_delete(tokens, true);
    }

    void create_index(TokenArr& tokens) {
        CTLOG::create_index(tokens[0], tokens[1], tokens[2]);
    }

    void drop_table(TokenArr& tokens) {
        CTLOG::drop_table(tokens[0]);
    }

    void drop_index(TokenArr& tokens) {
        CTLOG::drop_index(tokens[0]);
    }

    void _delete(TokenArr& tokens) {
        select_or_delete(tokens, false);
    }

    void (*api_exec[])(TokenArr&) = {
        create_table, drop_table, create_index, drop_index,
        select, insert, _delete,
    };

    void exec(TokenArr& tokens, SQLType& type) {
        assert(type < 7);
        api_exec[type](tokens);
    }
}
