// todo 12.4 afternoon
#pragma once
#include <iostream>
#include <utility>
#include <algorithm>
#include <queue>

// avlnode 二叉平衡树结点
template <typename K, typename V>
class avlnode
{
public:
    std::pair<K, V> kv;
    avlnode<K, V> *left;
    avlnode<K, V> *right;
    avlnode<K, V> *parent;
    int bf;
    // 注：新插入的结点的高度差为0
    avlnode(K key, V value) {
        kv.first = key;
        kv.second = value;
        left = nullptr;right = nullptr;parent = nullptr;
        bf = 0;
    }
    ~avlnode() = default;
};

// 抽象基类
// 二叉平衡树Iavlmap抽象基类
template <typename K, typename V>
class Iavlmap_mgr
{
public:
    virtual void init() = 0;
    virtual uint32_t Height() = 0;
    virtual bool insert(K key, V value) = 0;
    virtual const V* get(K key) = 0;
    // []运算符重载
    virtual const V *operator[](K key) = 0;
    virtual bool erase(K key) = 0;
    // 层序遍历
    virtual void levelTraversal() = 0;
    // 中序遍历
    virtual void inorderTraversal() = 0;
    // 纯虚析构函数，一定要有实现
    virtual ~Iavlmap_mgr() = default;

private:
    // 查询二叉平衡树（子树）的高度
    virtual uint32_t avlHeight(avlnode<K, V> *root) = 0;
    virtual void levelTraversal(avlnode<K, V> *root) = 0;
    virtual void inorderTraversal(avlnode<K, V> *root) = 0;
    virtual avlnode<K, V> *findavlnode(K key) = 0;
    // 判断二叉平衡树（子树）是否平衡
    // virtual bool isBalance(avlnode<K, V> *root) = 0;

    // 四种平衡方式的实现（插入使用）
    virtual void RotateLL(avlnode<K, V> *parent) = 0;
    virtual void RotateLR(avlnode<K, V> *parent) = 0;
    virtual void RotateRR(avlnode<K, V> *parent) = 0;
    virtual void RotateRL(avlnode<K, V> *parent) = 0;

    // 四种平衡方式的实现（删除使用） 注：实现采用插入的函数，但具体bf值需要再调整
    virtual void del_RotateLL(avlnode<K, V> *parent) = 0;
    virtual void del_RotateLR(avlnode<K, V> *parent) = 0;
    virtual void del_RotateRR(avlnode<K, V> *parent) = 0;
    virtual void del_RotateRL(avlnode<K, V> *parent) = 0;

    // 寻找指定结点的前驱，若无左子树，返回nullptr
    virtual avlnode<K, V> *findpre(avlnode<K, V> *node) = 0;
    // 寻找指定结点的后继，若无右子树，返回nullptr
    virtual avlnode<K, V> *findpost(avlnode<K, V> *node) = 0;
    // binary-sort 方式删除结点，cur指针用来指向实际删除结点的父结点（如果存在的话）
    virtual bool bs_deletenode(avlnode<K, V> *node, avlnode<K, V>*& cur, int& truedel) = 0;
};

// 定义map的事件类

// implement派生类实现抽象基类
template <typename K, typename V>
class Iavlmap_mgr_implement : public Iavlmap_mgr<K, V>
{
private:
    avlnode<K, V> *avlroot;
    uint32_t size;

public:
    void init() override;
    uint32_t Height() override;
    bool insert(K key, V value) override;
    bool erase(K key) override;
    // 根据键查询map的值，返回值的常量指针，若不存在返回nullptr
    const V* get(K key) override;
    const V* operator[](K key) override;
    void levelTraversal() override;
    void inorderTraversal() override;
    // 层析遍历释放堆区空间
    virtual ~Iavlmap_mgr_implement() override;

private:
    uint32_t avlHeight(avlnode<K, V> *root) override;
    void levelTraversal(avlnode<K, V> *root) override;
    void inorderTraversal(avlnode<K, V> *root) override;
    avlnode<K, V> *findavlnode(K key) override;
    void RotateLL(avlnode<K, V> *parent) override;
    void RotateLR(avlnode<K, V> *parent) override;
    void RotateRR(avlnode<K, V> *parent) override;
    void RotateRL(avlnode<K, V> *parent) override;
    void del_RotateLL(avlnode<K, V> *parent) override;
    void del_RotateLR(avlnode<K, V> *parent) override;
    void del_RotateRR(avlnode<K, V> *parent) override;
    void del_RotateRL(avlnode<K, V> *parent) override;
    avlnode<K, V> *findpre(avlnode<K, V> *node) override;
    avlnode<K, V> *findpost(avlnode<K, V> *node) override;
    bool bs_deletenode(avlnode<K, V> *node, avlnode<K, V>*& cur, int& truedel) override;
};

