#include "media/ImageLoader.hpp"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <wincodec.h>
#include <wrl/client.h>
#endif

#include <cstdint>
#include <sstream>
#include <utility>

namespace {
std::string hresultString(long result)
{
    std::ostringstream stream;
    stream << "HRESULT 0x" << std::hex << static_cast<unsigned long>(result);
    return stream.str();
}

LoadedImage failImage(std::string error)
{
    LoadedImage image;
    image.error = std::move(error);
    return image;
}
}

LoadedImage loadImageFileRgba(const std::filesystem::path& path)
{
#ifndef _WIN32
    (void)path;
    return failImage("image preview is only implemented on Windows");
#else
    static thread_local bool comReady = false;
    if (!comReady) {
        HRESULT comResult = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        if (FAILED(comResult) && comResult != RPC_E_CHANGED_MODE) {
            return failImage("COM initialization failed: " + hresultString(comResult));
        }
        comReady = true;
    }

    LoadedImage image;

    Microsoft::WRL::ComPtr<IWICImagingFactory> factory;
    HRESULT result = CoCreateInstance(
        CLSID_WICImagingFactory,
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&factory));
    if (FAILED(result)) {
        return failImage("WIC factory creation failed: " + hresultString(result));
    }

    Microsoft::WRL::ComPtr<IWICBitmapDecoder> decoder;
    result = factory->CreateDecoderFromFilename(
        path.wstring().c_str(),
        nullptr,
        GENERIC_READ,
        WICDecodeMetadataCacheOnDemand,
        &decoder);
    if (FAILED(result)) {
        return failImage("image decoder failed: " + hresultString(result));
    }

    Microsoft::WRL::ComPtr<IWICBitmapFrameDecode> frame;
    result = decoder->GetFrame(0, &frame);
    if (FAILED(result)) {
        return failImage("image frame decode failed: " + hresultString(result));
    }

    UINT width = 0;
    UINT height = 0;
    result = frame->GetSize(&width, &height);
    if (FAILED(result) || width == 0 || height == 0) {
        return failImage("image size is invalid");
    }

    constexpr UINT MaxDimension = 8192;
    constexpr std::uint64_t MaxPixels = 64ULL * 1024ULL * 1024ULL;
    const std::uint64_t pixelCount = static_cast<std::uint64_t>(width) * static_cast<std::uint64_t>(height);
    if (width > MaxDimension || height > MaxDimension || pixelCount > MaxPixels) {
        return failImage("image is too large for preview");
    }

    Microsoft::WRL::ComPtr<IWICFormatConverter> converter;
    result = factory->CreateFormatConverter(&converter);
    if (FAILED(result)) {
        return failImage("image converter creation failed: " + hresultString(result));
    }

    result = converter->Initialize(
        frame.Get(),
        GUID_WICPixelFormat32bppRGBA,
        WICBitmapDitherTypeNone,
        nullptr,
        0.0,
        WICBitmapPaletteTypeCustom);
    if (FAILED(result)) {
        return failImage("image conversion failed: " + hresultString(result));
    }

    const UINT stride = width * 4U;
    const UINT byteCount = stride * height;
    image.rgba.resize(byteCount);
    result = converter->CopyPixels(nullptr, stride, byteCount, image.rgba.data());
    if (FAILED(result)) {
        return failImage("image pixel copy failed: " + hresultString(result));
    }

    image.ok = true;
    image.width = static_cast<int>(width);
    image.height = static_cast<int>(height);
    return image;
#endif
}
