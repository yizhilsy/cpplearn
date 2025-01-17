#include <iostream>
#include "http_mgr_implement.h"
int main()
{
    Ihttp_mgr *http_mgr = new Ihttp_mgr_implement();
    http_mgr->init();
    string talentrank_url = "http://1.95.59.208:8077/topic/listTopic";
    http_mgr->get(-1, talentrank_url, nullptr);
    return 0;
}