template <typename K, typename V>
void Iavlmap_mgr_implement<K, V>:: init() {
    this->size = 0;
    this->avlroot = nullptr;
}

template <typename K, typename V>
uint32_t Iavlmap_mgr_implement<K, V>:: Height() {
    return this->avlHeight(this->avlroot);
}

template <typename K, typename V>
bool Iavlmap_mgr_implement<K, V>:: insert(K key, V value) {
    // 特判原先avlmap为空的情况
    if (avlroot == nullptr) {
        avlnode<K, V> *cur = new avlnode<K, V>(key, value);
        this->avlroot = cur;
        return true;
    }
    else {
        // 检查key值是否已经存在
        const V *iskeyexists = this->get(key);
        if (iskeyexists != nullptr) {
            return false;
        }
        avlnode<K, V> *cur = new avlnode<K, V>(key, value);
        // 从原先的avlmap中按照二叉搜索树的性质查找插入结点
        avlnode<K, V> *pfind = this->avlroot;
        avlnode<K, V> *pfather = nullptr;
        while (pfind != nullptr) {
            if (key < (pfind->kv.first)) {
                pfather = pfind;
                pfind = pfind->left;
            }
            else {
                pfather = pfind;
                pfind = pfind->right;
            }
        }
        
        // 进行插入操作
        cur->parent = pfather;
        if (key < pfather->kv.first) {
            pfather->left = cur;
        }
        else {
            pfather->right = cur;
        }
        this->size++;
        
        // 检查是否需要进行旋转调整
        while(pfather != nullptr) {
            // 根据新结点所属子树的位置调整其父结点的bf值
            if(cur->kv.first < pfather->kv.first) {
                pfather->bf--;
            }
            else { //
                pfather->bf++;
            }
            // 根据pfather->bf此时的值判断是否需要旋转
            if (pfather->bf == 0) { // 此时表明插入后父结点左右两颗子树的高度平衡，那么无需调整，因为插入前插入后父结点所属的子树高度没有变化
                break;
            }
            else if(pfather->bf == -1 || pfather->bf == 1) { // 此时表明父结点所属子树的高度发生了变化
                // 此时需要继续向上检索进行可能的调整
                cur = pfather;
                pfather = pfather->parent;
            }
            else {  // 出现了最小不平衡子树
                if(pfather->bf == 2) { // 大类型是R
                    if(cur->bf < 0) {   // RL类型
                        this->RotateRL(pfather);
                    }
                    else if(cur->bf > 0) {  // RR类型
                        this->RotateRR(pfather);
                    }
                }
                else if(pfather->bf == -2) { // 大类型是L
                    if(cur->bf < 0) {   // LL类型
                        this->RotateLL(pfather);
                    }
                    else if(cur->bf >0) {   // LR类型
                        this->RotateLR(pfather);
                    }
                }
                // avltree的性质保证调整最小不平衡子树即可使整体平衡，因为调整后失衡子树恢复为原来的高度了
                break;
            }            
        }
        // 插入成功
        return true;
    }
}

template <typename K, typename V>
const V* Iavlmap_mgr_implement<K, V>:: get(K key) {
    avlnode<K, V>* pfind = this->avlroot;
    while(pfind != nullptr && pfind->kv.first != key) {
        if (key < (pfind->kv.first)) {
            pfind = pfind->left;
        }
        else if (key > (pfind->kv.first)) {
            pfind = pfind->right;
        }
    }
    if (pfind != nullptr) { //查找到了对应的键
        return &(pfind->kv.second);
    }
    else {  // 未查找到对应的键
        return nullptr;
    }
}

