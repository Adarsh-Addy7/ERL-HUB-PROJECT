// PriorityDispatchQueue.h - Unit IV: generic template class backed by std::vector + heap algorithms
#ifndef RELIEFOPS_PRIORITY_DISPATCH_QUEUE_H
#define RELIEFOPS_PRIORITY_DISPATCH_QUEUE_H

#include <algorithm>
#include <cstddef>
#include <functional>
#include <stdexcept>
#include <vector>

namespace ReliefOps {

// The element for which Compare(a, b) is "largest" is dispatched first.
template <typename T, typename Compare = std::less<T> >
class PriorityDispatchQueue {
public:
    explicit PriorityDispatchQueue(Compare cmp = Compare()) : cmp_(cmp) {}

    void push(const T& item) {
        heap_.push_back(item);
        std::push_heap(heap_.begin(), heap_.end(), cmp_);
    }
    T pop() {
        if (heap_.empty()) throw std::out_of_range("PriorityDispatchQueue: pop from empty queue");
        std::pop_heap(heap_.begin(), heap_.end(), cmp_);
        T top = heap_.back();
        heap_.pop_back();
        return top;
    }
    const T& top() const {
        if (heap_.empty()) throw std::out_of_range("PriorityDispatchQueue: top of empty queue");
        return heap_.front();
    }
    bool empty() const { return heap_.empty(); }
    std::size_t size() const { return heap_.size(); }

private:
    std::vector<T> heap_;
    Compare cmp_;
};

} // namespace ReliefOps
#endif
