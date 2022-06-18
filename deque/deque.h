#include <iostream>
template<typename Iterator>
class revers_iterator {
  Iterator Iter;
 public:
  revers_iterator() = default;
  revers_iterator(const revers_iterator& other) = default;
  revers_iterator(const Iterator& Iter): Iter(Iter) {}

  revers_iterator& operator++() { return --Iter; }
  revers_iterator operator++(int) { return Iter--; }
  revers_iterator& operator--() { return ++Iter; }
  revers_iterator operator--(int) { return Iter--; }

  revers_iterator& operator+=(int num) { return Iter -= num; }
  revers_iterator& operator-=(int num) { return Iter += num; }
  revers_iterator operator+(int num) { return Iter - num; }
  revers_iterator operator-(int num) { return Iter + num; }

  revers_iterator& operator*() { return *Iter; }
  revers_iterator* operator->() { return Iter.operator->(); }

};

template<typename T>
class Deque {

 public:
  //------------------------------CONSTRUCTORS------------------------------------
  Deque(): array_(new T* [1]), firstElem_(0), size_(0), capacity(0) {
    try {
      reserve(1);
    } catch (...) {
      delete[] array_;
      throw;
    }
  }
  Deque(const Deque& other): Deque() {

    reserve(greaterDivisibleByN(increasingConstant, other));
    for (size_t i = 0; i < other.size_; ++i) {
      try {
        push_back(other[i]);
      } catch (...) {
        clear();
        throw;
      }
    }
  }

  Deque(const size_t number): Deque() {
    reserve(greaterDivisibleByN(increasingConstant));

    for (size_t i = 0; i < number; ++i) {
      try {
        push_back(T());
      } catch (...) {
        clear();
        throw;
      }
    }
  }

  Deque(const size_t number, const T& elem): Deque() {
    reserve(greaterDivisibleByN(increasingConstant));
    for (size_t i = 0; i < number; ++i) {
      try {
        push_back(elem);
      } catch (...) {
        clear();
        throw;
      }
    }
  }

  ~Deque() noexcept {
    clear();
  }

  Deque& operator=(const Deque& other) {
    try {
      Deque copy(other);
      swap(copy);
    } catch (...) {
      throw;
    }

    return *this;
  }

  void push_back(const T& elem) {
    if (backCapacity() >= sizeOfBucket * capacity) {
      reserve(increasingConstant * capacity);
    }
    try {
      new(pointerToElem(backCapacity())) T(elem);
    } catch (...) {
      throw;
    }
    ++size_;
  }

  void push_front(const T& elem) {
    if (firstElem_ == 0) {
      reserve(capacity * increasingConstant);
    }

    try {
      new(pointerToElem(firstElem_ - 1)) T(elem);
    } catch (...) {
      throw;
    }
    --firstElem_;
    ++size_;
  }

  void pop_back() {
    (--end())->~T();
    --size_;
  }
  void pop_front() {
    begin()->~T();
    ++firstElem_;
    --size_;
  }
  T& operator[](const size_t index) {
    return *(begin() + index);
  }

  const T& operator[](const size_t index) const {
    return *const_iterator(begin() + index);
  }

  T& at(const size_t index) {
    if (index >= size_) {
      throw std::out_of_range("index bigger than size");
    } else {
      return (*this)[index];
    }
  }

  const T& at(const size_t index) const {
    if (index >= size_) {
      throw std::out_of_range("index bigger than size");
    } else {
      return (*this)[index];
    }
  }

  size_t size() const noexcept {
    return size_;
  }

  template<bool is_const>
  class CommonIterator;
  using iterator = CommonIterator<false>;
  using const_iterator = CommonIterator<true>;

  //in deque must be ranndom accses iterator
  template<bool is_const>
  class CommonIterator {
   public:
    using iterator_category = std::random_access_iterator_tag;
    using value_type = T;
    using difference_type = size_t;
    using pointer = typename std::conditional<is_const, const T*, T*>::type;
    using reference = typename std::conditional<is_const, const T&, T&>::type;

    //copy constuctor && constuctor from pointer to elem
    CommonIterator(T* elem, T** bucket): elem_(elem), bucket_(bucket) {}
    CommonIterator(const CommonIterator& elem) = default;
    ~CommonIterator() = default;

    reference operator*() { return *elem_; }
    pointer operator->() { return elem_; }

    bool operator==(const CommonIterator& other) const { return elem_ == other.elem_; }
    bool operator!=(const CommonIterator& other) const { return elem_ != other.elem_; }

    bool operator<(const CommonIterator& other) const {
      if (bucket_ == other.bucket_) {
        return elem_ < other.elem_;
      }
      return bucket_ < other.bucket_;
    }
    bool operator>(const CommonIterator& other) const { return other < *this; }
    bool operator<=(const CommonIterator& other) const { return (*this < other) || (*this == other); }
    bool operator>=(const CommonIterator& other) const { return other <= *this; }

    operator const_iterator() const { return const_iterator(elem_, bucket_); }

    CommonIterator& operator+=(const int num) {
      int index = elem_ - bucket_[0] + num;
      if (index >= 0) {
        bucket_ += (index) / sizeOfBucket;
        elem_ = *bucket_ + (index % sizeOfBucket);
      } else {
        bucket_ -= (-index + sizeOfBucket - 1) / sizeOfBucket;
        elem_ = *bucket_ + (sizeOfBucket - (-index) % sizeOfBucket);
      }
      return *this;
    }
    CommonIterator& operator-=(const int num) {
      *this += (-num);
      return *this;
    }