template <typename K, typename V>
const V* Iavlmap_mgr_implement<K, V>::operator[](K key) {
    return this->get(key);
}

template <typename K, typename V>
bool Iavlmap_mgr_implement<K, V>:: erase(K key) {
    const avlnode<K, V> *deletep = this->findavlnode(key);
    // 指示删除的值
    int truedel = 0;
    int dvalue = deletep->kv.first;
    if (deletep == nullptr) { // 原map中不存在的键
        return false;
    }
    else {  // 原map中存在的键
        avlnode<K, V> *imbalnode = deletep->parent;
        // 执行BST的删除方式
        avlnode<K, V> *cur = nullptr;
        // 此时cur指向的是实际删除结点的父结点
        bool flag = this->bs_deletenode((avlnode<K, V>*)deletep, cur, truedel);
        // 执行向上调整，不同与插入，删除avl树的结点需要不断往上检索
        while(cur != nullptr) {
            if(cur->kv.first < dvalue) {
                cur->bf--;
            }
            else if(cur->kv.first > dvalue) {
                cur->bf++;
            }

            if(cur->bf == 1 || cur->bf == -1) { // 表明此时以cur为根结点的子树的高度没有发生改变，无需继续向上调整了
                break;
            }
            else if(cur->bf == 0) { // 以cur为根结点的子树一定总体高度减少了1，需要继续向上检查
                dvalue = cur->kv.first;
                cur = cur->parent;
            }
            // 找到了失衡点，进行旋转调整，旋转调整后的子树高度-1
            else if(cur->bf == 2) {  // 右侧子树较高
                // 寻找高度最高的孙子
                uint32_t rlh = this->avlHeight(cur->right->left);
                uint32_t rrh = this->avlHeight(cur->right->right);
                if(rrh >= rlh) {    // RR旋转类型
                    avlnode<K, V> *rson = cur->right;
                    this->del_RotateRR(cur);
                    dvalue = rson->kv.first;
                    cur = rson->parent;
                }
                else {  // RL旋转类型
                    avlnode<K, V> *rlgrandson = cur->right->left;
                    this->del_RotateRL(cur);
                    dvalue = rlgrandson->kv.first;
                    cur = rlgrandson->parent;
                }
            }
            // 找到了失衡点，进行旋转调整
            else if(cur->bf == -2) {    // 左侧子树较高
                uint32_t llh = this->avlHeight(cur->left->left);
                uint32_t lrh = this->avlHeight(cur->left->right);
                if(llh>=lrh) {  // LL旋转类型
                    avlnode<K, V> *lson = cur->left;
                    this->del_RotateLL(cur);
                    dvalue = lson->kv.first;
                    cur = lson->parent;
                }
                else {  // LR旋转类型
                    avlnode<K, V> *lrgrandson = cur->left->right;
                    this->del_RotateLR(cur);
                    dvalue = lrgrandson->kv.first;
                    cur = lrgrandson->parent;
                }
            }
        }
        return true;
    }
}

template <typename K, typename V>
void Iavlmap_mgr_implement<K, V>:: levelTraversal() {
    std::cout << "avlmap info(level traversal)" << std::endl;
    std::cout << "avl height: " << this->avlHeight(this->avlroot) << std::endl;
    this->levelTraversal(this->avlroot);
}

template <typename K, typename V>
void Iavlmap_mgr_implement<K, V>:: inorderTraversal() {
    std::cout << "avlmap info(inorder traversal)" << std::endl;
    std::cout << "avl height: " << this->avlHeight(this->avlroot) << std::endl;
    this->inorderTraversal(this->avlroot);
}

template <typename K, typename V>
Iavlmap_mgr_implement<K, V>:: ~Iavlmap_mgr_implement() {
    this->size = 0;
    std::queue<avlnode<K, V> *> q;
    if(this->avlroot != nullptr) {
        q.push(this->avlroot);
    }
    while (!q.empty()) {
        avlnode<K, V> *dptr = q.front();
        q.pop();
        if(dptr->left != nullptr) {
            q.push(dptr->left);
        }
        if(dptr->right != nullptr) {
            q.push(dptr->right);
        }
        delete dptr;
    }
}

