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
    reserve(1);
  }
  Deque(const Deque& other): Deque() {
    reserve((((other.size_ + sizeOfBucket - 1) / sizeOfBucket) / 3 + 1) * 3);
    for (size_t i = 0; i < other.size_; ++i) {
      push_back(other[i]);
    }
  }
  Deque(const size_t number): Deque() {
    reserve((((number + sizeOfBucket - 1) / sizeOfBucket) / 3 + 1) * 3);
    for (size_t i = 0; i < number; ++i) {
      push_back(T());
    }
  }
  Deque(const size_t number, const T& elem): Deque() {
    reserve((((number + sizeOfBucket - 1) / sizeOfBucket) / 3 + 1) * 3);
    for (size_t i = 0; i < number; ++i) {
      push_back(elem);
    }
  }

  ~Deque() {
    for (iterator iter = begin(); iter < end(); ++iter) {
      iter->~T();
    }
    for (size_t i = 0; i < capacity; ++i) {
      delete[] reinterpret_cast<uint8_t*>(array_[i]);
    }
    delete[] array_;
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
    if (firstElem_ + size_ >= sizeOfBucket * capacity) {
      reserve(3 * capacity);
    }
    try {
      new(*pointerToBucket(firstElem_ + size_) + placeInBucket(firstElem_ + size_)) T(elem);
    } catch (...) {
      throw;
    }
    ++size_;
  }

  void push_front(const T& elem) {
    if (firstElem_ == 0) {
      reserve(capacity * 3);
    }

    --firstElem_;
    ++size_;
    try {
      new(*pointerToBucket(firstElem_) + placeInBucket(firstElem_)) T(elem);
    } catch (...) {
      --size_;
      ++firstElem_;
      throw;
    }
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
    };
  }

  const T& at(const size_t index) const {
    if (index >= size_) {
      throw std::out_of_range("index bigger than size");
    } else {
      return (*this)[index];
    }
  }

  size_t size() const {
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
        index *= -1;
        bucket_ -= (index + sizeOfBucket - 1) / sizeOfBucket;
        elem_ = *bucket_ + (sizeOfBucket - (index) % sizeOfBucket);
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

    int operator-(const CommonIterator& other) {
      return sizeOfBucket * (bucket_ - other.bucket_) + (elem_ - bucket_[0] - (other.elem_ - other.bucket_[0]));
    }

   private:
    T* elem_;
    T** bucket_;
  };

  iterator begin() {
    return iterator(*pointerToBucket(firstElem_) + placeInBucket(firstElem_), pointerToBucket(firstElem_));
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
    return const_iterator(iterator(*pointerToBucket(firstElem_) + placeInBucket(firstElem_),
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

  void reserve(size_t cap) { //cap = capacity * 3
    T** newarray = new T* [cap];

    if (cap == 1) {
      *newarray = reinterpret_cast<T*> (new uint8_t[sizeOfBucket * sizeof(T)]);
    } else {
      for (size_t i = 0; i < (cap - capacity) / 2; ++i) {
        newarray[i] = reinterpret_cast<T*> (new uint8_t[sizeOfBucket * sizeof(T)]);
      }
      for (size_t i = (cap + capacity) / 2; i < cap; ++i) {
        newarray[i] = reinterpret_cast<T*> (new uint8_t[sizeOfBucket * sizeof(T)]);
      }
    }
//    std::cout << newarray[0] + 1 << std::endl;
    for (size_t i = 0; i < capacity; ++i) {
      try {
        new(newarray + (cap - capacity) / 2 + i) T*(array_[i]);
      } catch (...) {

        if (cap == 1) {
          delete[] reinterpret_cast<uint8_t*>(*array_);
        } else {
          for (size_t ind = 0; ind < cap / 3; ++ind) {
            delete[] reinterpret_cast<uint8_t*>(array_[ind]);
          }
          for (size_t ind = 2 * cap / 3; ind < cap; ++ind) {
            delete[] reinterpret_cast<uint8_t*>(array_[ind]);
          }
        }
        delete[] newarray;
        throw;
      }
    }
    delete[] array_;
    firstElem_ += (cap / 3 ? cap / 3 : 1) * sizeOfBucket;
    capacity = cap;
    array_ = newarray;

  }

  T** pointerToBucket(size_t index) const {
    return array_ + (index / sizeOfBucket);
  }
  size_t placeInBucket(size_t index) const {
    return (index % sizeOfBucket);
  }
  void swap(Deque<T>& other) {
    std::swap(array_, other.array_);
    std::swap(firstElem_, other.firstElem_);
    std::swap(size_, other.size_);
    std::swap(capacity, other.capacity);
  }
};

