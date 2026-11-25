#pragma once
#include <algorithm>
#include <cmath>

namespace skl::ui {
inline int spriteFrame(double normalized, int count) noexcept {
    if (count <= 1) return 0;
    if (!std::isfinite(normalized)) normalized = 0.0;
    return static_cast<int>(std::lround(std::clamp(normalized, 0.0, 1.0) * (count - 1)));
}
// Source PNG is a linear stream of contiguous frames. Its apparent width is
// frame height * count, and apparent height is frame width.
struct PackedFrameLayout {
    int frameWidth=0, frameHeight=0, count=0;
    bool valid() const noexcept { return frameWidth > 0 && frameHeight > 0 && count > 0; }
};
inline PackedFrameLayout packedFrameLayout(int width, int height, int frames) noexcept {
    if (width <= 0 || height <= 0 || frames <= 0 || width % frames != 0) return {};
    return {height, width / frames, frames};
}
}
