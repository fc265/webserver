#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <unordered_map>
#include "ThreadPool.h"
#include "http_conn.h"

#define BUF_SIZ 2048
#define MAX_EVENT_NUM 1024
#define MAX_USERS 10

using namespace std;

//作为配置
int port = 12345;

//tcp/ip协议族，ipv4
class CSocket{
public:
    CSocket(int port, const char *ip_char = nullptr){
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        in_addr inp;
        if(ip_char != nullptr){
            if(inet_aton(ip_char, &inp) != 1){
                cout << "ip trans error" << endl;
                throw exception();
            }
        }
        else{
            inp.s_addr = htonl(INADDR_ANY);
        }
        addr.sin_addr = inp;
        socket_fd = -1;
    }
    ~CSocket(){
        if(socket_fd != -1) close(socket_fd);
    }
    char* getIP(){
        return inet_ntoa(addr.sin_addr);
    }
    int getPort(){
        return ntohs(addr.sin_port);
    }
    int getSocket(){
        return socket_fd;
    }
    void create(){
        socket_fd = socket(PF_INET, SOCK_STREAM, 0);
        if(socket_fd == -1){
            cout << "socket create error" << endl;
            throw exception();
        }

        //accept设置为非阻塞
        setNonblock(socket_fd);

        //设置端口复用
        int reuse = 1;
        setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    }
    void _bind(){
        if(socket_fd == -1){
            cout << "socket not create" << endl;
            return ;
        }
        if(bind(socket_fd, (sockaddr *)&addr, sizeof(addr)) != 0){
            cout << "socket bind error" << endl;
            throw exception();
        }
    }
    void _listen(int backlog=5){
        if(socket_fd == -1){
            cout << "socket not create" << endl;
            return ;
        }
        if(listen(socket_fd, backlog) != 0){
            cout << "socket listen error" << endl;
            throw exception();
        }
    }
    int _accept(){
        sockaddr_in client;
        socklen_t client_len = sizeof(client);
        int conn_fd = accept(socket_fd, (sockaddr *) &client, &client_len);
        // if(conn_fd == -1){
        //     cout << "socket accept error" << endl;
        //     throw exception();
        // }
        cout << "socket_fd " << conn_fd << " accept" << endl;
        return conn_fd;
    }
    char *read(int conn_fd){
        memset(buffer, '\0', BUF_SIZ);
        int ret = recv(conn_fd, buffer, BUF_SIZ - 1, MSG_DONTWAIT);//非阻塞读
        return buffer;
    }
    void write(int conn_fd, char *msg){

        int nsend = send(conn_fd, msg, strlen(msg), MSG_DONTWAIT);//非阻塞写
        cout << "send message " << msg << endl;
    }
    void _connect(int port, const char *ip_char){
        sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);
        server_addr.sin_addr.s_addr = inet_addr(ip_char);
        if(connect(socket_fd, (sockaddr*) &server_addr, sizeof(server_addr)) == -1) {
            cout << "connect fail" << endl;
        }
    }
private:
    int socket_fd;
    sockaddr_in addr;
    char buffer[BUF_SIZ];
};



int main(){
    
    const char *sip = "172.30.44.193";
    CSocket *ss = new CSocket(port);
    ss->create();
    ss->_bind();
    ss->_listen();
    epoll_event events[MAX_EVENT_NUM];
    int epoll_fd = epoll_create(5);
    if(epoll_fd == -1){
        throw exception();
    }
    addfd(epoll_fd, ss->getSocket(), false);
    unordered_map<int, http_conn*> users;
    ThreadPool<http_conn>* pool = new ThreadPool<http_conn>();
    pool->createThreads();

    while(1){
        int ret = epoll_wait(epoll_fd, events, MAX_EVENT_NUM, -1);
        for(int i = 0; i < ret; i++){
            int listen_fd = events[i].data.fd;
            cout << "fd:" << listen_fd << ", in:" << (bool) (events[i].events & EPOLLIN) << ", out:" << (bool) (events[i].events & EPOLLOUT) << endl;
            if(listen_fd == ss->getSocket()){
                int conn_fd = ss->_accept();
                if(conn_fd == -1){
                    throw exception();
                }
                //setNonblock(conn_fd);
                http_conn *http_connection = new http_conn(conn_fd, epoll_fd);
                http_connection->init();
                users[conn_fd] = http_connection;
            }
            else if(events[i].events & EPOLLIN){
                if(users[listen_fd]->read()){
                    //pool->addTask(users[listen_fd]);
                    users[listen_fd]->process();
                }
                
            }
            else if(events[i].events & EPOLLOUT){
                users[listen_fd]->write();
                cout << "send msg" << endl;
            }
            else if(events[i].events & EPOLLRDHUP){
                epoll_ctl(epoll_fd, EPOLL_CTL_DEL, listen_fd, events + i);
                delete users[listen_fd];
                users.erase(listen_fd);
                close(listen_fd);
            }
        }
    }
    delete ss;
}