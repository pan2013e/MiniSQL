#ifndef _PCH_
#define _PCH_

#include <fstream>
#include <ostream>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <string>
#include <cstring>
#include <vector>
#include <memory>
#include <algorithm>
#include <regex>
#include <chrono>
#include <type_traits>
#include <stdexcept>
#include <list>
#include <map>
#include <sstream>
#include <unordered_map>
#include <sys/stat.h>
#include <cassert>
#include <future>
#ifdef __APPLE__
#include <dirent.h>
#else
#include <io.h>
#endif
#ifndef _WIN32
#include <unistd.h>
#endif
#include <stdarg.h>
#include <bitset>

#ifndef EXIT_FAILURE
#define EXIT_FAILURE -1
#endif

#ifdef _WIN32
const int BIN = 2;
const int OCT = 8;
const int DEC = 10;
const int HEX = 16;
#else
enum radix_type {
    BIN = 2,
    OCT = 8,
    DEC = 10,
    HEX = 16,
};
#endif

#if defined(_WIN32)||(_WIN64)
#define T_DIR ".meta\\schema\\"
#define I_DIR ".meta\\index\\"
#define D_DIR ".data\\"
#define T_NAME(x) ".meta\\schema\\"+(x)
#define I_NAME(x) ".meta\\index\\"+(x)
#define DATA(x) ".data\\"+(x)
#else
#define M_PREFIX ".meta"
#define __SCHEMA "schema"
#define __INDEX "index"
#define T_DIR std::string(M_PREFIX) + "/"+ __SCHEMA +"/"
#define I_DIR std::string(M_PREFIX) + "/" + __INDEX + "/"
#define D_DIR ".data/"
#define T_NAME(x) T_DIR+(x)
#define I_NAME(x) I_DIR+(x)
#define DATA(x) D_DIR+(x)
#endif

constexpr int INVALID_ID=-1;
constexpr int BlockSize=4096;

typedef std::string String;
typedef String SQLText;
typedef std::vector<String> TokenArr;
typedef std::pair<int,String> str_pair;
typedef std::pair<String,str_pair> att_pair;
// <name,<property,value_type>>
typedef std::list<att_pair> AttrArr;
typedef int block_id_t;
typedef int page_id_t;
typedef char byte;
typedef const char* Prompt;
typedef std::vector<std::vector<String>> Table;
enum Compare{Less,Less_Equal,Equal,Greater_Equal,Greater,Not_Equal};
typedef struct where{
    int a_id;
    Compare symbol;
    String val;
}WhereClause;

typedef std::vector<WhereClause> Condition;


enum SQLType {
    CREATE_TABLE,
    DROP_TABLE,
    CREATE_INDEX,
    DROP_INDEX,
    SELECT,
    INSERT,
    DELETE,
    QUIT,
    EXEC
};
enum Stage{
    DEFAULT,PARSE,CATALOG,INDEX,ACCESS,REC,CLEAR
};

/**
 * @brief exception used to prompt errors, derived from std class
 *
 */
class SQLException : public std::runtime_error {
public:
    explicit SQLException(const char *msg,Stage code=DEFAULT) : std::runtime_error(msg),code(code) {}
    explicit SQLException(String& msg,Stage code=DEFAULT) : std::runtime_error(msg),code(code) {}
    Stage errCode() const { return code; }
private:
    Stage code;
};

template<class T>
void ELAPSED_TIME(T&& func) {
	auto _start = std::chrono::steady_clock::now();
    func();
    auto _end=std::chrono::steady_clock::now();
	std::cout << "\033[1;36mQuery executed. Time: " << std::chrono::duration<double,std::milli>(_end-_start).count() << "(ms)." << std::endl;
}

#endif