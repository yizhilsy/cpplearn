// select模型的tcp网络库接口，select模型的应用适合小用户量访问
#pragma once

#include <stdint.h>
#include <sys/select.h>
#include "lru_interface.h"

#define MAX_SESSION_COUNT 1024  // 设置最大连接数宏。如修改，需重新编译
#define BUFFER_SIZE 1024		// 设置1KB的缓冲区大小
#define DEFAULT_PORT 8011 	 	// 设置默认端口号宏

// 回调函数抽象基类
class Ilisten_event
{
public:
	// false not call back,so onaccept is ensure succ
	virtual void on_accept(int32_t nId,const char* pRemoteIp,uint16_t wRemotePort) = 0;
	virtual void on_close(int32_t nId,const char* pRemoteIp,uint16_t wRemotePort,int32_t nError,const char* pErrorMsg) = 0;
	// virtual void on_read(int32_t nId,const char* pBuffer,uint32_t dwLen) = 0;
	virtual ~Ilisten_event() = default;
};

class Iconnect_event
{
public:
	virtual void on_connect(int32_t nId,const char* pRemoteIp,uint16_t wRemotePort,int32_t nError,const char* pErrorMsg) = 0;
	virtual void on_close(int32_t nId,const char* pRemoteIp,uint16_t wRemotePort,int32_t nError,const char* pErrorMsg) = 0;
	// virtual void on_read(int32_t nId,const char* pBuffer,uint32_t dwLen) = 0;
	virtual ~Iconnect_event() = default;
};

class Ipack_parser
{
public:
	virtual int32_t on_parse(const void* pData,uint32_t dwDataSize) = 0;
	virtual ~Ipack_parser() = default;
};

class Itcp_manager
{
public:
	virtual void release() = 0;
	// 初始化线程池中线程的数量
	virtual bool init(int32_t nThreadCount) = 0;
	// 该类被服务器端调用时的监听端口函数
	virtual int32_t listen(uint16_t wPort,Ilisten_event* pEvent,Ipack_parser* pParser) = 0;
	// 该类被客户端调用时的连接函数
	virtual int32_t connect(const char* pRemoteIp,uint16_t wPort,Iconnect_event* pEvent,Ipack_parser* pParser) = 0;
	// 使用线程池发送数据
	virtual int32_t send(int32_t nId,const void* pData,uint32_t dwDataSize) = 0;
	virtual void close(int32_t nId) = 0;
	// 析构函数释放资源
	virtual ~Itcp_manager() = default;
};

// 工厂函数
Itcp_manager* create_tcp_mgr();
