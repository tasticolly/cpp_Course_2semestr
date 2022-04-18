#include <cstddef>
#include <iterator>
#include <memory>
#include<iostream>
template<size_t N>
class StackStorage {

 public:
  char* getMemory() {
    return memory_;
  }
  char* getPointer() {
    return pointer_;
  }
  StackStorage() = default;
  char* allocate(size_t byte, size_t align) {
    char* p = pointer_ + (align - (reinterpret_cast<uintptr_t>(pointer_)) % align);

    pointer_ = p + byte;
    return p;
  }
  StackStorage(const StackStorage<N>&) = delete;
  void operator=(const StackStorage<N>&) = delete;
 private:
  char* pointer_ = memory_;
  char memory_[N];
};

template<typename T, size_t N>
class StackAllocator {
 public:
  using value_type = T;
  template<typename U>
  struct rebind {
    using other = StackAllocator<U, N>;
  };

  StackAllocator(const StackAllocator& other) = default;

  StackAllocator& operator=(const StackAllocator& other) {
    storage_ = other.storage_;
    return *this;
  }

  template<typename U, size_t M>
  friend
  class StackAllocator;

  template<typename U>
  StackAllocator(const StackAllocator<U, N>& other): storage_(other.storage_) {}

  StackAllocator(StackStorage<N>& storage): storage_(&storage) {}

  T* allocate(size_t num) {
    return reinterpret_cast<T*>(storage_->allocate(num * sizeof(T), alignof(T)));
  }
  template<typename... Args>
  void construct(T* p, const Args& ... args) {
    new(p) T(args...);
  }

  void destroy(T* p) {
    p->~T();
  }

  void deallocate(T*, size_t) {
  }

  template<typename T1, size_t N1, typename U, size_t M>
  friend bool operator==(const StackAllocator<T1, N1>& first, const StackAllocator<U, M>& second);

 private:
  StackStorage<N>* storage_;
};

template<typename T, size_t N, typename U, size_t M>
bool operator==(const StackAllocator<T, N>& first, const StackAllocator<U, M>& second) {
  return N == M && first.storage_ == second.storage_;
}

template<typename T, size_t N, typename U, size_t M>
bool operator!=(const StackAllocator<T, N>& first, const StackAllocator<U, M>& second) {
  return !(first == second);
}

template<typename T, typename Allocator_T = std::allocator<T>>
class List {
  using AllocTraits = std::allocator_traits<Allocator_T>;

 private:

  struct BaseNode {
    BaseNode* next = nullptr;
    BaseNode* prev = nullptr;
    BaseNode() = default;
    BaseNode(BaseNode* next, BaseNode* prev): next(next), prev(prev) {}
  };

  struct Node: BaseNode {
    T value;
    Node(BaseNode* next, BaseNode* prev, const T& value)
        : value(value) {
      BaseNode::prev = prev;
      BaseNode::next = next;
    }
    Node(BaseNode* next, BaseNode* prev): value() {
      BaseNode::prev = prev;
      BaseNode::next = next;
    }

  };

  void destruct() {

    while (size_) {
      pop_back();
    }
  }

  using NodeAlloc = typename AllocTraits::template rebind_alloc<Node>;
  using NodeTraits = std::allocator_traits<NodeAlloc>;

  void swap(List& other) {
    std::swap(allocator_, other.allocator_);
    std::swap(size_, other.size_);
    fakeNode_.next->prev = &other.fakeNode_;
    fakeNode_.prev->next = &other.fakeNode_;
    other.fakeNode_.next->prev = &fakeNode_;
    other.fakeNode_.prev->next = &fakeNode_;
    std::swap(other.fakeNode_, fakeNode_);
  }

 public:
  template<bool is_const>
  class CommonIterator;
  using iterator = CommonIterator<false>;
  using const_iterator = CommonIterator<true>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;


  //-------------------------CONSTUCTORS----------------------------------------------------

  List(const Allocator_T& alloc = Allocator_T()): fakeNode_(&fakeNode_, &fakeNode_), allocator_(alloc) {}
  List(size_t num, const T& value, const Allocator_T& alloc = Allocator_T()): List(alloc) {
    try {
      for (size_t i = 0; i < num; ++i) {
        push_back(value);
      }
    } catch (...) {
      destruct();
      throw;
    }
  }

