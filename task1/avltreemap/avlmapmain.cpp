#include <iostream>
#include <string>
#include <utility>
#include "avlmap_interface.h"
void test02() {
    std::cout << "=====test02=====" << std::endl;
    Iavlmap_mgr<int, std::string> *iavlmap_mgr = create_avl_map<int, std::string>();
    // 测试数据测试
    int testarray[22] = {33, 10, 44, 5, 20, 37, 78, 4, 8, 15, 25, 39, 50, 88, 1, 6, 12, 23, 28, 48, 62, 21};
    std::string stringarray[22] = {"thirty-three", "ten", "forty-four", "five", "twenty", "thirty-seven", "seventy-eight", "four", "eight", "fifteen", 
    "twenty-five", "thirty-nine", "fifty", "eighty-eight", "one", "six", "twelve", "twenty-three", "twenty-eight", "forty-eight", "sixty-two", "twenty-one"};
    for (int i = 0; i < 22; i++) {
        iavlmap_mgr->insert(testarray[i], stringarray[i]);
    }
    std::cout << "******init tree******" << std::endl;
    iavlmap_mgr->levelTraversal();
    iavlmap_mgr->erase(39);
    std::cout << "******after erase******" << std::endl;
    iavlmap_mgr->levelTraversal();
    delete iavlmap_mgr;
}

int main()
{
    Iavlmap_mgr<int, std::string>* iavlmap_mgr = create_avl_map<int, std::string>();
    // 插入元素设置
    int testarray[9] = { 16, 3, 7, 11, 9, 26, 18, 14, 15 };
    std::string teststr[9] = {"sixteen", "three", "seven", "eleven", "nine", "twenty-six", "eighteen", "fourteen", "fifteen"};
    iavlmap_mgr->init();
    for (int i = 0; i < 9; i++) {
        bool flag = iavlmap_mgr->insert(testarray[i], teststr[i]);
        std::cout << flag << std::endl;
        // iavlmap_mgr->levelTraversal()；
    }
    std::cout << "result:" << std::endl;
    iavlmap_mgr->levelTraversal();
    iavlmap_mgr->inorderTraversal();
    uint32_t height = iavlmap_mgr->Height();
    std::cout << "height: " << height << std::endl;
    std::cout << "get and operator[] func test" << std::endl;
    const std::string *pvalue = iavlmap_mgr->get(16);
    std::cout << *(pvalue) << std::endl;
    pvalue = iavlmap_mgr->get(26);
    std::cout << *(pvalue) << std::endl;
    pvalue = iavlmap_mgr->operator[](3);
    std::cout << *(pvalue) << std::endl;
    // 删除元素测试
    bool eraseflag = iavlmap_mgr->erase(11);
    std::cout << eraseflag << std::endl;
    iavlmap_mgr->levelTraversal();
    test02();
    delete iavlmap_mgr;
    return 0;
}