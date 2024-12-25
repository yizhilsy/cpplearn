#include <iostream>
#include <string>
#include <utility>
#include "lru_interface.h"
int main()
{
    Ilru_event<int, std::string> *ilru_event = new Ilru_event_implement<int, std::string>;
    Ilru_mgr<int, std::string> *ilru_mgr = new Ilru_mgr_implement<int, std::string>;
    ilru_mgr->init(3, ilru_event);
    std::pair<int, std::string> p1, p2, p3, p4;
    p1.first = 1;p1.second = "one";
    p2.first = 2;p2.second = "two";
    p3.first = 3;p3.second = "three";
    p4.first = 4;p4.second = "four";
    ilru_mgr->add(p1.first, &(p1.second));
    ilru_mgr->add(p2.first, &(p2.second));
    ilru_mgr->add(p3.first, &(p3.second));
    ilru_mgr->add(p4.first, &(p4.second));
    ilru_mgr->show_all();
    std::string *ptr = ilru_mgr->get(2);
    if (ptr != nullptr) {
        std::cout << *(ptr) << std::endl;
    }
    else {
        std::cout << "nullptr" << std::endl;
    }
    Ilru_mgr<int, std::string>* ilru_mgr2 = create_lru_mgr<int, std::string>();
    ilru_mgr2->init(4, ilru_event);
    std::pair<int, std::string> p5, p6, p7, p8, p9;
    p5 = {5, "five"};
    p6 = {6, "six"};
    p7 = {7, "seven"};
    p8 = {8, "eight"};
    p9 = {9, "nine"};
    ilru_mgr2->add(p5.first, &(p5.second));
    ilru_mgr2->add(p9.first, &(p9.second));
    ilru_mgr2->add(p8.first, &(p8.second));
    ilru_mgr2->add(p7.first, &(p7.second));
    ilru_mgr2->add(p6.first, &(p6.second));
    ilru_mgr2->show_all();
    delete ilru_event;
    delete ilru_mgr;
    delete ilru_mgr2;
    Ilru_mgr_implement<int, std::string> ilru_mgr3;
    return 0;
}