  List(size_t num, const Allocator_T& alloc = Allocator_T()): List(alloc) {
    try {
      while (num--) {
        emplaceBefore(begin());
      }
    } catch (...) {
      destruct();
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
      destruct();
      throw;
    }
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
      copy.destruct();
      throw;
    }
    swap(copy);
    return *this;
  }

  ~List() {
    destruct();
  }

  template<bool is_const>
  class CommonIterator {
   public:
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = T;
    using difference_type = long long;
    using pointer = typename std::conditional<is_const, const T*, T*>::type;
    using reference = typename std::conditional<is_const, const T&, T&>::type;
    BaseNode* get_node() {
      return node_;
    }
    CommonIterator() = default;
    CommonIterator(BaseNode* elem): node_(elem) {}
    CommonIterator(const CommonIterator& elem) = default;
    ~CommonIterator() = default;

    reference operator*() { return static_cast<Node*>(node_)->value; }
    pointer operator->() { return &static_cast<Node*>(node_)->value; }

    bool operator==(const CommonIterator& other) const { return node_ == other.node_; }
    bool operator!=(const CommonIterator& other) const { return node_ != other.node_; }

    operator const_iterator() const { return const_iterator(node_); }

    CommonIterator& operator++() {
      *this = node_->next;
      return *this;
    }

    CommonIterator operator++(int) {
      CommonIterator copy = *this;
      ++(*this);
      return copy;
    }

    CommonIterator& operator--() {
      *this = node_->prev;
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
    return iterator(fakeNode_.next);
  }
  iterator end() {
    return iterator(&fakeNode_);
  }
  const_iterator end() const {
    return const_iterator(&fakeNode_);
  }
  const_iterator begin() const {
    return cbegin();
  }
  const_iterator cbegin() const {
    return const_iterator(fakeNode_.next);
  }
  const_iterator cend() const {
    return const_iterator(&fakeNode_);
  }
  reverse_iterator rbegin() {
    return reverse_iterator(&fakeNode_);
  }
  reverse_iterator rend() {
    return reverse_iterator(fakeNode_.next);
  }
  const_reverse_iterator rbegin() const {
    return crbegin();
  }
  const_reverse_iterator rend() const {
    return crend();
  }

  const_reverse_iterator crbegin() const {
    return const_reverse_iterator(&fakeNode_);
  }
  const_reverse_iterator crend() const {
    return const_reverse_iterator(fakeNode_->next);
  }

  void putBefore(const_iterator iter, const T& elem) {
    Node* newNode = NodeTraits::allocate(allocator_, 1);
    try {
      NodeTraits::construct(allocator_, newNode, iter.get_node(), iter.get_node()->prev, elem);
    } catch (...) {
      NodeTraits::deallocate(allocator_, newNode, 1);
      throw;
    }
    iter.get_node()->prev->next = static_cast<BaseNode*>(newNode);
    iter.get_node()->prev = static_cast<BaseNode*>(newNode);
    ++size_;
  }

  void emplaceBefore(const_iterator iter) {
    Node* newNode = NodeTraits::allocate(allocator_, 1);
    try {
      NodeTraits::construct(allocator_, newNode, iter.get_node(), iter.get_node()->prev);
    } catch (...) {
      NodeTraits::deallocate(allocator_, newNode, 1);
      throw;
    }
    iter.get_node()->prev->next = static_cast<BaseNode*>(newNode);
    iter.get_node()->prev = static_cast<BaseNode*>(newNode);
    ++size_;
  }

  void redirect_for_erase(const_iterator iter) {
    iter.get_node()->next->prev = iter.get_node()->prev;
    iter.get_node()->prev->next = iter.get_node()->next;
    NodeTraits::destroy(allocator_, static_cast<Node*>(iter.get_node()));
    NodeTraits::deallocate(allocator_, static_cast<Node*>(iter.get_node()), 1);
    --size_;
  }

  const_iterator insert(const_iterator iter, const T& elem) {
    putBefore(iter, elem);
    return iterator(static_cast<BaseNode*>(iter.get_node()->prev));
  }

  const_iterator erase(const_iterator iter) {
    auto prev = --iter;
    redirect_for_erase(++iter);
    return prev;
  }

  void push_back(const T& elem) {
    putBefore(end(), elem);
  }
  void push_front(const T& elem) {
    putBefore(begin(), elem);
  }

  void pop_back() {
    redirect_for_erase(--end());
  }
  void pop_front() {
    redirect_for_erase(begin());
  }

  size_t size() const {
    return size_;
  }

  Allocator_T get_allocator() const {
    return Allocator_T(allocator_);
  }

 private:
  mutable BaseNode fakeNode_;
  size_t size_ = 0;
  NodeAlloc allocator_;
};

