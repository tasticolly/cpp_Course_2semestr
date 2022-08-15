#include <cstddef>
#include <iterator>
#include <memory>
#include <iostream>
#include <vector>
#include <cmath>

template<typename T, typename Allocator_T = std::allocator<T>>
class List {
  using AllocTraits = std::allocator_traits<Allocator_T>;
 private:
  struct BaseNode {
    BaseNode* next = nullptr;
    BaseNode* prev = nullptr;
    BaseNode() = default;
    BaseNode(const BaseNode& other) = default;
    BaseNode(BaseNode&& other) noexcept = default;
    BaseNode(BaseNode* next, BaseNode* prev): next(next), prev(prev) {}
    BaseNode& operator=(BaseNode&& other) noexcept = default;
    BaseNode& operator=(const BaseNode& other) noexcept = default;
  };

  struct Node: BaseNode {
    T* pvalue;
    Node(const Node& other) = default;
    Node(Node&& other) noexcept = default;
    Node(T* pval): pvalue(pval) {};
    Node& operator=(Node&& other) noexcept = default;
    Node& operator=(const Node& other) noexcept = default;
  };

  void destruct() noexcept {
    clear();
    BaseNodeTraits::destroy(baseAllocator_, fakeNode_);
    BaseNodeTraits::deallocate(baseAllocator_, fakeNode_, 1);
  }

  using NodeAlloc = typename AllocTraits::template rebind_alloc<Node>;
  using NodeTraits = std::allocator_traits<NodeAlloc>;


  using BaseNodeAlloc = typename AllocTraits::template rebind_alloc<BaseNode>;
  using BaseNodeTraits = std::allocator_traits<BaseNodeAlloc>;

  void swap(List& other) {
    std::swap(allocator_, other.allocator_);
    std::swap(baseAllocator_, other.baseAllocator_);
    std::swap(size_, other.size_);
    std::swap(other.fakeNode_, fakeNode_);
  }

 public:

  //-------------------------CONSTUCTORS----------------------------------------------------

  List(const Allocator_T& alloc = Allocator_T()): allocator_(alloc), baseAllocator_(alloc) {
    fakeNode_ = BaseNodeTraits::allocate(baseAllocator_, 1);
    try {
      BaseNodeTraits::construct(baseAllocator_, fakeNode_, fakeNode_, fakeNode_);
    } catch (...) {
      BaseNodeTraits::deallocate(baseAllocator_, fakeNode_, 1);
    }

  }
  List(size_t num, const T& value, const Allocator_T& alloc = Allocator_T()): List(alloc) {
    try {
      for (size_t i = 0; i < num; ++i) {
        push_back(value);
      }
    } catch (...) {
      clear();
      throw;
    }
  }

  List(size_t num, const Allocator_T& alloc = Allocator_T()): List(alloc) {
    try {
      while (num--) {
        emplace(begin());
      }
    } catch (...) {
      clear();
      throw;
    }

  }

  List(const List& other)
      : List(AllocTraits::select_on_container_copy_construction(other.get_allocator())) {
    try {
      for (auto it = other.begin(); it != other.end(); ++it) {
        push_back(*it);
      }
    } catch (...) {
      clear();
      throw;
    }
  }

  List(List&& other) noexcept
      : List(std::move(AllocTraits::select_on_container_copy_construction(other.get_allocator()))) {
    std::swap(fakeNode_, other.fakeNode_);
    size_ = other.size_;
    other.size_ = 0;
  }

  //------------------------------------------OPERATOR = ---------------------------
  List& operator=(const List& other) {
    auto copy = AllocTraits::propagate_on_container_copy_assignment::value ? List<T, Allocator_T>(other.allocator_) :
                List<T, Allocator_T>(AllocTraits::select_on_container_copy_construction(other.allocator_));
    try {
      for (auto it = other.begin(); it != other.end(); ++it) {
        copy.push_back(*it);
      }
    } catch (...) {
      copy.clear();
      throw;
    }
    swap(copy);
    return *this;
  }

