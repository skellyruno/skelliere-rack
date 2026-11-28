#pragma once
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>
namespace skl::native_delay {
// Fixed-capacity, page-tagged zero history. Logical clearing touches only a
// generation counter, and pages are zeroed before the first access in a new
// generation. Reserve is non-RT. First-touch OS page faults are still possible
// for reserved but previously unwritten memory, as in the original vector.
class EpochLine {
    static constexpr std::size_t page=128;
public:
    void reserve(std::size_t n){
        if(n<=capacity)return;
        std::unique_ptr<float[]> next(new float[n]);
        std::vector<std::uint64_t> nextTags((n+page-1)/page,0);
        for(std::size_t p=0;p<tags.size();++p)if(tags[p]==generation){
            const auto start=p*page,count=std::min(page,capacity-start);
            std::copy_n(data.get()+start,count,next.get()+start);
            std::fill(next.get()+start+count,next.get()+std::min(n,start+page),0.f);
            nextTags[p]=generation;
        }
        data=std::move(next);tags=std::move(nextTags);capacity=n;
    }
    void clear() noexcept {length=0;invalidate();}
    bool empty() const noexcept {return length==0;}
    void assign(std::size_t n,float value){
        reserve(n);length=n;invalidate();
        if(value!=0.f){for(std::size_t i=0;i<n;++i)(*this)[i]=value;}
    }
    float& operator[](std::size_t i) noexcept {
        assert(i<length);const auto p=i/page;
        if(tags[p]!=generation){const auto start=p*page;std::fill_n(data.get()+start,std::min(page,capacity-start),0.f);tags[p]=generation;}
        return data[i];
    }
    float operator[](std::size_t i) const noexcept {assert(i<length);return tags[i/page]==generation?data[i]:0.f;}
    // The upstream live-timing initialization uses swap on its control thread.
    void swap(std::vector<float>& incoming){
        std::vector<float> old(length);const auto& self=*this;
        for(std::size_t i=0;i<length;++i)old[i]=self[i];
        assign(incoming.size(),0.f);
        for(std::size_t i=0;i<length;++i)(*this)[i]=incoming[i];
        incoming.swap(old);
    }
private:
    void invalidate() noexcept {if(++generation==0){std::fill(tags.begin(),tags.end(),0);generation=1;}}
    std::unique_ptr<float[]> data;std::vector<std::uint64_t> tags;
    std::size_t length=0,capacity=0;std::uint64_t generation=1;
};
}
