#include <iostream>
#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <exception>
#include <fcntl.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/errno.h>
#include <unistd.h>
#include <netinet/in.h>
#include <signal.h>
#include <vector>
#include <random>
#include <unordered_set>
#include <map>
#include "tcp_interface.h"
#include "SocketException.h"
#include "SocketUtils.h"
// 线程池类
#include "ThreadPool.h"
#include "lru_interface.h"
#include "utils.h"

#define FD_SETSIZE 1024 // 设置select模型中描述符集合的大小

// 线程池中执行的任务函数声明
void READCLIENTREQUEST(int client_fd, std::vector<char*>& buffer_vector, std::mutex* buffer_locks, Ilru_mgr<int, sockaddr_in>* client_fd_address_lru, fd_set& allfds, 
                        fd_set& readfds, std::mutex& update_fd_set_mutex, std::unordered_map<int, std::mutex*>& client_file_mutex_map, 
                        std::unordered_set<int>& closing_client_fd, std::mutex& in_closing_mutex);

struct server_fd_set {
    fd_set readfds;
    fd_set allfds;
};

// 回调函数抽象基类的实现
// 服务端监听事件回调函数
class Ilisten_event_implement : public Ilisten_event 
{
public:
	// false not call back,so onaccept is ensure succ
	void on_accept(int32_t nId,const char* pRemoteIp,uint16_t wRemotePort) override;
	void on_close(int32_t nId,const char* pRemoteIp,uint16_t wRemotePort,int32_t nError,const char* pErrorMsg) override;
	// virtual void on_read(int32_t nId,const char* pBuffer,uint32_t dwLen) = 0;
	virtual ~Ilisten_event_implement() override = default;
};

// Ilisten_event_implement成员函数实现
void Ilisten_event_implement:: on_accept(int32_t nId,const char* pRemoteIp,uint16_t wRemotePort) {
    std::cout << "accept new client connect successfully, allocated client_fd is: " << nId << ", client RemoteIp: "
              << pRemoteIp << ", client RemotePort: " << wRemotePort << std::endl;
}

void Ilisten_event_implement:: on_close(int32_t nId,const char* pRemoteIp,uint16_t wRemotePort,int32_t nError,const char* pErrorMsg) {    
    if(pErrorMsg == nullptr) {  // 此时成功关闭客户端套接字
        std::cout << "client close successfully, client_fd is: " << nId << ", client RemoteIp: " << pRemoteIp << ", client RemotePort: "
              << wRemotePort << std::endl;
    }
    else {  // 此时关闭客户端套接字失败
        std::cout << "client close failed, client_fd is: " << nId << ", client RemoteIp: " << pRemoteIp << ", client RemotePort: "
              << wRemotePort << ", error message: " << pErrorMsg << std::endl;
    }
}

// 客户端连接事件回调函数
class Iconnect_event_implement : public Iconnect_event
{
public:
	void on_connect(int32_t nId,const char* pRemoteIp,uint16_t wRemotePort,int32_t nError,const char* pErrorMsg) override;
	void on_close(int32_t nId,const char* pRemoteIp,uint16_t wRemotePort,int32_t nError,const char* pErrorMsg) override;
	// void on_read(int32_t nId,const char* pBuffer,uint32_t dwLen) override;
	virtual ~Iconnect_event_implement() override = default;
};

void Iconnect_event_implement:: on_connect(int32_t nId,const char* pRemoteIp,uint16_t wRemotePort,int32_t nError,const char* pErrorMsg) {
    if (pErrorMsg == nullptr) {
        std::cout << "connect to server successfully, here client_fd is: " << nId << ", server RemoteIp: " << pRemoteIp << ", server RemotePort: "
              << wRemotePort << std::endl;
    }
    else {
        std::cout << "connect to server failed, here client_fd is: " << nId << ", server RemoteIp: " << pRemoteIp << ", server RemotePort: "
              << wRemotePort << ", error message: " << pErrorMsg << std::endl;
    }
}