  List& operator=(List&& other) noexcept {
    if (this != &other) {
      clear();
      allocator_ = std::move(AllocTraits::propagate_on_container_copy_assignment::value ? other.allocator_ : NodeAlloc(
          AllocTraits::select_on_container_copy_construction(other.allocator_)));
      baseAllocator_ =
          std::move(AllocTraits::propagate_on_container_copy_assignment::value ? other.baseAllocator_ : BaseNodeAlloc(
              AllocTraits::select_on_container_copy_construction(other.allocator_)));
      size_ = other.size_;
      std::swap(fakeNode_, other.fakeNode_);
      other.size_ = 0;
    }
    return *this;
  }
  void clear() noexcept {
    while (size_) {
      pop_back();
    }
  }

  ~List() noexcept {
    destruct();
  }

  template<bool is_const>
  class CommonIterator;
  using iterator = CommonIterator<false>;
  using const_iterator = CommonIterator<true>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  template<bool is_const>
  class CommonIterator {
   public:
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = T;
    using difference_type = long long;
    using pointer = typename std::conditional<is_const, const T*, T*>::type;
    using reference = typename std::conditional<is_const, const T&, T&>::type;
    BaseNode* getNode() {
      return node_;
    }
    CommonIterator() = default;
    CommonIterator(BaseNode* elem): node_(elem) {}
    CommonIterator(const CommonIterator& elem) = default;
    ~CommonIterator() = default;

    reference operator*() { return *static_cast<Node*>(node_)->pvalue; }
    pointer operator->() { return static_cast<Node*>(node_)->pvalue; }

    bool operator==(const CommonIterator& other) const { return node_ == other.node_; }
    bool operator!=(const CommonIterator& other) const { return node_ != other.node_; }

    operator const_iterator() const { return const_iterator(node_); }

    CommonIterator& operator++() {
      node_ = node_->next;
      return *this;
    }

    CommonIterator operator++(int) {
      CommonIterator copy = *this;
      ++(*this);
      return copy;
    }

    CommonIterator& operator--() {
      node_ = node_->prev;
      return *this;
    }

    CommonIterator operator--(int) {
      CommonIterator copy = *this;
      --(*this);
      return copy;
    }
   private:
    BaseNode* node_;
  };

  iterator begin() {
    return iterator(fakeNode_->next);
  }
  iterator end() {
    return iterator(fakeNode_);
  }
  const_iterator end() const {
    return const_iterator(fakeNode_);
  }
  const_iterator begin() const {
    return cbegin();
  }
  const_iterator cbegin() const {
    return const_iterator(fakeNode_->next);
  }
  const_iterator cend() const {
    return const_iterator(fakeNode_);
  }
  reverse_iterator rbegin() {
    return reverse_iterator(fakeNode_);
  }
  reverse_iterator rend() {
    return reverse_iterator(fakeNode_->next);
  }
  const_reverse_iterator rbegin() const {
    return crbegin();
  }
  const_reverse_iterator rend() const {
    return crend();
  }
  const_reverse_iterator crbegin() const {
    return const_reverse_iterator(fakeNode_);
  }
  const_reverse_iterator crend() const {
    return const_reverse_iterator(fakeNode_->next);
  }

  template<typename... Args>
  void emplace(const_iterator iter, Args&& ... args) {
    auto tmp = get_allocator();
    Node* newNode;
    T* newElem = AllocTraits::allocate(tmp, 1);
    try {
      AllocTraits::construct(tmp, newElem, std::forward<Args>(args)...);
    } catch (...) {
      AllocTraits::deallocate(tmp, newElem, 1);
      throw;
    }
    try {
      newNode = NodeTraits::allocate(allocator_, 1);
    } catch (...) {
      AllocTraits::destroy(tmp, newElem);
      AllocTraits::deallocate(tmp, newElem, 1);
      throw;
    }
    try {
      NodeTraits::construct(allocator_, newNode, newElem);
    } catch (...) {
      NodeTraits::deallocate(allocator_, newNode, 1);
      AllocTraits::destroy(tmp, newElem);
      AllocTraits::deallocate(tmp, newElem, 1);
      throw;
    }

    newNode->next = iter.getNode();
    newNode->prev = iter.getNode()->prev;
    iter.getNode()->prev->next = static_cast<BaseNode*>(newNode);
    iter.getNode()->prev = static_cast<BaseNode*>(newNode);
    ++size_;
  }

