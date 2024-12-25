#pragma once
#include <iostream>
#include <unordered_map>

// 工具类
// 双向链表类模板
template <class K, class V>
class DLinkedNode {
public:
	K key;
	V value;
	DLinkedNode<K, V> *prior;
	DLinkedNode<K, V> *next;
	DLinkedNode() {
		prior = nullptr;
		next = nullptr;
	}
	DLinkedNode(K key, V value, DLinkedNode<K, V>* prior, DLinkedNode<K, V>* next) {
		this->key = key;
		this->value = value;
		this->prior = prior;
		this->next = next;
	}
	~DLinkedNode() {}
};

// 抽象基类
// Ilru_event抽象基类
template<class K,class V>
class Ilru_event
{
public:
	virtual void on_remove(const K& key, V* pValue) = 0;
	virtual ~Ilru_event() = 0;
};

template<class K, class V>
Ilru_event<K, V>:: ~Ilru_event() {}

// 基于lru最近最久未使用的缓存类抽象基类
template<class K,class V>
class Ilru_mgr
{
public:
	// init maxsize and set event
	virtual void init(uint32_t dwMax, Ilru_event<K, V>* pEvent) = 0;
	// add 
	// 成功插入lru返回true,否则返回false
	virtual bool add(const K& key, const V* pValue) = 0;

	// erase
	virtual bool erase(const K &key) = 0;

	// // add 
	// virtual bool add(const K& key, const V &pValue) = 0;

	// get
	virtual V* get(const K& key) = 0;
	virtual K* getRearKey() = 0;
	virtual void listAllKeys(K* key_array) = 0;

	virtual uint32_t get_size() = 0;
	virtual uint32_t get_max_size() = 0;
	virtual void show_all() = 0;

	// 纯虚析构函数
	virtual ~Ilru_mgr();

private:
	// 双向链表相关的函数
	virtual void addToHead(DLinkedNode<K, V>* node) = 0;
	virtual void removeNode(DLinkedNode<K, V>* node) = 0;
	virtual void updateToHead(DLinkedNode<K, V>* node) = 0;
	virtual K removeTail() = 0;
};

// 虚函数中，基类的析构函数一定是虚函数且必须有实现，不建议声明为纯虚函数
template <class K, class V>
Ilru_mgr<K, V>:: ~Ilru_mgr() {}

// implement派生类实现抽象基类
// Ilru_event_implement
template<class K, class V>
class Ilru_event_implement : public Ilru_event<K, V>
{
public:
	void on_remove(const K &key, V *pValue) override;
	virtual ~Ilru_event_implement() = default;
};

template<class K, class V>
void Ilru_event_implement<K, V>:: on_remove(const K &key, V *pValue) {
	std::cout << "Ilru_event_printremove: has removed <" << key << ", " << *(pValue) << ">" << std::endl;
}

// Ilru_mgr_implement
template <class K, class V>
class Ilru_mgr_implement : public Ilru_mgr<K, V>
{
	//implement的成员变量
public:
	// lru缓存表中的当前容量与最大容量
	uint32_t size;
	uint32_t max_size;
	// 使用stl的哈希表，键为K，值为DLinkedNode<K, V>*类型的指针
	std::unordered_map<K, DLinkedNode<K, V>*> cachemap;
	// 双向链表伪头结点，伪尾结点
	DLinkedNode<K, V> *head;
	DLinkedNode<K, V> *rear;
	// Ilru_event事件抽象基类指针
	Ilru_event<K, V>* ilru_event;

public:
	// init maxsize and set event
	void init(uint32_t dwMax, Ilru_event<K, V>* pEvent) override;
	// add 
	bool add(const K& key, const V* pValue) override;

	// // add 
	// virtual bool add(const K& key, const V &pValue) = 0;

	// erase
	bool erase(const K &key) override;

	// get
	V* get(const K& key) override;
	K* getRearKey() override;
	void listAllKeys(K *key_array) override;
	uint32_t get_size() override;
	uint32_t get_max_size() override;
	void show_all() override;

	// Ilru_mgr_implement实现类的析构函数，实际上重写了基类的析构函数
	virtual ~Ilru_mgr_implement() override;

private:
	// 双向链表相关的函数
	void addToHead(DLinkedNode<K, V>* node) override;
	void removeNode(DLinkedNode<K, V>* node) override;
	void updateToHead(DLinkedNode<K, V>* node) override;
	K removeTail() override;
};

template <class K, class V>
void Ilru_mgr_implement<K, V>:: init(uint32_t dwMax, Ilru_event<K, V>* pEvent) {
	this->max_size = dwMax;
	this->ilru_event = pEvent;
	this->size = 0;
	this->head = new DLinkedNode<K, V>;
	this->rear = new DLinkedNode<K, V>;
	head->next = rear;head->prior = nullptr;
	rear->prior = head;rear->next = nullptr;
}