void Iconnect_event_implement:: on_close(int32_t nId,const char* pRemoteIp,uint16_t wRemotePort,int32_t nError,const char* pErrorMsg) {
    if (pErrorMsg == nullptr) {
        std::cout << "client close successfully, client_fd is: " << nId << ", preceding server RemoteIp: " << pRemoteIp << ", preceding server RemotePort: "
              << wRemotePort << std::endl;
    }
    else {
        std::cout << "client close failed, client_fd is: " << nId << ", preceding server RemoteIp: " << pRemoteIp << ", preceding server RemotePort: "
              << wRemotePort << ", error message: " << pErrorMsg << std::endl;
    }
}

// 数据包解析回调函数，可根据协议自定义解析函数，如解析http协议，json协议等
class Ipack_parser_implement : public Ipack_parser
{
public:
	int32_t on_parse(const void* pData,uint32_t dwDataSize) override;
	virtual ~Ipack_parser_implement() override = default;
};

int32_t Ipack_parser_implement:: on_parse(const void* pData, uint32_t dwDataSize) {
    std::cout << "parse data success, data size: " << dwDataSize << std::endl;
    std::cout << "data content: " << (char*)pData << std::endl;
    return 0;
}

// 此tcp网络库引入内部线程，线程池。支持作为服务器端监听多个端口；支持作为客户端接收多个服务器端端连接及发送数据；
class Itcp_manager_implement : public Itcp_manager
{
private:                                                             
    
    // 内部线程池及与线程池相关的成员变量
    ThreadPool thread_pool;                                                     // 线程池类

    // 与服务器端相关的成员变量
    std::map<int, sockaddr_in> server_info_map;                                 // 服务器套接字描述符与服务器地址的映射
    std::unordered_map<int, Ilru_mgr<int, sockaddr_in>*> server_client_lru_map; // 各个服务器套接字与其对应的客户端lru缓存
    Ilru_event<int, sockaddr_in> *lru_event;                                    // lru事件抽象基类指针
    
    std::map<int, server_fd_set> server_fd_set_map;                             // 服务器套接字描述符与select模型中的描述符集合的映射
    std::map<int, int> server_fd_max_fd;                                        // 服务器套接字描述符与其对应的最大文件描述符的映射
    std::map<int, bool> server_controller;                                      // 服务器套接字是否开启的控制器
    std::unordered_map<int, Ilisten_event *> server_listen_event;                         // 服务器套接字对应的监听事件回调函数
    std::unordered_map<int, Ipack_parser *> server_parser;                                // 服务器套接字对应的数据包解析器
    
    // 服务器套接字关闭过程中相关的成员变量
    std::map<int, bool> server_is_shutting_down;                                // 服务器套接字是否正在关闭的过程中
    std::unordered_map<int, std::condition_variable*> server_eventLoopCondition;// 服务器套接字与唤醒condition的映射
    std::unordered_map<int, std::mutex*> server_eventLoopMutex;                 // 服务器套接字与condition的互斥锁映射

    // 缓冲区相关成员变量
    std::vector<char*> buffer_vector;                                           // 指向堆区缓冲区的指针向量，会被线程池内的工作线程调用
    std::mutex* buffer_locks;                                                   // buffer锁向量，保护buffer_vector中每个buffer指针指向的缓冲区

    // 与客户端相关的成员变量
    std::map<int, sockaddr_in> client_info_map;                                 // 客户端套接字描述符与客户端地址的映射
    std::unordered_map<int, Iconnect_event *> client_connect_event;                       // 客户端套接字对应的连接事件回调函数
    std::unordered_map<int, Ipack_parser *> client_parser;                                // 客户端套接字对应的数据包解析器

    // 内部线程事件循环函数，仅用于select模型的循环
    void eventLoop(int server_fd);

public:
	void release() override;
	// 初始化线程池中线程的数量
	bool init(int32_t nThreadCount) override;
	// 该类被服务器端调用时的监听端口函数
	int32_t listen(uint16_t wPort,Ilisten_event* pEvent,Ipack_parser* pParser) override;
	// 该类被客户端调用时的连接函数
	int32_t connect(const char* pRemoteIp,uint16_t wPort,Iconnect_event* pEvent,Ipack_parser* pParser) override;
	// 使用线程池发送数据
    int32_t send(int32_t nId,const void* pData,uint32_t dwDataSize) override;
	
    // 关闭服务器socket并释放资源
    void close(int32_t nId) override;
	// 析构函数释放资源
	~Itcp_manager_implement() override;
};