    CommonIterator operator+(const int num) {
      CommonIterator copy = *this;
      copy += num;
      return copy;
    }

    CommonIterator operator-(const int num) {
      CommonIterator copy = *this;
      copy -= num;
      return copy;
    }

    CommonIterator& operator++() {
      *this += 1;
      return *this;
    }

    CommonIterator operator++(int) {
      CommonIterator copy = *this;
      ++(*this);
      return copy;
    }

    CommonIterator& operator--() {
      *this -= 1;
      return *this;
    }

    CommonIterator operator--(int) {
      CommonIterator copy = *this;
      --(*this);
      return copy;
    }

    int operator-(const CommonIterator& other) const {
      return sizeOfBucket * distBetweenBuckets(other) + distBetweenElems(other);
    }

   private:
    T* elem_;
    T** bucket_;

    int distBetweenBuckets(const CommonIterator& other) const {
      return bucket_ - other.bucket_;
    }

    int distBetweenElems(const CommonIterator& other) const {
      return elem_ - bucket_[0] - (other.elem_ - other.bucket_[0]);
    }
  };

  iterator begin() {
    return iterator(pointerToElem(firstElem_), pointerToBucket(firstElem_));
  }
  iterator end() {
    return begin() + size_;
  }
  const_iterator end() const {
    return cend();
  }
  const_iterator begin() const {
    return cbegin();
  }
  const_iterator cbegin() const {
    return const_iterator(iterator(pointerToElem(firstElem_),
                                   pointerToBucket(firstElem_)));
  }
  const_iterator cend() const {
    return const_iterator(begin() + size_);
  }

  revers_iterator<iterator> rbegin() {
    return revers_iterator<iterator>(--begin());
  }
  revers_iterator<iterator> rend() {
    return revers_iterator<iterator>(--end());
  }

  revers_iterator<const_iterator> crbegin() const {
    return revers_iterator<const_iterator>(--begin());
  }
  revers_iterator<const_iterator> crend() const {
    return revers_iterator<const_iterator>(--end());
  }

  void insert(iterator iter, const T& elem) {
    push_back(elem);
    for (iterator it = --end(); it > iter; --it) {
      std::swap(*it, *(it - 1));
    }
  }
  void erase(iterator iter) {
    for (iterator it = iter; it < end() - 1; ++it) {
      *it = *(it + 1);
    }
    pop_back();
  }

 private:
  T** array_;
  size_t firstElem_;
  size_t size_;
  size_t capacity;
  static const size_t sizeOfBucket = 32;
  static const size_t increasingConstant = 3;

  size_t greaterDivisibleByN(size_t n, const Deque& other) const {
    return (numOfBuckets(other) / n + 1) * n;
  }
  size_t numOfBuckets(const Deque& other) const {
    return (other.size_ + sizeOfBucket - 1) / sizeOfBucket;
  }

  auto pointerToElem(size_t indexOfElem) const {
    return *pointerToBucket(indexOfElem) + placeInBucket(indexOfElem);
  }

  void clear() noexcept {
    for (iterator iter = begin(); iter < end(); ++iter) {
      iter->~T();
    }
    for (size_t i = 0; i < capacity; ++i) {
      delete[] reinterpret_cast<uint8_t*>(array_[i]);
    }
    delete[] array_;
  }

  void reserve(size_t cap) { //cap = capacity * 3
    T** newarray;

    size_t leftBorder = 0;
    size_t rightBorder = (cap + capacity) / 2;
    try {
      newarray = new T* [cap];
      if (cap == 1) {
        *newarray = allocateRawMemory();
      } else {
        for (leftBorder = 0; leftBorder < (cap - capacity) / 2; ++leftBorder) {
          newarray[leftBorder] = allocateRawMemory();
        }
        for (rightBorder = (cap + capacity) / 2; rightBorder < cap; ++rightBorder) {
          newarray[rightBorder] = allocateRawMemory();
        }
        for (size_t i = 0; i < capacity; ++i) {
          new(newarray + (cap - capacity) / 2 + i) T*(array_[i]);
        }
      }
    } catch (...) {
      if (cap == 1) {
        deallocateMemory(0);
      } else {
        for (size_t ind = 0; ind < leftBorder; ++ind) {
          deallocateMemory(ind);
        }
        for (size_t ind = (cap + capacity) / 2; ind < rightBorder; ++ind) {
          deallocateMemory(ind);
        }
      }
      delete[] newarray;
      throw;
    }

    delete[] array_;
    firstElem_ += (cap / increasingConstant ? cap / increasingConstant : 1) * sizeOfBucket;
    capacity = cap;
    array_ = newarray;

  }

  T* allocateRawMemory() {
    return reinterpret_cast<T*>(new uint8_t[sizeOfBucket * sizeof(T)]);
  }
  void deallocateMemory(size_t index) {
    delete[] reinterpret_cast<uint8_t*>(array_[index]);
  }
  size_t backCapacity() {
    return firstElem_ + size_;
  }
  T** pointerToBucket(size_t index) const {
    return array_ + (index / sizeOfBucket);
  }
  size_t placeInBucket(size_t index) const {
    return index % sizeOfBucket;
  }
  void swap(Deque<T>& other) {
    std::swap(array_, other.array_);
    std::swap(firstElem_, other.firstElem_);
    std::swap(size_, other.size_);
    std::swap(capacity, other.capacity);
  }
};

