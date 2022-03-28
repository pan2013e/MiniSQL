/**
 * @file Interpreter.h
 * @author  pzy (zy_pan@zju.edu.cn)
 * @brief Header for SQL parser
 * @version 1.0
 * @date 2021-05-17
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#ifndef _INTERPRETER_
#define _INTERPRETER_

#include "pch.h"
#include "API.h"

/**
 * @brief fetch, store, parse SQL queries
 * 
 */
class Query {
public:
    explicit Query(String& s, bool in = false) : text(s), inside_exec(in) {}
    void exec_wrapper();
    void parse_wrapper();

private:
    Query() = default;
    Query(Query&) = default;
    void exec();
    void traceback();
    void parse();
    SQLText text;
    TokenArr tokens;
    SQLType type;
    bool inside_exec=false;
};

/**
 * @brief Console session
 * 
 */
class Session {
public:
    void start();
    void listen();
};

void trim(String&);

#endif