void Itcp_manager_implement::release() {
    // 关闭所有记录的服务器套接字及释放每个套接字对应的资源
    for (auto& server_info : server_info_map) {
        int server_fd = server_info.first;
        this->close(server_fd);
    }
}

Itcp_manager_implement::~Itcp_manager_implement() {
    this->release();
    // 释放lru_event
    delete this->lru_event;
    // 释放缓冲区
    for (int i = 0; i < (int)this->buffer_vector.size(); i++) {
        delete buffer_vector[i];
    }
    delete buffer_locks;
}

bool Itcp_manager_implement::init(int32_t nThreadCount) {
    // 初始化lru_event
    this->lru_event = new Ilru_event_implement<int, sockaddr_in>();

    // 在堆区声明缓冲区，并将相应的缓冲区指针存入buffer_vector中，缓冲区的数量是声明的线程数量
    for (int i = 0; i < nThreadCount; i++) {
        char *buffer = new char[BUFFER_SIZE + 1];
        memset(buffer, 0, sizeof(char)*(BUFFER_SIZE + 1));
        buffer_vector.push_back(buffer);
    }
    buffer_locks = new std::mutex[nThreadCount];
    
    // 设置线程池中的线程数量
    try {
        bool init_pool_result = thread_pool.init(nThreadCount);
        if (init_pool_result == false) {
            throw std::invalid_argument("Invalid thread count or threads' num surpass 16!");
        }
        else {return true;}
    }
    catch (const std::invalid_argument& e) {
        std::cerr << e.what() << std::endl;
        return false;
    }
}

int32_t Itcp_manager_implement::listen(uint16_t wPort,Ilisten_event* pEvent,Ipack_parser* pParser) {
    // 0. 初始化与服务器端有关的参数
    // server socket base info
    int server_fd;
    sockaddr_in server_address;
    int addrlen = sizeof(server_address);
    // lru
    Ilru_mgr<int, sockaddr_in> *lru_mgr = create_lru_mgr<int, sockaddr_in>();
    lru_mgr->init(MAX_SESSION_COUNT, lru_event);
    
    // 1. 创建服务器套接字，并设置为非阻塞类型
    try {
        server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd < 0) {
           throw SocketException("create tcp type socket failed"); 
        }
        else {
            // 设置服务器套接字为非阻塞
            int server_flag = fcntl(server_fd, F_GETFL, 0);
            if (server_flag == -1) {
                throw std::runtime_error("fcntl failed");
            }
            if (fcntl(server_fd, F_SETFL, server_flag | O_NONBLOCK) == -1) {
                throw std::runtime_error("fcntl failed to set non-blocking");
            }
            std::cout << "create tcp type socket success, server_fd is: " << server_fd << std::endl;
        }
    }
    catch (const SocketException& e) {
        std::cerr << e.what() << std::endl;
        return -1;
    }
    catch (const std::runtime_error& e) {
        std::cerr << e.what() << std::endl;
        ::close(server_fd);
        return -1;
    }

    // 2. 设置服务器监听的地址，端口参数
    memset(&server_address, 0, addrlen);
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(wPort);

    // 3. 将1中创建的socket套接字绑定到设置的server_address变量
    try {
        int bind_result = bind(server_fd, (struct sockaddr *)&server_address, sizeof(server_address));
        if (bind_result == -1) {
            throw SocketException("socket bind server address failed", server_fd);
        }
        else {
            std::cout << "socket: " << server_fd << " socket bind server address success, binded address is below" << std::endl;
            printSockAddrIn(server_address);
        }
    }
    catch (const SocketException& e) {
        std::cerr << e.what() << " socket_fd: " << e.get_socket_fd() << std::endl;
        ::close(server_fd);
        return -1;
    }

    // 4. 监听连接，设置最大允许连接数为MAX_SESSION_COUNT
    try {
        int listen_result = ::listen(server_fd, MAX_SESSION_COUNT);
        if (listen_result == -1) {
            throw SocketException("socket listen failed", server_fd);
        }
        else {
            std::cout << "socket: " << server_fd << " listen success, listened address is below" << std::endl;
            printSockAddrIn(server_address);
        }
    }
    catch(const SocketException& e) {
        std::cerr << e.what() << " socket_fd: " << e.get_socket_fd() << std::endl;
        return -1;
    }

    // 5. 设置select函数的参数
    server_fd_set server_set;
    FD_ZERO(&server_set.allfds);
    FD_SET(server_fd, &server_set.allfds);
    
    // 6. 更新server_fd与服务器相关参数的映射
    this->server_info_map[server_fd] = server_address;
    this->server_client_lru_map[server_fd] = lru_mgr;
    this->server_fd_set_map[server_fd] = server_set;
    this->server_fd_max_fd[server_fd] = server_fd;
    this->server_controller[server_fd] = true;
    this->server_listen_event[server_fd] = pEvent;
    this->server_parser[server_fd] = pParser;
    this->server_is_shutting_down[server_fd] = false;
    this->server_eventLoopCondition[server_fd] = new std::condition_variable();
    this->server_eventLoopMutex[server_fd] = new std::mutex();

    // 7. 利用线程池实现内部线程
    this->thread_pool.enqueueTask([server_fd, this]() {
        std::cout << "thread_pool start boot server thread! server_fd: " << server_fd << std::endl;
        try {
            this->eventLoop(server_fd);
        }
        catch (const SocketException& e) {
            std::cerr << e.what() << " fd: " << e.get_socket_fd() << std::endl;
            return;
        }
        catch (const std::runtime_error& e) {
            std::cerr << e.what() << std::endl;
            return;
        }
    });

    return server_fd;
}

