
#include "Image.h"

DynamicPixel Image::load(const unsigned u, const unsigned v) const {
    ImageView view(*this, myPixels);
    return view.load(u, v);
}

void Image::store(const unsigned u, const unsigned v, const DynamicPixel &pixel) {
    if (u >= width() || v >= height()) return;

    const size_t stride = pixel_stride(pixelType());
    const size_t pixelStride = stride * static_cast<size_t>(channels());
    const size_t offset = (v * width() + u) * pixelStride;

    auto pixels = static_cast<char*>(myPixels);

    const int ch = static_cast<int>(channels());

    memcpy(pixels + offset + 0 * stride, pixel.r, stride);

    if (ch >= 2)
        memcpy(pixels + offset + 1 * stride, pixel.g, stride);
    if (ch >= 3)
        memcpy(pixels + offset + 2 * stride, pixel.b, stride);
    if (ch == 4)
        memcpy(pixels + offset + 3 * stride, pixel.a, stride);
}

DynamicPixel ImageView::load(const unsigned u, const unsigned v) const {
    if (u >= width() || v >= height()) return {};

    const size_t stride = pixel_stride(pixelType());
    const size_t pixelStride = stride * static_cast<size_t>(channels());
    const size_t offset = (v * width() + u) * pixelStride;

    auto pixels = static_cast<const char*>(myPixels);
    DynamicPixel result{};

    const int ch = static_cast<int>(channels());

    memcpy(&result.r, pixels + offset + 0 * stride, stride);

    if (ch >= 2)
        memcpy(&result.g, pixels + offset + 1 * stride, stride);
    if (ch >= 3)
        memcpy(&result.b, pixels + offset + 2 * stride, stride);
    if (ch == 4)
        memcpy(&result.a, pixels + offset + 3 * stride, stride);

    return result;
}