  void emplace(const_iterator iter, T* pvalue) {
    Node* newNode = NodeTraits::allocate(allocator_, 1);
    try {
      NodeTraits::construct(allocator_, newNode, pvalue);
    } catch (...) {
      NodeTraits::deallocate(allocator_, newNode, 1);
    }
    newNode->next = iter.getNode();
    newNode->prev = iter.getNode()->prev;
    iter.getNode()->prev->next = static_cast<BaseNode*>(newNode);
    iter.getNode()->prev = static_cast<BaseNode*>(newNode);
    ++size_;
  }

  void push_back(T* pvalue) {
    emplace(end(), pvalue);
  }

  void push_front(T* pvalue) {
    emplace(begin(), pvalue);
  }

  void redirectForErase(const_iterator iter) noexcept {
    auto tmp = get_allocator();
    iter.getNode()->next->prev = iter.getNode()->prev;
    iter.getNode()->prev->next = iter.getNode()->next;

    AllocTraits::destroy(tmp, (iterator(iter.getNode())).operator->());
    AllocTraits::deallocate(tmp, (iterator(iter.getNode())).operator->(), 1);
    NodeTraits::destroy(allocator_, static_cast<Node*>(iter.getNode()));
    NodeTraits::deallocate(allocator_, static_cast<Node*>(iter.getNode()), 1);
    --size_;
  }

  const_iterator insert(const_iterator iter, const T& elem) {
    emplace(iter, elem);
    return iterator(static_cast<BaseNode*>(iter.getNode()->prev));
  }

  const_iterator insert(const_iterator iter, T&& elem) {
    emplace(iter, std::move(elem));
    return iterator(static_cast<BaseNode*>(iter.getNode()->prev));
  }

  iterator erase(const_iterator iter) noexcept {
    auto prev = --iter;
    redirectForErase(++iter);
    return iterator(prev.getNode());

  }

  void push_back(const T& elem) {
    emplace(end(), elem);
  }
  void push_back(T&& elem) {
    emplace(end(), std::move(elem));
  }

  void push_front(const T& elem) {
    emplace(begin(), elem);
  }

  void push_front(T&& elem) {
    emplace(begin(), std::move(elem));
  }

  void pop_back() noexcept {
    redirectForErase(--end());
  }
  void pop_front() noexcept {
    redirectForErase(begin());
  }

  size_t size() const noexcept {
    return size_;
  }

  Allocator_T get_allocator() const {
    return Allocator_T(allocator_);
  }

  void destroyNode(const_iterator iter) noexcept {
    iter.getNode()->next->prev = iter.getNode()->prev;
    iter.getNode()->prev->next = iter.getNode()->next;
    NodeTraits::destroy(allocator_, static_cast<Node*>(iter.getNode()));
    NodeTraits::deallocate(allocator_, static_cast<Node*>(iter.getNode()), 1);
    --size_;
  }

 private:
  mutable BaseNode* fakeNode_ = nullptr;
  size_t size_ = 0;
  NodeAlloc allocator_;
  BaseNodeAlloc baseAllocator_;

};

template<typename Key, typename Value, typename Hash = std::hash<Key>, typename EqualTo = std::equal_to<Key>,
    typename Alloc = std::allocator<std::pair<const Key, Value>>>
class UnorderedMap {
 public:
  using NodeType = std::pair<const Key, Value>;
  using AllocTraits = std::allocator_traits<Alloc>;

  using iterator = typename List<NodeType, Alloc>::iterator;
  using const_iterator = typename List<NodeType, Alloc>::const_iterator;

  size_t size() const noexcept {
    return dataList.size();
  }

  iterator begin() {
    return iterator(dataList.begin());
  }
  iterator end() {
    return iterator(dataList.end());
  }
  const_iterator end() const {
    return cend();
  }
  const_iterator begin() const {
    return cbegin();
  }
  const_iterator cbegin() const {
    return const_iterator(dataList.begin());
  }
  const_iterator cend() const {
    return const_iterator(dataList.end());
  }

  const_iterator find(const Key& key) const {
    return find(key, getHash(key));
  }

  iterator find(const Key& key) {
    return find(key, getHash(key));

  }

  const_iterator find(Key&& key) const {
    return find(std::move(key), getHash(key));
  }

  iterator find(Key&& key) {
    return find(std::move(key), getHash(key));

  }