void Itcp_manager_implement::eventLoop(int server_fd) {
    // 获取此server_fd对应的lru基类指针
    Ilru_mgr<int, sockaddr_in> *client_fd_address_lru = server_client_lru_map[server_fd];
    // 获取数据类型为回调函数类的指针
    Ilisten_event *listen_event = server_listen_event[server_fd];
    // 获取数据类型为数据包解析器类的指针
    Ipack_parser *parser = server_parser[server_fd];

    // 防止重复分发任务的哈希表及其互斥锁，用于服务器端读取已连接的客户端发送的数据
    std::unordered_set<int> in_process_client_fd;
    std::mutex in_process_mutex;
    // 用于控制更新fd_set的互斥锁，避免select模型访问已经被销毁的socket_fd
    std::mutex update_fd_set_mutex;
    // 避免重复关闭一个客户端套接字及重复释放其相关资源
    std::unordered_set<int> closing_client_fd;
    std::mutex in_closing_mutex;

    // 一个服务器套接字对应的客户端变量
    int client_size = 0;
    int *client_fd_array = nullptr;

    // select模型中的超时时间
    timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    // 不同文件对应的mutex，键为client_fd，值为mutex指针
    std::unordered_map<int, std::mutex*> client_file_mutex_map;

    while (server_controller.find(server_fd) != server_controller.end() && this->server_controller[server_fd] == true) {    // 检查此服务端是否还需要继续运行
        int activity = 0;

        {
            // 申请互斥锁避免多线程对于fd_set数据类型的竞态操作
            std::unique_lock<std::mutex> lock(update_fd_set_mutex);
            // 赋值readfds
            server_fd_set_map[server_fd].readfds = server_fd_set_map[server_fd].allfds;
            // 设置阻塞模式的select，activity为准备好读取的描述符
            activity = select(server_fd_max_fd[server_fd] + 1, &server_fd_set_map[server_fd].readfds, nullptr, nullptr, &timeout);
        }

        if (activity < 0) {
            std::cerr << "select error: " << std::strerror(errno) << " (errno: " << errno << ")" << std::endl;
            throw std::runtime_error("select error");
        }
        else if (activity == 0) {
            continue;
        }
        else {  // 此时有可连接的描述符
            // 从客户端lru缓存中检索已连接的客户端是否有数据传输，需要持锁访问避免异步问题
            {
                std::unique_lock<std::mutex> lock(update_fd_set_mutex);
                client_size = client_fd_address_lru->get_size();
                client_fd_array = new int[client_size];
                client_fd_address_lru->listAllKeys(client_fd_array);
            }

            for (int i = 0; i < client_size; i++) {
                int client_fd = client_fd_array[i];
                // 每次取出client_fd时都需要申请加锁，因为可能有其他线程正在执行关闭client_fd，加锁避免分发线程池已经执行且关闭了client_fd的任务
                {
                    std::unique_lock<std::mutex> lock(update_fd_set_mutex);
                    if (isSocketValid(client_fd) == true && FD_ISSET(client_fd, &server_fd_set_map[server_fd].readfds)) { // 此时clientfd客户端有新的数据到达
                        {
                            // 无论是查询还是声明任务已分发，都需要加锁，避免分发已经被线程池处理后且关闭client_fd的任务
                            std::unique_lock<std::mutex> lock(in_process_mutex);
                            if (in_process_client_fd.find(client_fd) != in_process_client_fd.end()) {   // 此时表明该客户端套接字已经被一个工作线程处理，不再重复分发
                                continue;
                            }
                            // 声明此时任务已分发，避免重复分发
                            in_process_client_fd.insert(client_fd);
                        }
                        // 调用线程池进行读取数据的任务
                        server_fd_set& server_set = this->server_fd_set_map[server_fd];
                        std::cout << "client_fd: " << client_fd << "塞入任务队列" << std::endl;
                        thread_pool.enqueueTask([client_fd, this, client_fd_address_lru, &server_set, &in_process_client_fd, &in_process_mutex, &update_fd_set_mutex, &client_file_mutex_map, &closing_client_fd, &in_closing_mutex]() { 
                            try {
                                std::cout<<"thread pool now accept a read data task! client_fd: "<< client_fd << std::endl;
                                READCLIENTREQUEST(client_fd, this->buffer_vector, this->buffer_locks, client_fd_address_lru, server_set.allfds, server_set.readfds, update_fd_set_mutex, client_file_mutex_map, closing_client_fd, in_closing_mutex);
                            }
                            catch (const SocketException& e) {
                                std::cerr << e.what() << " client_fd: " << e.get_socket_fd() << std::endl;
                            }
                            {
                                // 任务执行完毕，删除重复分发标记
                                std::unique_lock<std::mutex> lock(in_process_mutex);
                                in_process_client_fd.erase(client_fd);
                            }
                        });
                    }
                }
            }

            // 检查server_fd服务器socket是否有新的客户端连接
            if (FD_ISSET(server_fd, &server_fd_set_map[server_fd].readfds)) {    // 此时有新的客户端连接
                while (true) {  // 将此时accept队列的所有客户端连接处理
                    sockaddr_in client_address;
                    int client_addrlen = sizeof(client_address);
                    // 由于将server_fd设置为非阻塞模式，所以此时accept函数不会阻塞
                    int client_fd = accept(server_fd, (struct sockaddr *)&client_address, (socklen_t *)&client_addrlen);
                    if (client_fd < 0) {
                        if (errno == EWOULDBLOCK || errno == EAGAIN) { // 此时表明accept队列中无新的客户端连接
                            std::cout << "has accepted all new client connections" << std::endl;
                            break;
                        }
                        else {
                            throw SocketException("accept client connection failed", server_fd);
                        }
                    }
                    else if (client_fd == 0) { // 此时表明无新的客户端连接
                        std:: cout << "no new client connection" << std::endl;
                        break;
                    }
                    else { // 此时表明有新的客户端连接
                        listen_event->on_accept(client_fd, inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));
                        // 线程池中的工作线程可能会并行访问修改client_file_mutex_map,server_fd_max_fd,client_fd_address_lru，因此需要加锁，确保线程安全
                        {
                            std:: cout << "开始分配给新连接的客户端: " << client_fd << "相关的资源！" << std::endl;
                            std::unique_lock<std::mutex> lock(update_fd_set_mutex);
                            // 更新max_fd
                            server_fd_max_fd[server_fd] = std::max(server_fd_max_fd[server_fd], client_fd);
                            // 为client_file_mutex_map中的client_fd在堆区申请读写文件的互斥锁
                            client_file_mutex_map[client_fd] = new std::mutex();

                            // 检查lru缓存是否已满
                            int lru_max_size = client_fd_address_lru->get_max_size();
                            int size = client_fd_address_lru->get_size();
                            if (size < lru_max_size) { // 此时lru缓存未满
                                // 插入新client_fd到lru缓存
                                client_fd_address_lru->add(client_fd, &client_address);
                                FD_SET(client_fd, &server_fd_set_map[server_fd].allfds);
                            }
                            else { // 此时lru缓存已满，需要同步更新allfds套接字集合
                                // 获取lru缓存中最近最久未使用的客户端socket键
                                int rear_client_fd = *(client_fd_address_lru->getRearKey());
                                // 检查rear_client_fd目前是否被线程池的某个工作线程正在执行关闭
                                {
                                    std::unique_lock<std::mutex> lock(in_closing_mutex);
                                    if(closing_client_fd.find(rear_client_fd) != closing_client_fd.end()) { // 这个client_fd正在被某个工作线程关闭，不再执行因为lru缓存已满的关闭系列操作
                                        continue;
                                    }
                                    // 此时该客户端socket没有被线程池中的工作线程关闭，加入closing_client_fd集合，避免重复关闭
                                    closing_client_fd.insert(rear_client_fd);
                                }

                                // 关闭最近最久未使用的客户端socket
                                try {
                                    int close_result = ::close(rear_client_fd);
                                    if (close_result == -1) {
                                        throw SocketException("close rear client_fd failed", rear_client_fd);
                                    }
                                    else {
                                        listen_event->on_close(rear_client_fd, inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port), 0, nullptr);
                                    }
                                }
                                catch (const SocketException& e) {  // 关闭最近最久未使用的客户端时出错的异常处理
                                    std::cerr << e.what() << " client_fd: " << e.get_socket_fd() << std::endl;
                                    listen_event->on_close(rear_client_fd, inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port), errno, strerror(errno));
                                    continue;
                                }
                                
                                // 释放lru末尾的客户端socket rear_client_fd 对应的在堆区的文件mutex
                                if(client_file_mutex_map.find(rear_client_fd) != client_file_mutex_map.end()) {
                                    delete client_file_mutex_map[rear_client_fd];
                                    client_file_mutex_map.erase(rear_client_fd);
                                }
                                // 从allfds中消除最近最久未使用的客户端socket
                                FD_CLR(rear_client_fd, &server_fd_set_map[server_fd].allfds);

                                // 插入新client_fd到lru缓存
                                client_fd_address_lru->add(client_fd, &client_address);
                                // 插入新client_fd到allfds套接字集合
                                FD_SET(client_fd, &server_fd_set_map[server_fd].allfds);

                                // 释放closing_client_fd集合中的rear_client_fd
                                {
                                    std::unique_lock<std::mutex> lock(in_closing_mutex);
                                    closing_client_fd.erase(rear_client_fd);
                                }
                            }
                        }
                    }
                }
            }
        }
        // 及时释放该轮循环中申请的内存
        delete[] client_fd_array;
    }

    // 清空file_mutex_map中声明在堆区的互斥锁
    for (const auto& file_mutex : client_file_mutex_map) {
        delete file_mutex.second;
    }

    {
        // 持有锁后唤醒close函数中等待的线程
        std::unique_lock<std::mutex> lock(*(this->server_eventLoopMutex[server_fd]));
        this->server_is_shutting_down[server_fd] = false;
        this->server_eventLoopCondition[server_fd]->notify_all();
    }
}

