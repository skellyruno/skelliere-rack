// Minimal stand-in for the one JUCE class the DSP touches, so the DSP can be tested without JUCE.
#pragma once
#include <vector>
#include <cstring>
namespace juce {
template <typename T> class AudioBuffer {
public:
    AudioBuffer(int ch, int n) : owned_((size_t) ch, std::vector<T>((size_t) n, T(0))), n_(n) { for (auto& v : owned_) ptrs_.push_back(v.data()); }
    AudioBuffer(T* const* d, int ch, int start, int n) : n_(n) { for (int c = 0; c < ch; ++c) ptrs_.push_back(d[c] + start); }
    int getNumChannels() const { return (int) ptrs_.size(); }
    int getNumSamples() const { return n_; }
    T* getWritePointer(int c) { return ptrs_[(size_t) c]; }
    T* const* getArrayOfWritePointers() { return ptrs_.data(); }
private:
    std::vector<std::vector<T>> owned_; std::vector<T*> ptrs_; int n_;
};
}
