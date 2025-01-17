#pragma once

#include <stdint.h>
#include <string>

using namespace std;

// 回调函数
class Ihttp_event
{
public:
	// virtual void on_data(uint64_t qwTransId, const void* pData, int32_t nDataSize, int32_t nConstTime) = 0;
	// virtual void on_error(uint64_t qwTransId, uint32_t dwErrorCode, const char* pErrorInfo, int32_t nCostTime) = 0;
	virtual ~Ihttp_event() = default;
};

// http抽象基类，使用curl库发起http请求
class Ihttp_mgr
{
public:
	virtual bool init() = 0;
	virtual bool release() = 0;
	virtual bool get(uint64_t qwTransId, const string& strUrl, Ihttp_event* pEvent, int32_t nTimeout = 5) = 0;
	// virtual bool post(uint64_t qwTransId, const string& strUrl, const string& strPos, Ihttp_event* pEvent, int32_t nTimeout = 5) = 0;
	// virtual void set_end_flag(const string& strEndFlag) = 0;
	virtual ~Ihttp_mgr() = default;
};

Ihttp_mgr* create_http_mgr();