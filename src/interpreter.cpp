#include "interpreter.h"
#include "buffer.h"
#include "readline/readline.h"
#include "readline/history.h"

Prompt WelcomeMSG = "Welcome to the MiniSQL monitor.  Commands end with ; .";
Prompt Console = "\033[1;34mMiniSQL> \033[0m";
const char* TypeStr[] = {
    "CREATE_TABLE",
    "DROP_TABLE",
    "CREATE_INDEX",
    "DROP_INDEX",
    "SELECT",
    "INSERT",
    "DELETE",
    "QUIT",
    "EXECFILE"
};
const char* StageStr[]={
    "Undefined","Parser","Catalog","Index","Internal","Record"
};
const char* clear = "clear";
const char* quit = "quit";
const char* del = "delete from (\\w+)[ ]*(where.+)?";
const char* _select = "select \\* from (\\w+)[ ]*(where.+)?";
const char* create = R"(create table (\w+)[ ]*(\(.+\)))";
const char* insert = R"(insert into (\w+)[ ]*values[ ]*(\(.+\)))";
const char* drop = "drop table (\\w+)";
const char* execfile = "execfile ([.\\w/]+)";
const char* drop_index = "drop index (\\w+)";
const char* create_index = R"(create index (\w+)[ ]*on[ ]*(\w+)[ ]*(\([ ]*\w+[ ]*\)))";

extern BufferPool data_buf, catalog_buf;

using std::regex;
using std::regex_match;
using std::regex_search;
using std::smatch;

int eoq = 0;

int bind_cr(int count, int key) {
    printf("\n");
    if (eoq == 1) {
        rl_done = 1;
        eoq = 0;
    } else {
        printf("\033[1;34m       > \033[0m");
    }
    return 0;
}

int bind_eoq(int count, int key) {
    eoq = 1;
    printf(";");
    return 0;
}

int readline_startup() {
    rl_bind_key('\n', bind_cr);
    rl_bind_key('\r', bind_cr);
    rl_bind_key(';', bind_eoq);
    return 0;
}

void Session::start() {
    printf("%s\n", WelcomeMSG);
    if (isatty(STDIN_FILENO)) {
        rl_readline_name = "MiniSQL";
        rl_startup_hook = readline_startup;
    }
}

void Session::listen() {
    while (1) {
        char* line = readline(Console);
        add_history(line);
        String buf(line);
        Query q(buf);
        try {
            q.parse_wrapper();
        }
        catch (const SQLException& e) {
            if (e.errCode() == CLEAR) continue;
            printf("\033[1;31m%s Error: %s\n\033[0m", StageStr[e.errCode()], e.what());
            continue;
        }
        q.exec_wrapper();
    }
}

/**
 * @brief trim surplus spaces,linefeeds and tabs
 * 
 * @param s string to be trimmed
 */
void trim(String& s) {
    if (s.empty())
        return;
    for (char & it : s)
        if (it == '\n' || it == '\t') it = ' ';
    s.erase(0, s.find_first_not_of(' '));
    s.erase(s.find_last_not_of(' ') + 1);
    String::iterator next;
    for (String::iterator it = s.begin();it != s.end();it = next) {
        next = ++it;
        it--;
        if (*it == ' ' && *next == ' ') {
            next = s.erase(it);
        }
    }
}

/**
 * @brief split string into tokens according to regex pattern
 * 
 * @param tokens 
 * @param s 
 * @param pat std::regex pattern
 */
static void token_split(TokenArr& tokens, String& s, regex& pat) {
    std::sregex_token_iterator p(s.begin(), s.end(), pat, -1);
    std::sregex_token_iterator end;
    while (p != end) {
        String temp = *p;
        trim(temp);
        tokens.emplace_back(temp);
        p++;
    }
}

/**
 * @brief encode tokens into json type for debug purpose
 * 
 */
void Query::traceback()  {
    printf("\033[1;31mTraceback:\n");
    printf("{\n    \"type\" : \"%s\",\n", TypeStr[type]);
    printf("    \"tokens\" : [\n");
    for (int i = 0;i < tokens.size();i++) {
        printf("        \"%s\",\n", tokens[i].c_str());
    }
    printf("    ],\n}\n\033[0m");
}

/**
 * @brief parse SQL query
 * 
 */