  void rehash(const size_t& count) {

    auto copy = List<NodeType, Alloc>(allocator_);

    table =
        std::vector<tableElem, typename AllocTraits::template rebind_alloc<tableElem>>(count,
                                                                                       {copy.end(), copy.end()},
                                                                                       allocator_);

    sizeOfTable_ = count;
    while (begin() != end()) {
      auto it = begin();
      auto& currentKey = it->first;
      size_t currentHash = getHash(currentKey);
      if (table[currentHash].second == end()) {
        copy.push_front(it.operator->());
        dataList.destroyNode(it);
        table[currentHash] = {begin(), begin()};
      } else {
        copy.emplace(++table[currentHash].second, it.operator->());
        dataList.destroyNode(it);
        --table[currentHash].second;
      }
    }
    dataList = std::move(copy);
  }

  void reserve(size_t count) {
    if (count > size())
      rehash(std::ceil(static_cast<float>(count) / maxLoadFactor_));
  }

  float load_factor() const noexcept {
    return float(size()) / float(sizeOfTable_);
  }

  float max_load_factor() const noexcept {
    return maxLoadFactor_;
  }

  void max_load_factor(float ml) {
    maxLoadFactor_ = ml;
  }

  size_t max_size() {
    return AllocTraits::max_size(allocator_);
  }

  template<typename... Args>
  std::pair<iterator, bool> emplace(Args&& ... args) {
    if (load_factor() >= maxLoadFactor_) {
      rehash(sizeOfTable_ * 2);
    }
    NodeType* newKeyValue = AllocTraits::allocate(allocator_, 1);
    try {
      AllocTraits::construct(allocator_, newKeyValue, std::forward<Args>(args)...);
    } catch (...) {
      AllocTraits::deallocate(allocator_, newKeyValue, 1);
    }
    auto& currentKey = (*newKeyValue).first;
    size_t currentHash = getHash(currentKey);
    auto pos = find(currentKey, currentHash);
    auto result = std::make_pair(pos, false);

    if (pos == end()) {
      if (table[currentHash].second == end()) {
        dataList.push_front(newKeyValue);
        table[currentHash] = {begin(), begin()};
        result = std::make_pair(begin(), true);
      } else {
        dataList.emplace(++table[currentHash].second, newKeyValue);
        result = std::make_pair(--table[currentHash].second, true);

      }
    } else {
      AllocTraits::destroy(allocator_, newKeyValue);
      AllocTraits::deallocate(allocator_, newKeyValue, 1);
    }

    return result;
  }

  std::pair<iterator, bool> insert(const NodeType& elem) {
    return emplace(elem);
  }

  std::pair<iterator, bool> insert(NodeType&& elem) {
    return emplace(std::move(elem));
  }

  template<typename P>
  std::pair<iterator, bool> insert(P&& value) {
    return emplace(std::forward<P>(value));
  }

  template<typename Iter>
  void insert(Iter begin, Iter end) {
    reserve(size() + std::abs(std::distance(begin, end)));
    for (auto it = begin; it != end; ++it)
      insert(*it);
  }

  void erase(const_iterator begin, const_iterator end) noexcept {

    auto copy(begin);
    while (begin != end) {
      ++begin;
      erase(copy);
      copy = begin;
    }

  }

  void erase(const_iterator it) noexcept {
    size_t currentHash = getHash(it->first);
    if (table[currentHash].first == table[currentHash].second) {
      dataList.erase(it);
      table[currentHash] = {end(), end()};
    } else if (const_iterator(table[currentHash].second) == it) {
      table[currentHash].second = dataList.erase(it);
    } else if (const_iterator(table[currentHash].first) == it) {
      table[currentHash].first = ++dataList.erase(it);
    } else {
      dataList.erase(it);
    }
  }

  Value& at(const Key& key) {
    auto position = find(key);
    if (position != end())
      return position->second;
    else
      throw std::range_error("Wrong key");
  }

  const Value& at(const Key& key) const {
    auto position = find(key);
    if (position != end())
      return position->second;
    else
      throw std::range_error("Wrong key");
  }

  Value& at(Key&& key) {
    auto position = find(std::move(key));
    if (position != end())
      return position->second;
    else
      throw std::range_error("Wrong key");
  }

  const Value& at(Key&& key) const {
    auto position = find(std::move(key));
    if (position != end())
      return position->second;
    else
      throw std::range_error("Wrong key");
  }

