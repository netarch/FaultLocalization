#ifndef __FAULT_LOCALIZE_DEFS__
#define __FAULT_LOCALIZE_DEFS__

#include <vector>
#include <set>
#include <unordered_set>
#include <atomic>
#include <chrono>

using namespace std;

#define MAX_PATH_LENGTH 4

template <typename T>
struct SmallVector{
    T arr[MAX_PATH_LENGTH];
    char arr_size, ind;
    SmallVector(vector<T> &v): arr_size(v.size()), ind(0){
        //arr = new T[arr_size];
        for (T& elt: v){
            arr[ind++] = elt;
        }
    }
    SmallVector(int _size): arr_size(_size), ind(0){
        //arr = new T[arr_size];
    }
    void push_back(T elt){
        arr[ind++] = elt;
    }
    bool operator==(const SmallVector<T>& rhs) {
        if (arr_size != rhs.arr_size) return false;
        bool equal = true;
        for (int i=0; i<arr_size; i++){
            equal = equal & (arr[i] == rhs.arr[i]);
        }
        return equal;
    }
    bool operator==(const vector<T>& rhs) {
        if (arr_size != rhs.size()) return false;
        bool equal = true;
        for (int i=0; i<arr_size; i++){
            equal = equal & (arr[i] == rhs[i]);
        }
        return equal;
    }
    char size() {
        return arr_size;
    }
    class iterator{
        public:
            typedef iterator self_type;
            typedef T value_type;
            typedef T& reference;
            typedef T* pointer;
            typedef std::forward_iterator_tag iterator_category;
            typedef int difference_type;
            iterator(pointer ptr) : ptr_(ptr) { }
            self_type operator++() { self_type i = *this; ptr_++; return i; }
            self_type operator++(int junk) { ptr_++; return *this; }
            reference operator*() { return *ptr_; }
            pointer operator->() { return ptr_; }
            bool operator==(const self_type& rhs) { return ptr_ == rhs.ptr_; }
            bool operator!=(const self_type& rhs) { return ptr_ != rhs.ptr_; }
        private:
            pointer ptr_;
    };

    class const_iterator{
        public:
            typedef const_iterator self_type;
            typedef T value_type;
            typedef T& reference;
            typedef T* pointer;
            typedef int difference_type;
            typedef std::forward_iterator_tag iterator_category;
            const_iterator(pointer ptr) : ptr_(ptr) { }
            self_type operator++() { self_type i = *this; ptr_++; return i; }
            self_type operator++(int junk) { ptr_++; return *this; }
            const reference operator*() { return *ptr_; }
            const pointer operator->() { return ptr_; }
            bool operator==(const self_type& rhs) { return ptr_ == rhs.ptr_; }
            bool operator!=(const self_type& rhs) { return ptr_ != rhs.ptr_; }
        private:
            pointer ptr_;
    };
    iterator begin() { return iterator(arr); }
    iterator end() { return iterator(arr + arr_size); }
    const_iterator begin() const { return const_iterator(arr); }
    const_iterator end() const { return const_iterator(arr + arr_size); }
};


typedef pair<int, int> PII;
typedef pair<double, double> PDD;
typedef SmallVector<int> Path;
typedef PII Link;
//!TODO: run speed tests b/w set and unordered_set
typedef set<int> Hypothesis; //contains link_ids

namespace std {
    template <>
    struct hash<PII>{
        std::size_t operator()(const PII& pii) const {
            return hash<int>()(pii.first) ^ hash<int>()(pii.second);
        }
    };
}

struct HypothesisPointerCompare{
    explicit HypothesisPointerCompare(Hypothesis *h_):h(h_) {}
    inline bool operator()(const Hypothesis* h2) const {
        return *h == *h2;
    }
private:
    Hypothesis *h;
};


template< class T1, class T2 >
constexpr bool operator==( const pair<T1,T2>& lhs, const pair<T1,T2>& rhs );

// C++ template to print pair<> 
// class by using template 
template <typename T, typename S> 
ostream& operator<<(ostream& os, const pair<T, S>& v) { 
    os << "("; 
    os << v.first << ", " 
        << v.second << ")"; 
    return os; 
}

// C++ template to print pair<> 
// class by using template 
template <typename T, typename S> 
constexpr pair<T, S> operator+(const pair<T, S>& v1, const pair<T, S>& v2) { 
    return pair<T, S>(v1.first + v2.first, v1.second + v2.second);
}

// C++ template to print vector container elements 
template <typename T> 
ostream& operator<<(ostream& os, const vector<T>& v) { 
    os << "["; 
    for (int i = 0; i < v.size(); ++i) { 
        os << v[i]; 
        if (i != v.size() - 1) 
            os << ", "; 
    }
    os << "]"; 
    return os; 
} 

// C++ template to print set container elements 
template <typename T> 
ostream& operator<<(ostream& os, const set<T>& v) { 
    os << "["; 
    for (auto it : v) { 
        os << it; 
        if (it != *v.rbegin()) 
            os << ", "; 
    } 
    os << "]"; 
    return os; 
} 

// C++ template to print unordered_set container elements 
template <typename T> 
ostream& operator<<(ostream& os, const unordered_set<T>& v) { 
    os << "["; 
    bool first = true; 
    for (auto it : v) { 
        if (! first) os <<", ";
        os << it; 
        first = false;
    } 
    os << "]"; 
    return os; 
} 

const bool PATH_KNOWN=false;
const bool CONSIDER_REVERSE_PATH=false;
const bool VERBOSE=true;

class SpinLock {
    std::atomic_flag locked = ATOMIC_FLAG_INIT ;
    public:
    void lock() {
        while (locked.test_and_set(std::memory_order_acquire));
    }
    void unlock() {
        locked.clear(std::memory_order_release);
    }
};

static inline double GetTimeSinceMilliSeconds(chrono::time_point<chrono::system_clock> start_time){
    return chrono::duration_cast<chrono::milliseconds>(
           chrono::high_resolution_clock::now() - start_time).count()*1.0e-3;
}

#endif
