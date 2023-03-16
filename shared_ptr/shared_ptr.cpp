#include <iostream>
#include <vector>
#include <memory>

template<typename T>
class WeakPtr;

struct BaseControlBlock {
    size_t shared_count;
    size_t weaked_count;

    BaseControlBlock(const size_t& sc, const size_t& wc) : shared_count(sc), weaked_count(wc) {}

    virtual ~BaseControlBlock() = default;

    virtual void* getPtr() = 0;

    virtual void destroy_object() = 0;

    virtual void deallocate() = 0;
};

template<typename T, typename Deleter, typename Alloc>
struct ControlBlockDirect : public BaseControlBlock {
    T* ptr;
    Deleter deleter;
    Alloc alloc;

    void* getPtr() override {
        return reinterpret_cast<void*>(ptr);
    }

    void destroy_object() override {
        deleter(ptr);
        ptr = nullptr;
    }

    void deallocate() override {
        typename std::allocator_traits<Alloc>::template rebind_alloc<ControlBlockDirect<T, Deleter, Alloc>>(
                alloc).deallocate(this, 1);

    }

    ControlBlockDirect(const size_t& sc, const size_t& wc, T* ptr, const Deleter& deleter, const Alloc& allocator)
            : BaseControlBlock(sc, wc), ptr(ptr), deleter(deleter), alloc(allocator) {}

};

template<typename T, typename Alloc>
struct ControlBlockViaMakeShared : public BaseControlBlock {
    T object;
    Alloc alloc;

    void* getPtr() override {
        return reinterpret_cast<void*>(&object);
    }

    void destroy_object() override {
        std::allocator_traits<Alloc>::destroy(alloc, &object);
    }

    void deallocate() override {
        typename std::allocator_traits<Alloc>::template rebind_alloc<ControlBlockViaMakeShared<T, Alloc>>(
                alloc).deallocate(this, 1);

    }

    template<typename... Args>
    ControlBlockViaMakeShared(const size_t& sc, const size_t& wc, const Alloc& alloc, Args&& ... args)
            : BaseControlBlock(
            sc, wc), object(std::forward<Args>(args)...), alloc(alloc) {}

};

template<typename T>
class SharedPtr {
    template<typename TT>
    using is_suit = std::enable_if_t<std::is_same_v<T, TT> || std::is_base_of_v<T, TT>>;


public:
    SharedPtr() = default;

    template<typename TT = T, typename Deleter = std::default_delete<TT>, typename Alloc = std::allocator<TT>, typename = is_suit<TT>> //TODO: why it is working
    SharedPtr(TT* ptrObject, const Deleter& deleter = Deleter(), const Alloc& alloc = Alloc()): objectPtr(
            reinterpret_cast<T*>(ptrObject)) {
        using AllocatorControlBlock = typename std::allocator_traits<Alloc>::template rebind_alloc<ControlBlockDirect<TT, Deleter, Alloc>>;
        using AllocatorControlBlockTraits = typename std::allocator_traits<AllocatorControlBlock>;
        AllocatorControlBlock allocCB = alloc;
        auto pointerCB = AllocatorControlBlockTraits::allocate(allocCB, 1);
        ::new ((void *)pointerCB) ControlBlockDirect<TT,Deleter,Alloc>( 1, 0, reinterpret_cast<T*>(ptrObject), deleter,
                                               alloc);
        cb = reinterpret_cast<BaseControlBlock*>(pointerCB);
    }

    SharedPtr(const SharedPtr<T>& other) : cb(other.cb), objectPtr(other.objectPtr) {
        if (cb != nullptr) {
            ++(cb->shared_count);
        }
    }

    template<typename TT = T, typename = is_suit<TT>>
    SharedPtr(const SharedPtr<TT>& other): cb(other.cb), objectPtr(other.objectPtr) { //TODO: but this doesn't work
        if (cb != nullptr) {
            ++(cb->shared_count);
        }
    }

    SharedPtr(SharedPtr<T>&& other) : cb(other.cb), objectPtr(other.objectPtr) {
        other.objectPtr = nullptr;
        other.cb = nullptr;
    }

    template<typename TT, typename = is_suit<TT>>
    SharedPtr(SharedPtr<TT>&& other): cb(other.cb), objectPtr(other.objectPtr) {
        other.objectPtr = nullptr;
        other.cb = nullptr;
    }

    SharedPtr& operator=(SharedPtr<T>&& other) {
        auto tmp = SharedPtr<T>(std::move(other));
        swap(tmp);
        return *this;
    }

    template<typename TT, typename = is_suit<TT>>
    SharedPtr& operator=(SharedPtr<TT>&& other) {
        auto tmp = SharedPtr<T>(std::move(other));
        swap(tmp);
        return *this;
    }


    SharedPtr& operator=(const SharedPtr<T>& other) {
        auto tmp = SharedPtr<T>(other);
        swap(tmp);
        return *this;
    }

    template<typename TT, typename = is_suit<TT>>
    SharedPtr& operator=(const SharedPtr<TT>& other) {
        auto tmp = SharedPtr<T>(other);
        swap(tmp);
        return *this;
    }