int32_t Itcp_manager_implement:: connect(const char* pRemoteIp,uint16_t wPort,Iconnect_event* pEvent,Ipack_parser* pParser) {
    // 1. 创建tcp客户端连接套接字
    int client_connect_fd;
    sockaddr_in client_target_address;
    try {
        client_connect_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (client_connect_fd < 0) {
            throw SocketException("create tcp type socket failed");
        }
        else {
            std::cout << "create tcp type socket success, client_fd is: " << client_connect_fd << std::endl;
        }
    }
    catch(const SocketException& e) {
        std::cerr << e.what() << std::endl;
        return -1;
    }

    // 2. 设置服务器地址信息
    client_target_address.sin_family = AF_INET;
    inet_pton(AF_INET, pRemoteIp, &client_target_address.sin_addr);
    client_target_address.sin_port = htons(wPort);

    // 3. 连接到目标服务器
    try {
        int connect_result = ::connect(client_connect_fd, (struct sockaddr*)&client_target_address, sizeof(client_target_address));
        if (connect_result == -1) {
            throw SocketException("connection to target server failed", client_connect_fd);
        }
        else {
            // pEvent->on_connect(client_connect_fd, pRemoteIp, wPort, 0, nullptr);
            std::cout << "client socket_fd: " << client_connect_fd << " connected to the server at " << pRemoteIp << ":" << wPort << std::endl;
        }
    }
    catch (const SocketException& e) {
        std::cerr << e.what() << "client_fd: " << e.get_socket_fd() << std::endl;
        // pEvent->on_connect(client_connect_fd, pRemoteIp, wPort, errno, strerror(errno));
    }

    // 4. 更新client_info_map客户端套接字与相关资源的映射
    this->client_info_map[client_connect_fd] = client_target_address;
    this->client_connect_event[client_connect_fd] = pEvent;
    this->client_parser[client_connect_fd] = pParser;
    return client_connect_fd;
}

