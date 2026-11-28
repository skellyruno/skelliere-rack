#pragma once
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <memory>
#include <vector>
namespace skl {
class SlotDelayArena {
public:
    using Storage = std::vector<float>;
    SlotDelayArena():storage(std::make_shared<Storage>()){}
    void beginModule() noexcept {cursor=0;}
    std::size_t reserve(std::size_t count) {
        const auto offset=cursor;cursor+=count;
        if(storage->size()<cursor)storage->resize(cursor,0.f);
        return offset;
    }
    bool extendLast(std::size_t offset,std::size_t previous,std::size_t count) {
        if(offset+previous!=cursor)return false;
        cursor=offset+count;if(storage->size()<cursor)storage->resize(cursor,0.f);return true;
    }
    std::shared_ptr<Storage> storage;
private:std::size_t cursor=0;
};
inline thread_local SlotDelayArena* preparingSlotDelayArena=nullptr;
class SlotDelayPreparation {
public:
    explicit SlotDelayPreparation(SlotDelayArena& arena) noexcept:previous(preparingSlotDelayArena){arena.beginModule();preparingSlotDelayArena=&arena;}
    ~SlotDelayPreparation(){preparingSlotDelayArena=previous;}
    SlotDelayPreparation(const SlotDelayPreparation&)=delete;
    SlotDelayPreparation& operator=(const SlotDelayPreparation&)=delete;
private:SlotDelayArena* previous;
};

// Vector-like storage for the pinned native history containers. While a slot
// is prepared, reserve a disjoint slice in its shared arena. The owning module
// must reset before use following a type change; no other module may read it.
// Outside slot preparation (standalone engines and advanced Delay), use the
// original private vector semantics. No native DSP arithmetic is changed.
class SlotHistoryVector {
public:
    void reserve(std::size_t n) {
        if(!arena && !local.capacity() && preparingSlotDelayArena) {
            arena=preparingSlotDelayArena->storage;offset=preparingSlotDelayArena->reserve(n);reserved=n;return;
        }
        if(!arena){local.reserve(n);return;}
        if(n<=reserved)return;
        if(auto* a=preparingSlotDelayArena;a&&a->storage==arena&&a->extendLast(offset,reserved,n)){reserved=n;return;}
        // Same fallback semantics as the original vector if a caller exceeds
        // its declared capacity. The rack's bounded controls never take it.
        std::vector<float> next;next.reserve(n);next.assign(arena->begin()+offset,arena->begin()+offset+length);local=std::move(next);arena.reset();offset=reserved=length=0;
    }
    void assign(std::size_t n,float value){reserve(n);if(!arena){local.assign(n,value);return;}length=n;std::fill_n(arena->data()+offset,n,value);}
    void clear() noexcept {if(arena)length=0;else local.clear();}
    void shrink_to_fit(){if(!arena)local.shrink_to_fit();}
    bool empty() const noexcept {return size()==0;}
    std::size_t size() const noexcept {return arena?length:local.size();}
    float& operator[](std::size_t i) noexcept {assert(i<size());return arena?(*arena)[offset+i]:local[i];}
    const float& operator[](std::size_t i)const noexcept {assert(i<size());return arena?(*arena)[offset+i]:local[i];}
    void swap(std::vector<float>& incoming){if(!arena){local.swap(incoming);return;}std::vector<float> previous(arena->begin()+offset,arena->begin()+offset+length);reserve(incoming.size());if(arena){length=incoming.size();std::copy(incoming.begin(),incoming.end(),arena->begin()+offset);}else local=std::move(incoming);incoming.swap(previous);}
private:
    std::shared_ptr<SlotDelayArena::Storage> arena;std::vector<float> local;
    std::size_t offset=0,reserved=0,length=0;
};
}
