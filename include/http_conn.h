#ifndef HTTP_CONN_H
#define HTTP_CONN_H

#define MAX_BUF 2048

#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <cstring>
#include <stdarg.h>
#include <unordered_map>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/uio.h>
#include <sys/socket.h>
#include "mysql_conn.h"

#define READ_EVENT 1
#define WRITE_EVENT 2

using namespace std;



void addfd(int epoll_fd, int fd, bool enable_et);

void modfd(int epoll_fd, int socket_fd, int EVENT, bool enable_et);

void setNonblock(int fd);

enum STATE{
    NO_REQUEST, GET_REQUEST, ERROR404, POST_REQUEST
};

enum ACTION{
    NO_POST, REGISTER, LOGIN, NEWUSR, REGUSR, ERROR_POST, GET_IMG, GET_VD, SUBSCRIBE
};


static unordered_map<int, const char*> http_code = 
{
    {200, "OK"},
    {404, "Not Found"}
};

class http_conn{

public:
    static int user_cnt;
    static int uid;
    http_conn(int socket_fd, int epoll_fd);
    ~http_conn();
    void init();
    STATE process_read();
    bool process_write();
    void process();
    bool read();
    bool write();

private:

    int socket_fd;
    int epoll_fd;
    char buffer[MAX_BUF];
    char m_write_buf[MAX_BUF];
    int m_write_idx;
    int m_read_idx;
    char* m_file_address;
    struct stat m_file_stat;
    struct iovec m_iv[2];
    int m_iv_count;
    STATE state;
    ACTION action;
    void parse_line(char* data, char* method, char* url, char* body);
    bool add_response(const char*format,...);
    bool add_status_line(int status,const char*title);
    bool add_headers(int content_len);
    bool add_content_length(int content_len);
    bool add_linger();
    bool add_blank_line();
    bool add_content(const char*content);
    bool map_file(const char* filepath);
    mysql_conn *db;
};



#endif