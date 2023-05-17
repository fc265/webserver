#include "http_conn.h"

using namespace std;

void addfd(int epoll_fd, int fd, bool enable_et){
    enable_et = false;
    epoll_event ev;
    ev.events = EPOLLIN;
    if(enable_et) ev.events |= EPOLLET;
    ev.data.fd = fd;
    if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev) == -1){
        cout << "epoll add error" << endl;
        throw exception();
    }
}

void modfd(int epoll_fd, int socket_fd, int EVENT, bool enable_et){
    epoll_event *ev = new epoll_event();
    ev->events = EVENT;
    ev->data.fd = socket_fd;
    if(enable_et) ev->events |= EPOLLET;
    if(epoll_ctl(epoll_fd, EPOLL_CTL_MOD, socket_fd, ev) == -1){
        cout << "epoll modify error" << endl;
        throw exception();
    }
}


void setNonblock(int fd){
    int flags;
    if ((flags = fcntl(fd, F_GETFL, NULL)) < 0) {
        throw exception();
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        throw exception();
    }
}



int http_conn::user_cnt = 0;
int http_conn::uid = 10;

http_conn::http_conn(int socket_fd, int epoll_fd){
    this->socket_fd = socket_fd;
    this->epoll_fd = epoll_fd;
    addfd(epoll_fd, socket_fd, true);
    user_cnt++;
    uid++;
    m_file_address = 0;
    db = new mysql_conn("root", "123", "webserver");
    init();
    cout << "http_conn construct" << endl;
}

http_conn::~http_conn(){
    user_cnt--;
    cout << "http_conn destroy" << endl;
}

void http_conn::init(){

    memset(buffer, '\0', sizeof buffer);
    m_read_idx = 0;
    memset(m_write_buf, '\0', sizeof m_write_buf);
    m_write_idx = 0;
    
    if(m_file_address){
        munmap(m_file_address, m_file_stat.st_size);
    }
    m_file_address = 0;
    state = STATE::NO_REQUEST;
    action = ACTION::NO_POST;
    m_iv_count = 1;
    modfd(epoll_fd, socket_fd, EPOLLIN, true);
}


bool http_conn::map_file(const char* filepath){
    if(stat(filepath, &m_file_stat) < 0) {
        m_file_stat.st_size = 0;
        return false;
    }
    int fd=open(filepath,O_RDONLY);
    m_file_address = (char*) mmap(0,m_file_stat.st_size,PROT_READ,MAP_PRIVATE,fd,0);
    m_iv_count = 2;
    close(fd);
    return true;
}

STATE http_conn::process_read(){
    char* method = new char[10];
    char* url = new char[1024];
    char* body = new char[1024];
    char* filepath = new char[1024];
    memset(method, '\0', sizeof method);
    memset(url, '\0', sizeof url);
    memset(body, '\0', sizeof body);
    memset(filepath, '\0', sizeof filepath);
    parse_line(buffer, method, url, body);
    if(strcmp(method, "GET") == 0){
        state = STATE::GET_REQUEST;
        *filepath = '.';
        strcat(filepath, url);
        if(strcmp(filepath, "./") == 0){
            strcat(filepath, "public/judge.html");//作为配置
        }
    }
    else if(strcmp(method, "POST") == 0){
        state = STATE::POST_REQUEST;
        char *ch = url;
        while(!(*ch >= '0' && *ch <= '9')){
            ch++;
        }
        *filepath = '.';
        strcat(filepath, "/public/");
        char* user = new char[10];
        char* pwd = new char[20];
        memset(user, '\0', sizeof user);
        memset(pwd, '\0', sizeof pwd);
        int cnt = 0;
        char *p = body;
        while(*p){
            if(*p == '='){
                cnt ++;
                if(cnt == 1){
                    user = p + 1;
                }
                else {
                    pwd = p + 1;
                }
            }
            if(*p == '&'){
                *p = '\0';
            }
            p++;
        }
        if(*ch == '0'){
            action = ACTION::NEWUSR;
            strcat(filepath, "register.html");
        }
        else if(*ch == '1'){
            action = ACTION::REGUSR;
            strcat(filepath, "log.html");
        }
        else if(*ch == '2'){
            action = ACTION::LOGIN;
            db->initDB("localhost", db->user, db->pwd, db->db_name, 0);
            string name(user, strlen(user));
            string sql = "select pwd from user_info where name='" + name + "';" ;
            if(db->exeSQL(sql)){
                vector<vector<string>> data = db->process_data();
                if(data.size() != 0 && strcmp(pwd, data[0][0].c_str()) == 0){
                    strcat(filepath, "welcome.html");
                }
                else{
                    strcat(filepath, "logError.html");
                }
            }
            else{
                throw exception();
            }
            db->closeDB();
        }
        else if(*ch == '3'){
            action = ACTION::REGISTER;
            db->initDB("localhost", db->user, db->pwd, db->db_name, 0);
            string name(user, strlen(user));
            string password(pwd, strlen(pwd));
            string sql = "insert into user_info(uid, name, pwd) values(" + to_string(uid) + ", '" + name + "', '" + password +"');";
            if(db->exeSQL(sql)){
                strcat(filepath, "log.html");
            }
            else{
                strcat(filepath, "registerError.html");
            }
            db->closeDB();
        }
        else if(*ch == '5'){
            action = ACTION::GET_IMG;
            strcat(filepath, "picture.html");
        }
        else if(*ch == '6'){
            action = ACTION::GET_VD;
            strcat(filepath, "video.html");
        }
        else if(*ch == '7'){
            action = ACTION::SUBSCRIBE;
            strcat(filepath, "fans.html");
        }
        else{
            action = ACTION::ERROR_POST;
        }
    }
    modfd(epoll_fd, socket_fd, EPOLLOUT, true);
    if(!map_file(filepath)) state = STATE::ERROR404;
    return state;
}

