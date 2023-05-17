#ifndef MYSQL_CONNECTION_H
#define MYSQL_CONNECTION_H

#include "mysql/mysql.h"
#include <iostream>
#include <vector>

using namespace std;

class mysql_conn
{
public:
    mysql_conn(const char* user, const char* pwd, const char* db_name);
    ~mysql_conn();
    bool initDB(const char* host,const char* user,const char* pwd,const char* db_name, unsigned int port); //连接mysql
    bool exeSQL(string sql);   //执行sql语句
    void closeDB();
    vector<vector<string>> process_data();
    const char* db_name;
    const char* pwd;
    const char* user;
private:
    MYSQL mysql;          //连接mysql句柄指针
    MYSQL_RES *result;    //指向查询结果的指针
};

#endif