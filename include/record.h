#ifndef _RECORD_
#define _RECORD_

#include "pch.h"
#include <stdlib.h>
#include <stdint.h>

namespace RECORD_old {

String int_32bit(int t);

String double_to_64bit(double d);

int _32bit_to_int(String s);

double _64bit_to_double(String s);

int attribute_size(int a);

inline String attribute_code(int type, String attribute);

inline String code_attribute(int type, String code);

bool IsRedundancy(String tablename, int type, String attribute, int first_position, int line_size);

void Insert(String tablename, int* type, std::vector<String>& attribute, int* unique);

Table SelectWithoutCondition(String tablename, int* type, int attribute_num);

bool IsSatisfy(std::vector<String> attribute, Condition& C, int* type);

Table SelectWithCondition(String tablename, int *type, int attribute_num, Condition& C);

void Delete(String tablename, int *type, int attribute_num, Condition& C);

void DeleteWithoutCondition(String tablename);
}

namespace RECORD {

void int_to_4B(int32_t t, char temp[4]);

int32_t _4B_to_int(char temp[4]);

void double_to_8B(double d,char temp[8]);

double _8B_to_double(char temp[8]);

int attribute_size(int a);

int char4_to_int(char temp[4]);

void resetSize(byte* t, int size);

void attribute_code(int type, String attribute, char* temp);

String code_attribute(int type, char* temp);

bool IsRedundancy(String tablename, int type, String attribute, int first_position, int line_size);

void Insert(String tablename, int* type, std::vector<String>& attribute, int* unique);

Table SelectWithoutCondition(String tablename, int* type, int attribute_num);

bool IsSatisfy(std::vector<String> attribute, Condition& C, int* type);

Table SelectWithCondition(String tablename, int *type, int attribute_num, Condition& C);

void Delete(String tablename, int *type, int attribute_num, Condition& C);

void DeleteWithoutCondition(String tablename);
}
#endif