// 利用平衡因子的高度计算方式，复杂度优化至O(log2n)
template <typename K, typename V>
uint32_t Iavlmap_mgr_implement<K, V>:: avlHeight(avlnode<K, V>* root) {
    if(root == nullptr) {
        return 0;
    }
    else {
        if(root->bf <= 0) { // 此时左子树较高或两棵子树相等高
            return avlHeight(root->left) + 1;
        }
        else {  // 此时右子树较高
            return avlHeight(root->right) + 1;
        }
    }
}

template <typename K, typename V>
void Iavlmap_mgr_implement<K, V>:: levelTraversal(avlnode<K, V> *root) {
    std::queue<avlnode<K, V> *> q;
    avlnode<K, V> *p = root;
    q.push(p);
    while(!q.empty()) {
        p = q.front();
        q.pop();
        std::cout << "<" << p->kv.first << ", " << p->kv.second << ">" << " " << p->bf << std::endl;
        if(p->left != nullptr) {
            q.push(p->left);
        }
        if(p->right != nullptr) {
            q.push(p->right);
        }
    }
}

template <typename K, typename V>
void Iavlmap_mgr_implement<K, V>:: inorderTraversal(avlnode<K, V> *root) {
    if(root != nullptr) {
        inorderTraversal(root->left);
        std::cout << "<" << root->kv.first << ", " << root->kv.second << ">" << "; bf: " << root->bf << std::endl;
        inorderTraversal(root->right);
    }
}

template <typename K, typename V>
avlnode<K, V>* Iavlmap_mgr_implement<K, V>:: findavlnode(K key) {
    avlnode<K, V>* pfind = this->avlroot;
    while(pfind != nullptr && pfind->kv.first != key) {
        if (key < (pfind->kv.first)) {
            pfind = pfind->left;
        }
        else if (key > (pfind->kv.first)) {
            pfind = pfind->right;
        }
    }
    return pfind;
}

template <typename K, typename V>
void Iavlmap_mgr_implement<K, V>:: RotateLL(avlnode<K, V>* parent) {
    // 更新pparent
    avlnode<K, V> *pparent = parent->parent;
    avlnode<K, V> *lson = parent->left;
    if (pparent == nullptr) {   // 此时parent结点就是根结点
        // 更新avlroot指针指向的结点
        lson->parent = nullptr;
        this->avlroot = lson;
    }
    else {
        if(parent == pparent->left) {   // parent结点是pparent结点的左孩子
            pparent->left = lson;
        }
        else {  // parent结点是pparent结点的右孩子
            pparent->right = lson;
        }
        lson->parent = pparent;
    }
    // 调整不平衡子树
    if (lson->right != nullptr) { // lson的右子树存在
        lson->right->parent = parent;
        parent->left = lson->right;
    }
    else {  // lson的右子树不存在
        parent->left = nullptr;
    }
    parent->parent = lson;
    lson->right = parent;
    lson->bf = 0;
    parent->bf = 0;
}

template <typename K, typename V>
void Iavlmap_mgr_implement<K, V>::RotateRR(avlnode<K, V> *parent) {
    // 更新pparent
    avlnode<K, V> *pparent = parent->parent;
    avlnode<K, V> *rson = parent->right;
    if (pparent == nullptr) {   // 此时parent结点就是根结点
        rson->parent = nullptr;
        this->avlroot = rson;
    }
    else {
        if(pparent->left == parent) {   // parent结点是pparent结点的左孩子
            pparent->left = rson;
        }
        else {  // parent结点是pparent结点的右孩子
            pparent->right = rson;
        }
        rson->parent = pparent;
    }
    // 调整不平衡子树f
    if (rson->left != nullptr) {    // rson的左子树存在
        rson->left->parent = parent;
        parent->right = rson->left;
    }
    else {  // rson的左子树不存在
        parent->right = nullptr;
    }
    parent->parent = rson;
    rson->left = parent;
    parent->bf = 0;
    rson->bf = 0;
}

