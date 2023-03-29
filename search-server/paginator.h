#pragma once

#include <tgmath.h>
#include <iostream>
#include <vector>


template <typename Iterator>
class IteratorRange {

public:
    IteratorRange(Iterator begin, Iterator end);
    
    const Iterator begin() const; 
    
    const Iterator end() const;
    
    Iterator begin();
    
    Iterator end();
    
    size_t size() const;
    
private:
    Iterator begin_;
    Iterator end_;
    size_t size_;
};



    template <typename Iterator>
    IteratorRange<Iterator>::IteratorRange(Iterator begin, Iterator end) :
    begin_(begin),
    end_(end),
    size_(distance(begin_, end_))
    {
    }
    
    template <typename Iterator>
    const Iterator IteratorRange<Iterator>::begin() const {
        return begin_;
    }
    
    template <typename Iterator>
    const Iterator IteratorRange<Iterator>::end() const {
        return end_;
    }
    
    template <typename Iterator>
    Iterator IteratorRange<Iterator>::begin() {
        return begin_;
    }
    
    template <typename Iterator>
    Iterator IteratorRange<Iterator>::end() {
        return end_;
    }
    
    template <typename Iterator>
    size_t IteratorRange<Iterator>::size() const {
        return size_;
    }

 
template <typename Iterator>
class Paginator {
public:
    Paginator(Iterator begin, Iterator end, int size); 
    
    auto size() const;
    
    auto begin() const;
    
    auto end() const;
    
    auto begin(); 
    
    auto end();
    
private:
    std::vector<IteratorRange<Iterator>> pages_;
}; 
 



template <typename Iterator>
    Paginator<Iterator>::Paginator(Iterator begin, Iterator end, int size) {
       auto page_begin = begin;
       auto page_end = begin;
       int pages_amount = ceil(double(distance(begin, end)) / size);
       for(int i = 0; i < pages_amount; ++i ) {
            if(i == pages_amount - 1) {
                page_end = end;
            }
            else {
                advance(page_end, size);
            }
            advance(page_begin, size * i);
            pages_.push_back(IteratorRange<Iterator>(page_begin, page_end));
        }
    }
    
template <typename Iterator>
    auto Paginator<Iterator>::size() const {
        return pages_.size();
    }
    
    
template <typename Iterator>
    auto Paginator<Iterator>::begin() const {
        return pages_.begin();
    }
    
template <typename Iterator>
    auto Paginator<Iterator>::end() const {
        return pages_.end();
    }
    
template <typename Iterator>
    auto Paginator<Iterator>::begin() {
        return pages_.begin();
    }
    

template <typename Iterator>
    auto Paginator<Iterator>::end() {
        return pages_.end();
    }
    


template <typename Container>
auto Paginate(const Container& container, size_t page_size);
 
template <typename Container>
auto Paginate(const Container& container, size_t page_size) {
    return Paginator(begin(container), end(container), page_size);
}