  Value& operator[](const Key& key) {
    auto position = find(key);
    if (position != end()) {
      return position->second;
    }
    return insert({key, Value()}).first->second;
  }

//-----------------------------------------------CONSTRUCTORS-----------------------------------
  UnorderedMap(const Alloc& alloc = Alloc()): allocator_(alloc) {}
  UnorderedMap(size_t bucket_count): sizeOfTable_(bucket_count) {}

  UnorderedMap(const UnorderedMap& other)
      : UnorderedMap(AllocTraits::select_on_container_copy_construction(other.allocator_)) {
    insert(other.begin(), other.end());
  }

  UnorderedMap(UnorderedMap&& other) noexcept
      : sizeOfTable_(other.sizeOfTable_),
        maxLoadFactor_(other.maxLoadFactor_),
        allocator_(std::move(AllocTraits::select_on_container_copy_construction(other.allocator_))),
        dataList(std::move(other.dataList)),
        table(std::move(other.table)), equalTo_(std::move(other.equalTo_)), hash_(std::move(other.hash_)) {
    other.sizeOfTable_ = 0;

  }

  UnorderedMap& operator=(const UnorderedMap& other) {
    auto copy =
        AllocTraits::propagate_on_container_copy_assignment::value ? UnorderedMap<NodeType, Alloc>(other.allocator_) :
        UnorderedMap<NodeType, Alloc>(AllocTraits::select_on_container_copy_construction(other.allocator_));
    copy.insert(other.begin(), other.end());
    swap(copy);
    return *this;
  }

  UnorderedMap& operator=(UnorderedMap&& other) noexcept {
    if (this != &other) {
      allocator_ = std::move(AllocTraits::propagate_on_container_copy_assignment::value ? other.allocator_
                                                                                        : AllocTraits::select_on_container_copy_construction(
              other.allocator_));
      sizeOfTable_ = other.sizeOfTable_;
      other.sizeOfTable_ = 0;
      maxLoadFactor_ = other.maxLoadFactor_;
      dataList = std::move(other.dataList);
      table = std::move(other.table);
      equalTo_ = std::move(other.equalTo_);
      hash_ = std::move(other.hash_);

    }
    return *this;
  }

//----------------------------------------------DESTRUCTOR------------------------------------------

  ~UnorderedMap() = default;

 private:

  void destructList() noexcept {
    dataList.clear();
  }
  size_t getHash(const Key& key) const {
    return hash_(key) % sizeOfTable_;
  }

  iterator find(const Key& key, const size_t& currentHash) {
    auto tmp = ++table[currentHash].second;
    --table[currentHash].second;
    for (auto iter = table[currentHash].first; iter != tmp && iter != end(); ++iter) {
      if (equalTo_(key, iter->first)) {
        return iter;
      }
    }
    return end();
  }

  const_iterator find(const Key& key, const size_t& currentHash) const {
    return const_cast<UnorderedMap*>(this)->find(key, currentHash);
  }

  void clear() {
    while (size()) {
      erase(begin());
    }
  }
 private:

  void swap(UnorderedMap other) {
    std::swap(sizeOfTable_, other.sizeOfTable_);
    std::swap(maxLoadFactor_, other.maxLoadFactor_);
    std::swap(allocator_, other.allocator_);
    std::swap(dataList, other.dataList);
    std::swap(table, other.table);
    std::swap(equalTo_, other.equalTo_);
    std::swap(hash_, other.hash_);
  }

  size_t sizeOfTable_ = DEFAULT_SIZE_OF_TABLE;
  float maxLoadFactor_ = 0.7;
  Alloc allocator_ = Alloc();
  List<NodeType, Alloc> dataList = List<NodeType, Alloc>(allocator_);
  using tableElem = std::pair<iterator, iterator>;
  std::vector<tableElem, typename AllocTraits::template rebind_alloc<tableElem>> table =
      std::vector<tableElem, typename AllocTraits::template rebind_alloc<tableElem>>(sizeOfTable_,
                                                                                     {end(), end()},
                                                                                     allocator_);
  EqualTo equalTo_ = EqualTo();
  Hash hash_ = Hash();
  static const size_t DEFAULT_SIZE_OF_TABLE = 128;

};