bool http_conn::read(){
    int ret = recv(socket_fd, buffer, MAX_BUF - 1, MSG_DONTWAIT);//非阻塞读
    cout << "recv:" << buffer << endl;
    if(ret > 0) return true;
    else return false;
    
}

bool http_conn::process_write(){
    if(state == STATE::GET_REQUEST || state == STATE::POST_REQUEST){
        if(action != ACTION::ERROR_POST) add_status_line(200, http_code[200]);
        else add_status_line(404, http_code[404]);
    }
    else if(state == STATE::ERROR404){
        add_status_line(404, http_code[404]);
    }
    return add_headers(m_file_stat.st_size);
    
}

bool http_conn::write(){
    int write_st = 0;
    int bytes_have_send = 0;
    while(bytes_have_send < m_file_stat.st_size || m_write_idx != 0){
        m_iv[0].iov_base=m_write_buf + write_st;
        m_iv[0].iov_len=m_write_idx;
        m_iv[1].iov_base=m_file_address + bytes_have_send;
        m_iv[1].iov_len=m_file_stat.st_size - bytes_have_send; 
        int len = writev(socket_fd, m_iv, m_iv_count);
        if(len <= -1) {
            if(errno == EAGAIN){
                modfd(epoll_fd, socket_fd, EPOLLOUT, true);
                return true;
            }
            return false;
        }
        int tmp = min(len, m_write_idx);
        write_st += tmp;
        m_write_idx -= tmp;
        len -= tmp;
        bytes_have_send += len;
    }
    
    init();
}

void http_conn::process(){
    if(process_read() == STATE::NO_REQUEST){
        modfd(epoll_fd, socket_fd, EPOLLIN, true);
        return ;
    }
    if(!process_write()){
        return ;
    }
    modfd(epoll_fd, socket_fd, EPOLLOUT, true);
}

void http_conn::parse_line(char* data, char* method, char* url, char* body){
    char* tmp = data;
    char* l = data;
    int cnt_blk = 0, len = 0;
    while(1){
        len++;
        if(*tmp == ' '){
            cnt_blk++;
            *tmp = '\0';
            if(cnt_blk == 1){
                strcpy(method, l);
            }
            else if(cnt_blk == 2){
                strcpy(url, l);
            }
            l = tmp + 1;
        }
        else if(*tmp == '\n' && *(tmp - 1) == '\r'){
            if(len == 2){
                strcpy(body, tmp + 1);
                break;
            }
            len = 0;
        }
        tmp++;
    }
    if(!url){
        cout << "url is error" << endl;
        return ;
    }
    if(strcmp(method, "GET") == 0){
        cout << "parse GET method" << endl;
    }
    else if(strcmp(method, "POST") == 0){
        cout << "parse POST method" << endl;
    }
}


/*往写缓冲中写入待发送的数据*/

bool http_conn::add_response(const char*format,...){
    va_list arg_list;
    va_start(arg_list,format);
    int len=vsnprintf(m_write_buf+m_write_idx,MAX_BUF-1-m_write_idx,format,arg_list);
    m_write_idx+=len;
    va_end(arg_list);
    return true;
}

bool http_conn::add_status_line(int status,const char*title){
    return add_response("%s %d %s \r\n","HTTP/1.1",status,title);
}

bool http_conn::add_headers(int content_len){
    add_content_length(content_len);
    add_linger();
    add_blank_line();
    return true;
}

bool http_conn::add_content_length(int content_len){
    return add_response("Content-Length:%d\r\n",content_len);
}

bool http_conn::add_linger(){
    return add_response("Connection:%s\r\n", "keep-alive");
}

bool http_conn::add_blank_line(){
    return add_response("%s","\r\n");
}

bool http_conn::add_content(const char*content){
    return add_response("%s",content);
}