int32_t Itcp_manager_implement:: send(int32_t nId,const void* pData,uint32_t dwDataSize) {
    ssize_t bytesSent;
    try {
        bytesSent = ::send(nId, pData, dwDataSize, 0);
        if (bytesSent < 0) {
            throw SocketException("send data to targerfd failed", nId);
        }
        else {
            std::cout << "send data to targetfd successfully, targetfd: " << nId << " sent bytes: " << bytesSent << std::endl;
        }
    }
    catch (const SocketException& e) {
        std::cerr << e.what() << "targerfd: " << e.get_socket_fd() << std::endl;
        return -1;
    }
    return bytesSent;
}

void Itcp_manager_implement:: close(int32_t nId) {
    std::cout << "close now!" << std::endl;
    // 检测nId是否为此对象的服务器套接字描述符
    if (this->server_info_map.find(nId) == this->server_info_map.end()) {
        std::cerr << "server_fd: " << nId << " is not in the server_info_map!" << std::endl;
        return;
    }

    // 获取server_fd对应的监听事件回调函数
    Ilisten_event *listen_event = this->server_listen_event[nId];
    {
        std::unique_lock<std::mutex> lock(*this->server_eventLoopMutex[nId]);
        // 1. 将server_is_shutting_down[nId]设置为 true
        this->server_is_shutting_down[nId] = true;
        // 2. 更新server_controller为false，eventLoop线程将逐渐停止
        this->server_controller[nId] = false;
        // 3. close线程等待eventLoop线程完全结束运行
        this->server_eventLoopCondition[nId]->wait(lock, [this, nId]() {
            return (this->server_is_shutting_down.find(nId) != this->server_is_shutting_down.end()) && !this->server_is_shutting_down[nId];
        });
        // 释放lock(this->server_eventLoopMutex[nId])锁
    }
    
    // 4. 关闭服务器套接字
    try {
        int close_result = ::close(nId);
        if (close_result == -1) {
            throw SocketException("close server socket failed", nId);
        }
        else {
            listen_event->on_close(nId, inet_ntoa(this->server_info_map[nId].sin_addr), ntohs(this->server_info_map[nId].sin_port), 0, nullptr);
        }
    }
    catch (const SocketException& e) {
        std::cerr << e.what() << "server_fd: " << e.get_socket_fd() << std::endl;
        listen_event->on_close(nId, inet_ntoa(this->server_info_map[nId].sin_addr), ntohs(this->server_info_map[nId].sin_port), errno, strerror(errno));
    }

    // 5. 清理与nId服务端套接字相关的资源
    this->server_info_map.erase(nId);
    delete this->server_client_lru_map[nId];
    this->server_client_lru_map.erase(nId);
    this->server_fd_set_map.erase(nId);
    this->server_fd_max_fd.erase(nId);
    this->server_controller.erase(nId);
    this->server_listen_event.erase(nId);
    this->server_parser.erase(nId);
    this->server_is_shutting_down.erase(nId);
    delete this->server_eventLoopCondition[nId];
    this->server_eventLoopCondition.erase(nId);
    delete this->server_eventLoopMutex[nId];
    this->server_eventLoopMutex.erase(nId);

    std::cout << "server_fd: "<< nId << "'s resources have been cleaned successfully!" << std::endl;
}

