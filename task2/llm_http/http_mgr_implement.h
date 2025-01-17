#pragma once

#include <iostream>
#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <fcntl.h>
#include <exception>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/errno.h>
#include <unistd.h>
#include <netinet/in.h>
#include <signal.h>
#include <curl/curl.h>

#include "http_mgr_interface.h" // llmhttp接口头文件

class Ihttp_event_implement : public Ihttp_event
{
public:
	// void on_data(uint64_t qwTransId, const void* pData, int32_t nDataSize, int32_t nConstTime) override;
	// void on_error(uint64_t qwTransId, uint32_t dwErrorCode, const char* pErrorInfo, int32_t nCostTime) override;
	virtual ~Ihttp_event_implement() override = default;
};

class Ihttp_mgr_implement : public Ihttp_mgr
{
// 成员变量
private:
    CURL *curl;
    CURLcode result;
    std::string response;
    curl_slist *headers;

    // 成员函数
public:
	bool init() override;
    bool release() override;
	bool get(uint64_t qwTransId, const string& strUrl, Ihttp_event* pEvent, int32_t nTimeout = 5) override;
	// bool post(uint64_t qwTransId, const string& strUrl, const string& strPos, Ihttp_event* pEvent, int32_t nTimeout = 5) override;
	// void set_end_flag(const string& strEndFlag) override;
	virtual ~Ihttp_mgr_implement() override;
};

// 工厂函数构造一个 Ihttp_mgr_implement 实例
Ihttp_mgr* create_http_mgr() {
    return new Ihttp_mgr_implement;
}

bool Ihttp_mgr_implement::init() {
    // 初始化CURL
    try {
        curl = curl_easy_init();
        if (curl) {
            headers = nullptr;
            headers = curl_slist_append(headers, "User-Agent: Mozilla/5.0");
            headers = curl_slist_append(headers, "Content-Type: application/json");
            return true;
        }
        else {
            throw std::runtime_error("initializate curl fail");
        }
    }
    catch (const runtime_error& e) {
        std::cerr << e.what() << std::endl;
        return false;
    }
}

bool Ihttp_mgr_implement::release() {
    // 清理 http 请求头 headers 资源
    curl_slist_free_all(headers);
    // 清理 CURL 资源
    curl_easy_cleanup(curl);
    return true;
}

Ihttp_mgr_implement::~Ihttp_mgr_implement() {
    response.clear();
    this->release();
}

bool Ihttp_mgr_implement::get(uint64_t qwTransId, const string& strUrl, Ihttp_event* pEvent, int32_t nTimeout) {    
    // 设置请求的 URL
    curl_easy_setopt(curl, CURLOPT_URL, strUrl.c_str());
    // 设置回调函数的数据指针
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    // 设置超时时间
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, nTimeout);

    try {
        result = curl_easy_perform(curl);
        if (result != CURLE_OK) {
            throw std::runtime_error("GET Request Fail!");
        }
        else {
            std::cout << "GET Response:\n" << response << std::endl;
            return true;
        }
    }
    catch (const std::runtime_error& e) {
        std::cerr << e.what() << std::endl;
        return false;
    }
}