    ~SharedPtr() {
        if (!cb) return;
        --cb->shared_count;
        if (cb->shared_count == 0) {
            cb->destroy_object();
            if (cb->weaked_count == 0)
                cb->deallocate();
        }
    }

    void swap(SharedPtr& other) {
        std::swap(cb, other.cb);
        std::swap(objectPtr, other.objectPtr);
    }

private:

public:
    BaseControlBlock* cb = nullptr;
    T* objectPtr = nullptr;

    explicit SharedPtr(BaseControlBlock* cb) : cb(cb), objectPtr(reinterpret_cast<T*>(cb->getPtr())) {
        ++cb->shared_count;
    };

    friend WeakPtr<T>;

    template<typename TT, typename Alloc, typename... Args>
    friend SharedPtr<TT> allocateShared(const Alloc& alloc, Args&& ... args);

    template<typename TT, typename... Args>
    friend SharedPtr<TT> makeShared(Args&& ... args);


public:
    T* get() const {
        return objectPtr;
    }


    T& operator*() {
        return *objectPtr;
    }

    const T& operator*() const {
        return *objectPtr;
    }

    T* operator->() noexcept {
        return objectPtr;
    }

    const T* operator->() const noexcept {
        return objectPtr;
    }

    size_t use_count() const {
        return cb ? cb->shared_count : 0;
    }

    void reset() noexcept {
        *this = SharedPtr<T>();
    };

    template<typename TT>
    void reset(TT* otherPtr) {
        *this = otherPtr;
    }


};

template<typename T, typename Alloc, typename... Args>
SharedPtr<T> allocateShared(const Alloc& alloc, Args&& ... args) {
    using AllocatorControlBlock = typename std::allocator_traits<Alloc>::template rebind_alloc<ControlBlockViaMakeShared<T, Alloc>>;
    using AllocatorControlBlockTraits = typename std::allocator_traits<AllocatorControlBlock>;
    AllocatorControlBlock allocCB = alloc;
    auto pointerCB = AllocatorControlBlockTraits::allocate(allocCB, 1);
    AllocatorControlBlockTraits::construct(allocCB, pointerCB, 0, 0, alloc, std::forward<Args>(args)...);
    return SharedPtr<T>(pointerCB);
}


template<typename T, typename... Args>
SharedPtr<T> makeShared(Args&& ... args) {
    return allocateShared<T>(std::allocator<T>(), std::forward<Args>(args)...);
}

template<typename T>
class WeakPtr {

public:
    BaseControlBlock* cb = nullptr;
    T* objectPtr = nullptr;


    template<typename TT>
    using is_suit = std::enable_if_t<std::is_same_v<T, TT> || std::is_base_of_v<T, TT>>;
public:
    WeakPtr() = default;

    WeakPtr(const SharedPtr<T>& shpr) : cb(shpr.cb), objectPtr(shpr.objectPtr) {
        if (cb) ++cb->weaked_count;
    }

    template<typename T_ = T, typename = is_suit<T_>>
    WeakPtr(const SharedPtr<T_>& shpr): cb(shpr.cb), objectPtr(shpr.objectPtr) {
        if (cb) ++cb->weaked_count;
    }


    WeakPtr(const WeakPtr<T>& source) : cb(source.cb), objectPtr(source.objectPtr) {
        if (cb) ++cb->weaked_count;
    }

    template<typename T_ = T, typename =is_suit<T_>>
    WeakPtr(const WeakPtr<T_>& source): cb(source.cb), objectPtr(source.objectPtr) {
        if (cb) ++cb->weaked_count;
    }


    WeakPtr(WeakPtr<T>&& source) : cb(source.cb), objectPtr(source.objectPtr) {
        source.cb = nullptr;
        source.objectPtr = nullptr;
    }

    template<typename T_ = T, typename =is_suit<T_>>
    WeakPtr(WeakPtr<T_>&& source): cb(source.cb), objectPtr(source.objectPtr) {
        source.cb = nullptr;
        source.ptr = nullptr;
    }


    void swap(WeakPtr<T>& right) {
        std::swap(cb, right.cb);
        std::swap(objectPtr, right.objectPtr);
    }

    template<typename T_>
    WeakPtr& operator=(const SharedPtr<T_>& shpr) {
        auto tmp = WeakPtr<T>(shpr);
        swap(tmp);
        return *this;
    }

    template<typename T_>
    WeakPtr& operator=(const WeakPtr<T_>& source) {
        auto tmp = WeakPtr<T>(source);
        swap(tmp);
        return *this;
    }

    template<typename T_>
    WeakPtr& operator=(WeakPtr<T_>&& source) {
        auto tmp = WeakPtr<T>(std::move(source));
        swap(tmp);
        return *this;
    }

    auto use_count() const {
        return cb ? cb->shared_count : 0;
    }

    bool expired() const {
        return cb && cb->shared_count == 0;
    }

    auto lock() const {
        return !expired() ? SharedPtr<T>(cb) : SharedPtr<T>();
    }

    ~WeakPtr() {
        if (!cb) return;
        --cb->weaked_count;
        if (cb->weaked_count == 0 && cb->shared_count == 0) {
            cb->deallocate();
        }
    }


};