// 线程池中执行的任务函数定义
void READCLIENTREQUEST(int client_fd, std::vector<char*>& buffer_vector, std::mutex* buffer_locks, Ilru_mgr<int, sockaddr_in>* client_fd_address_lru, fd_set& allfds, 
                        fd_set& readfds, std::mutex& update_fd_set_mutex, std::unordered_map<int, std::mutex*>& client_file_mutex_map, 
                        std::unordered_set<int>& closing_client_fd, std::mutex& in_closing_mutex) {
    int buffer_index = -1;
    {
        // 检索是否还有空余的buffer缓冲区，并申请加锁
        for (int i = 0; i < (int)buffer_vector.size(); i++) {
            std::unique_lock<std::mutex> lock_guard(buffer_locks[i], std::try_to_lock);
            if (lock_guard.owns_lock()) {   // 如果成功加锁
                buffer_index = i;
                std::cout << "lock buffer index: " << buffer_index << std::endl;
                break;
            }
        }
        // 如果没有空余的buffer缓冲区，随机选择一个buffer缓冲区申请加锁
        if (buffer_index == -1) {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> dis(0, (int)buffer_vector.size() - 1);
            buffer_index = dis(gen); // 随机选择一个索引
            std::unique_lock<std::mutex> lock_guard(buffer_locks[buffer_index]);
            std::cout << "lock buffer index: " << buffer_index << std::endl;
        }

        // 此时获取了buffer_index对应的buffer缓冲区的访问权限
        char *buffer = buffer_vector[buffer_index];
        int bytes_read = read(client_fd, buffer, BUFFER_SIZE);
        
        if (bytes_read < 0) {
            throw SocketException("read client request failed", client_fd);
        }
        else if (bytes_read == 0) { // 此时客户端关闭连接
            {
                std::unique_lock<std::mutex> lock(in_closing_mutex);
                if(closing_client_fd.find(client_fd) != closing_client_fd.end()) {  // 此时表明此客户端socket正在被select主线程关闭，不再执行关闭操作，避免重复关闭
                    std::cout << "Client disconnected successfully, socket FD: " << client_fd << std::endl;
                    return;
                }
            }
            
            // 此时表明select主线程没有关闭此客户端socket
            {
                std::unique_lock<std::mutex> lock(in_closing_mutex);
                closing_client_fd.insert(client_fd);
            }
            {
                // 修改对应server_fd的 allfds 集合时，需要加锁避免多线程同时修改
                std::unique_lock<std::mutex> lock(update_fd_set_mutex);
                // 声明此客户端socket正在被关闭，避免重复关闭
                if(::close(client_fd) == -1) {
                    throw SocketException("close client socket failed", client_fd);
                }
                // 清除lru缓存中的客户端socket及FD_ALL, FD_READ集合中的客户端socket及client_file_mutex_map中在堆区申请的mutex
                client_fd_address_lru->erase(client_fd);
                if(client_file_mutex_map.find(client_fd) != client_file_mutex_map.end()) {
                    delete client_file_mutex_map[client_fd];
                    client_file_mutex_map.erase(client_fd);
                }
                FD_CLR(client_fd, &allfds);
                FD_CLR(client_fd, &readfds);
            }
            {
                std::unique_lock<std::mutex> lock(in_closing_mutex);
                closing_client_fd.erase(client_fd);
            }
            std::cout << "Client disconnected successfully, socket FD: " << client_fd << std::endl;
        }
        else {  // 此时客户端正常有数据传输
            buffer[bytes_read] = '\0';
            std::cout << "bytes_read: " << bytes_read << std::endl;
            // 创建/打开基于client_fd的日志文件，以追加方式写入
            std::string file_name = "./server_log/client_fd_" + std::to_string(client_fd) + ".log";
            // 由于多线程追加写入与client_fd对应的log日志文件，需要加锁避免多线程同时写入导致的资源竞争问题
            {
                std::unique_lock<std::mutex> lock(*(client_file_mutex_map[client_fd]));
                std::ofstream file(file_name, std::ios::app);
                if(file.is_open() == true) {
                    std::cout << "Received: " << buffer << std::endl;
                    std::cout << std::flush;  // 确保立即输出到终端
                    file << "Received: " << buffer << "\n";    // 到file里写入接收到的数据
                    file.flush();                                   // 刷新缓冲区
                }
                else {
                    throw std::runtime_error("failed to open or append file: " + file_name);
                }
            }
            memset(buffer, 0, BUFFER_SIZE);
        }
        // 花括号结束时，lock_guard对象被销毁，buffer_locks[buffer_index]解锁，可以被其他线程加锁
    }
}