template <class K, class V>
bool Ilru_mgr_implement<K, V>:: add(const K& key, const V* pValue) {
	if(cachemap.find(key) != cachemap.end()) {	// 原先lru缓存已经有了这个键
		// 更新键值对的值
		DLinkedNode<K, V> *node = cachemap[key];
		node->value = *(pValue);
		// 将更新后的结点置为双向链表表头
		this->updateToHead(node);
	}
	else {	// 原先lru缓存所没有的新的键
		// 检查当前容量
		if (this->size >= this->max_size) { // 当前容量已满
			// 淘汰链表尾部元素
			K tailKey = this->removeTail();
			this->size--;
			// 销毁相应的cachemap中的键值对
			cachemap.erase(tailKey);
			// 申请新结点堆区空间并插入双向链表表头
			DLinkedNode<K, V> *node = new DLinkedNode<K, V>(key, *(pValue), nullptr, nullptr);
			this->addToHead(node);
			this->size++;
			// 更新cachemap
			cachemap.insert({key, node});
		}
		else {	// 当前容量未满
			// 申请新结点堆区空间并插入双向链表表头
			DLinkedNode<K, V> *node = new DLinkedNode<K, V>(key, *(pValue), nullptr, nullptr);
			this->addToHead(node);
			this->size++;
			// 更新cachemap
			cachemap.insert({key, node});
		}
	}
	return true;
}

template <class K, class V>
bool Ilru_mgr_implement<K, V>:: erase(const K &key) {
	if (cachemap.find(key) == cachemap.end()) {	// 该键在lru中不存在
		return false;
	}
	else {	// 该键在lru中存在
		DLinkedNode<K, V> *node = cachemap[key];
		// 删除在双向链表中的结点
		this->removeNode(node);
		delete node;
		// 删除在cachemap中的键值对
		cachemap.erase(key);
		// lru目前大小减一
		this->size--;
		return true;
	}
}

template <class K, class V>
V* Ilru_mgr_implement<K, V>:: get(const K& key) {
	if(cachemap.find(key) != cachemap.end()) {	// 输入的键在lru中存在，返回V*
		return &(cachemap[key]->value);
	}
	else {	// 输入的键在lru中不存在，返回nullptr
		return nullptr;
	}
}

template <class K, class V>
K* Ilru_mgr_implement<K, V>:: getRearKey() {
	if (this->size == 0) {
		return nullptr;
	}
	else {
		return &(this->rear->prior->key);
	}
}

template <class K, class V>
void Ilru_mgr_implement<K, V>:: listAllKeys(K* key_array) {
	int i = 0;
	DLinkedNode<K, V> *dlptr = this->head->next;
	while(dlptr != this->rear) {
		key_array[i] = dlptr->key;
		dlptr = dlptr->next;
	}
}


template <class K, class V>
uint32_t Ilru_mgr_implement<K, V>:: get_size() {
	return this->size;
}

template <class K, class V>
uint32_t Ilru_mgr_implement<K, V>:: get_max_size() {
	return this->max_size;
}

template <class K, class V>
void Ilru_mgr_implement<K, V>:: show_all() {
	std::cout << "lru cache info" << std::endl;
	std::cout << "size: " << this->size << "\t" << "max_size: " << this->max_size << std::endl;
	DLinkedNode<K, V> *dlptr = this->head->next;
	while(dlptr != this->rear) {
		std::cout << "<" << dlptr->key << ", " << dlptr->value << ">" << std::endl;
		dlptr = dlptr->next;
	}
}


template <class K, class V>
void Ilru_mgr_implement<K, V>:: addToHead(DLinkedNode<K, V>* node) {
	node->prior = head;
	node->next = head->next;
	head->next->prior = node;
	head->next = node;
}

template <class K, class V>
void Ilru_mgr_implement<K, V>:: removeNode(DLinkedNode<K, V>* node) {
	node->prior->next = node->next;
	node->next->prior = node->prior;
}

template <class K, class V>
void Ilru_mgr_implement<K, V>:: updateToHead(DLinkedNode<K, V>* node) {
	removeNode(node);
	addToHead(node);
}

template <class K, class V>
K Ilru_mgr_implement<K, V>:: removeTail() {
	DLinkedNode<K, V> *tailnode = this->rear->prior;
	this->rear->prior = tailnode->prior;
	tailnode->prior->next = this->rear;
	K tailKey = tailnode->key;
	V *pValue = &(tailnode->value);
	// 调用Ilru_event抽象基类指针
	this->ilru_event->on_remove(tailKey, pValue);
	// 释放堆区元素
	delete tailnode;
	return tailKey;
}

template <class K, class V>
Ilru_mgr_implement<K, V>:: ~Ilru_mgr_implement() {
	// 释放cachemap和双向链表内存
	cachemap.clear();
	DLinkedNode<K, V> *dlptr = head;
	while(dlptr != nullptr) {
		DLinkedNode<K, V>* dlnext = dlptr->next;
		delete dlptr;
		dlptr = dlnext;
	}
}

// 工厂函数，函数内部在堆区域创建派生类对象，赋值给函数返回的基类指针
template <class K, class V>
Ilru_mgr<K, V> *create_lru_mgr() {
	return new Ilru_mgr_implement<K, V>;
}