void Query::parse() {
    regex r_del(del), r_ins(insert), r_create(create), r_quit(quit), r_drop(drop), r_dropind(drop_index), r_createind(create_index), r_exec(execfile), r_sel(_select),r_clear(clear);
    if (regex_match(text, r_create)) {
        type = CREATE_TABLE;
        smatch sm;
        regex_search(text, sm, r_create);
        tokens.emplace_back(sm[1]);
        String temp = sm[2];
        temp.erase(0, temp.find_first_not_of('('));
        temp = temp.substr(0, temp.length() - 1);
        trim(temp);
        if (temp.empty()) throw SQLException("Parameters required in '( )'",PARSE);
        regex comma(",");
        token_split(tokens, temp, comma);
    } else if (regex_match(text, r_drop)) {
        type = DROP_TABLE;
        smatch sm;
        regex_search(text, sm, r_drop);
        tokens.emplace_back(sm[1]);
    } else if (regex_match(text, r_createind)) {
        type = CREATE_INDEX;
        smatch sm;
        regex_search(text, sm, r_createind);
        tokens.emplace_back(sm[1]);
        tokens.emplace_back(sm[2]);
        String temp = sm[3];
        temp.erase(0, temp.find_first_not_of('('));
        temp.erase(temp.find_last_not_of(')') + 1);
        trim(temp);
        tokens.emplace_back(temp);
    } else if (regex_match(text, r_dropind)) {
        type = DROP_INDEX;
        smatch sm;
        regex_search(text, sm, r_dropind);
        tokens.emplace_back(sm[1]);
    } else if (regex_match(text, r_sel)) {
        type = SELECT;
        smatch sm;
        regex_search(text, sm, r_sel);
        tokens.emplace_back(sm[1]);
        if (sm[2] != "") {
            String cond = sm[2];
            cond = cond.substr(6, cond.length());
            regex r_and("and");
            token_split(tokens, cond, r_and);
        }
    } else if (regex_match(text, r_ins)) {
        type = INSERT;
        smatch sm;
        regex_search(text, sm, r_ins);
        tokens.emplace_back(sm[1]);
        String temp = sm[2];
        temp.erase(0, temp.find_first_not_of('('));
        temp.erase(temp.find_last_not_of(')') + 1);
        trim(temp);
        if (temp.empty()) throw SQLException("Parameters required in 'values( )'",PARSE);
        regex comma(",");
        token_split(tokens, temp, comma);
    } else if (regex_match(text, r_del)) {
        type = DELETE;
        smatch sm;
        regex_search(text, sm, r_del);
        tokens.emplace_back(sm[1]);
        if (sm[2] != "") {
            String cond = sm[2];
            cond = cond.substr(6, cond.length());
            regex r_and("and");
            token_split(tokens, cond, r_and);
        }
    } else if (regex_match(text, r_quit)) {
        type = QUIT;
        printf("Bye\n");
        catalog_buf.flush_all();
        data_buf.flush_all();
        exit(0);
    } else if (regex_match(text, r_exec)) {
        type = EXEC;
        smatch sm;
        regex_search(text, sm, r_exec);
        tokens.emplace_back(sm[1]);
    } else if (regex_match(text, r_clear)) {
        #ifndef _WIN32
        system("clear");
        #else
        system("cls");
        #endif
        throw SQLException("", CLEAR);
    } else {
        String errmsg = "Undefined SQL syntax in '" + text + "'";
        throw SQLException(errmsg,PARSE);
    }
    #ifdef DEBUG
    traceback();
    #endif
}

void Query::parse_wrapper() {
    trim(text);
    if (!text.length()) return;
    parse();
}

void Query::exec_wrapper() {
    if(text.empty()) return;
    try {
        if(type==EXEC || !inside_exec)
            ELAPSED_TIME([&] { exec(); });
        else exec();
    }
    catch (const SQLException &exec_except) {
        printf("\033[1;31m%s Error: %s\n\033[0m",StageStr[exec_except.errCode()],exec_except.what());
        // traceback();
    }
}
/**
 * @brief transfer control to API and execute query
 * 
 */
void Query::exec() {
    if (type == EXEC) {
        static String fname="";
        if (inside_exec && tokens[0] == fname) throw  SQLException("Recursive execution of the same file is forbidden", ACCESS);
        std::ifstream sql_in(tokens[0]);
        if (!sql_in.is_open()) {
            String err_msg="Unable to open file '"+tokens[0]+"', please check existence or permission";
            throw SQLException(err_msg.c_str(),ACCESS);
        } else {
            fname = tokens[0];
            while (!sql_in.eof()) {
                String buf;
                std::getline(sql_in, buf, ';');
                Query q(buf,true);
                try {
                    q.parse_wrapper();
                }
                catch (const SQLException& e) {
                    if (e.errCode() == CLEAR) continue;
                    printf("\033[1;31mParser Error: %s\n\033[0m", e.what());
                    continue;
                }
                q.exec_wrapper();
            }
        }
    } else {
        API::exec(tokens, type);
    }
}
