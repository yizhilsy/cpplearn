#include <iostream>
using namespace std;

class Base {
public:
    virtual void print() {
        cout<<"Base print"<<endl;
    }
    ~Base() { // 普通析构函数
        cout << "Base destructor called" << endl;
    }
};

class Derived : public Base {
public:
    int num1;
    Derived() {
        cout << "Derived constructor called" << endl;
        num1 = 10;
    }
    void print() override {
        cout<<"Derived print"<<endl;
        num1 = 30;
        cout << "num1: " << num1 << endl;
    }
    void functiononlybyderived() {
        cout << "onlybyderived" << endl;
    }
    ~Derived() {
        cout << "Derived destructor called" << endl;
    }
};

int main() {
    Base* obj1 = new Derived();
    Derived* obj3 = new Derived();
    obj1->print();
    delete obj1;
    cout << "----------------" << endl;
    Derived obj2;
    delete obj3;
    cout << "----------------" << endl;
    return 0;
}