template <typename K, typename V>
void Iavlmap_mgr_implement<K, V>::RotateLR(avlnode<K, V>* parent) {
    // 获取将要调整的孩子，孙子结点指针
    avlnode<K, V> *lson = parent->left;
    avlnode<K, V> *lrgrandson = lson->right;
    
    // 调整不平衡子树，下面调用的单旋函数对于lson,lrgrandson结点的bf计算都是错误的，需要分类讨论重新调整
    // 以lson为parent执行一次左旋
    this->RotateRR(lson);
    // 第一次左旋后，lrgrandson一定作为左旋子树后的parent了，再做一次右单旋
    this->RotateLL(parent);

    // 根据三种grandson的值判断bf该如何赋值
    int grandson_bf = lrgrandson->bf;
    if(grandson_bf == 0) { // 将要旋转的子树只有三个结点的特殊情况
        parent->bf = 0;
        lson->bf = 0;
        lrgrandson->bf = 0;
    }
    else if(grandson_bf == 1) { // LR情形下插入了LR的右子树处
        parent->bf = 0;
        lson->bf = -1;
        lrgrandson = 0;
    }
    else if(grandson_bf == -1) { // LR情形下插入了LR的左子树处
        parent->bf = 0;
        lson->bf = -1;
        lrgrandson = 0;
    }
}

template <typename K, typename V>
void Iavlmap_mgr_implement<K, V>::RotateRL(avlnode<K, V>* parent) {
    // 获取将要调整的孩子，孙子结点指针
    avlnode<K, V> *rson = parent->right;
    avlnode<K, V> *rlgrandson = rson->left;

    // 调整不平衡子树，下面调用的单旋函数对于rson,rlgrandson结点的bf计算都是错误的，需要分类讨论重新调整
    // 以rson为parent执行一次右单旋
    this->RotateLL(rson);
    // 第一次右旋后，rlgrandson一定作为右旋子树后的parent了，再做一次左单旋
    this->RotateRR(parent);

    // 根据三种grandson的值判断bf该如何赋值
    int grandson_bf = rlgrandson->bf;
    if (grandson_bf == 0) { // 将要旋转的子树只有三个结点的特殊情况
        parent->bf = 0;
        rson->bf = 0;
        rlgrandson->bf = 0;
    }
    else if(grandson_bf == 1) { // RL情形下插入了RL的右子树处
        parent->bf = -1;
        rson->bf = 0;
        rlgrandson->bf = 0;
    }
    else if(grandson_bf == -1) { // RL情形下插入了RL的左子树处
        parent->bf = 0;
        rson->bf = 1;
        rlgrandson->bf = 0;
    }
}

template <typename K, typename V>
void Iavlmap_mgr_implement<K, V>::del_RotateLL(avlnode<K, V>* parent) {
    avlnode<K, V> *lson = parent->left;
    int ori_bf = lson->bf;
    this->RotateLL(parent);
    if (ori_bf == 0) {
        lson->bf = 1;
        parent->bf = -1;
    }
    else if(ori_bf == -1) {
        lson->bf = 0;
        parent->bf = 0;
    }
}

template <typename K, typename V>
void Iavlmap_mgr_implement<K, V>::del_RotateRR(avlnode<K, V>* parent) {
    avlnode<K, V> *rson = parent->right;
    int ori_bf = rson->bf;
    this->RotateRR(parent);
    if(ori_bf == 0) {
        rson->bf = -1;
        parent->bf = 1;
    }
    else if (ori_bf == 1) {
        rson->bf = 0;
        parent->bf = 0;
    }
}

template <typename K, typename V>
void Iavlmap_mgr_implement<K, V>::del_RotateLR(avlnode<K, V>* parent) {
    avlnode<K, V> *lson = parent->left;
    avlnode<K, V> *lrgrandson = lson->right;
    int ori_bf = lrgrandson->bf;
    this->RotateLR(parent);
    if (ori_bf == 1) { // lrgrandson的右子树高于左子树
        lrgrandson->bf = 0;
        lson->bf = -1;
        parent->bf = 0;
    }
    else if(ori_bf == 0) {  // lrgrandson的两棵子树高度相等
        lrgrandson->bf = 0;
        lson->bf = 0;
        parent->bf = 0;
    }
    else if(ori_bf == -1) { // lrgrandson的左子树高度高于右子树
        lrgrandson->bf = 0;
        lson->bf = 0;
        parent->bf = 1;
    }
}

template <typename K, typename V>
void Iavlmap_mgr_implement<K, V>::del_RotateRL(avlnode<K, V>* parent) {
    avlnode<K, V> *rson = parent->right;
    avlnode<K, V> *rlgrandson = rson->left;
    int ori_bf = rlgrandson->bf;
    this->RotateRL(parent);
    if (ori_bf == 1) { // rlgrandson的右子树高于左子树
        rlgrandson->bf = 0;
        rson->bf = 0;
        parent->bf = -1;
    }
    else if(ori_bf == 0) {  // rlgrandson的两棵子树高度相等
        rlgrandson->bf = 0;
        rson->bf = 0;
        parent->bf = 0;
    }
    else if(ori_bf == -1) { // rlgrandson的左子树高于右子树
        rlgrandson->bf = 0;
        rson->bf = 1;
        parent->bf = 0;
    }
}

template <typename K, typename V>
avlnode<K, V>* Iavlmap_mgr_implement<K, V>:: findpre(avlnode<K, V>* node) {
    avlnode<K, V> *f = node;
    avlnode<K, V> *prenode = node->left;
    while (prenode != nullptr) {
        f = prenode;
        prenode = prenode->right;
    }
    return (f == node) ? nullptr : f;
}

template <typename K, typename V>
avlnode<K, V>* Iavlmap_mgr_implement<K, V>:: findpost(avlnode<K, V>* node) {
    avlnode<K, V> *f = node;
    avlnode<K, V> *postnode = node->right;
    while(postnode != nullptr) {
        f = postnode;
        postnode = postnode->left;
    }
    return (f == node) ? nullptr : f;
}

template <typename K, typename V>
bool Iavlmap_mgr_implement<K, V>:: bs_deletenode(avlnode<K, V>* node, avlnode<K, V>*& cur, int& truedel) {
    if(node == nullptr) {   // 不存在的结点
        cur = nullptr;
        return false;
    }
    else {
        avlnode<K, V> *father = node->parent;
        if (node->left == nullptr && node->right == nullptr) { // 被删结点是叶子结点
            if(father == nullptr) { // 此时是整棵avl树只有一个结点的情况
                this->avlroot = nullptr;
            }
            else {
                if(father->left == node) {
                    father->left = nullptr;
                }
                else if(father->right == node) {
                    father->right = nullptr;
                }
            }
            cur = father;
            truedel = node->kv.first;
            delete node;
        }
        else if(node->left != nullptr && node->right == nullptr) {  // 被删结点仅存在左子树
            if(father == nullptr) {
                this->avlroot = node->left;
            }
            else {
                if(father->left == node) {
                    father->left = node->left;
                }
                else if(father->right == node) {
                    father->right = node->left;
                }
            }
            cur = father;
            truedel = node->kv.first;
            delete node;
        }
        else if(node->left == nullptr && node->right != nullptr) {  // 被删结点仅存在右子树
            if(father == nullptr) {
                this->avlroot = node->right;
            }
            else {
                if(father->left == node) {
                    father->left = node->right;
                }
                else if (father->right == node) {
                    father->right = node->right;
                }
            }
            cur = father;
            truedel = node->kv.first;
            delete node;
        }
        else if(node->left != nullptr && node->right != nullptr) {
            // 统一寻找前驱代替，且一定存在前驱
            avlnode<K, V> *pre = this->findpre(node);
            node->kv = pre->kv;
            // 如果pre是非叶子结点，那么仅可能会是只有一侧子树的情况，递归调用函数只会执行一次，作用是删掉前驱结点
            this->bs_deletenode(pre, cur, truedel);
        }
        return true;
    }
}

// 工厂函数
template <typename K, typename V>
Iavlmap_mgr<K, V>* create_avl_map() {
    return new Iavlmap_mgr_implement<K, V>;
}