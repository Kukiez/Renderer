#include "TextureResourceType.h"
#include <Image/ImageLoader.h>
#include <Renderer/Resource/Buffer/BufferComponentType.h>
#include "TextureFactory.h"
#include "TextureQuery.h"

Texture2D CreateTexture2DFromDescriptor(TextureDescriptor2D& tex2Desc) {
    return std::visit([&]<typename Img>(Img& img) {
        using FromImage = TextureDescriptor2D::FromImage;
        using FromFile = TextureDescriptor2D::FromFile;
        using FromEmpty = TextureDescriptor2D::FromEmpty;

        if constexpr (std::is_same_v<Img, FromImage>) {
            FromImage& fromImage = img;

            return Texture2D(fromImage.image, tex2Desc.params);
        } else if constexpr (std::is_same_v<Img, FromFile>) {
            FromFile& fromFile = img;
            const Image image = ImageLoader::load(fromFile.path, fromFile.options);
            return Texture2D(image, tex2Desc.params);
        } else {
            FromEmpty& fromEmpty = img;
            return Texture2D(fromEmpty.width, fromEmpty.height, tex2Desc.params);
        }
    }, tex2Desc.image);
}

Texture2DMS CreateTexture2DMSFromDescriptor(const TextureDescriptor2DMS& tex2Desc) {
    return Texture2DMS(tex2Desc.width, tex2Desc.height, tex2Desc.samples, tex2Desc.format);
}

void MapMaterialBufferReserve(BufferResourceStorage& storage, BufferKey& bufferKey, const size_t reserve) {
    if (bufferKey == NULL_BUFFER_KEY) {
        bufferKey = storage.createBuffer<size_t>(500, BufferUsageHint::IMMUTABLE);
    } else {
        if (bufferKey.sizeIn<size_t>() < reserve) {
            // TODO resize
            assert(false);
        }
    }
}

size_t TextureResourceType::makeTextureResident(const unsigned textureGpuId, const TextureKey key) {
    MapMaterialBufferReserve(renderer->getBufferStorage(), resource2DMaterialBuffer, key.id() + 1);

    const size_t handle = glGetTextureHandleARB(textureGpuId);
    glMakeTextureHandleResidentARB(handle);

    auto mapped = resource2DMaterialBuffer.mapRange<size_t>(key.index(), 1);
    mapped[0] = handle;
    return handle;
}

Texture2D& TextureResourceType::createTexture2D(TextureKey key, TextureDescriptor2D &descriptor) {
    auto& entry = textures2D.emplace_back();
    entry.texture = CreateTexture2DFromDescriptor(descriptor);
  //  entry.params = descriptor.params;

    allTextures[key.id()].resID = textures2D.size() - 1;

    const size_t handle = makeTextureResident(entry.texture.id(), key);

    allTextures[key.id()].handle = handle;
    return entry.texture;
}

Texture2DMS & TextureResourceType::createTexture2DMS(TextureKey texture, TextureDescriptor2DMS &descriptor) {
    auto& entry = textures2DMS.emplace_back();
    entry.texture = CreateTexture2DMSFromDescriptor(descriptor);

    const size_t handle = makeTextureResident(entry.texture.id(), texture);

    allTextures[texture.id()].handle = handle;
    allTextures[texture.id()].resID = textures2DMS.size() - 1;
    return entry.texture;
}

Texture2DArray & TextureResourceType::createTextureArray2D(TextureKey texture, TextureDescriptor2DArray &descriptor) {
    auto& entry = textures2DArray.emplace_back();
    entry.texture = Texture2DArray(descriptor.images, descriptor.params);

    const size_t handle = makeTextureResident(entry.texture.id(), texture);

    allTextures[texture.id()].handle = handle;
    allTextures[texture.id()].resID = textures2DArray.size() - 1;
    return entry.texture;
}

Cubemap& TextureResourceType::createEmptyCubemap(TextureKey key, const EmptyCubemapDescriptor2D &descriptor) {
    auto& entry = cubemaps.emplace_back();
    entry.cubemap = Cubemap(descriptor.width, descriptor.height, descriptor.params);
    entry.params = descriptor.params;

    allTextures[key.id()].resID = cubemaps.size() - 1;

    const size_t handle = makeTextureResident(entry.cubemap.id(), key);
    allTextures[key.id()].handle = handle;

    return entry.cubemap;
}

TextureResourceType::TextureResourceType(Renderer& renderer) : renderer(&renderer) {
    generator.generate(TextureTarget::TEXTURE_2D);
    cpuToGpu.emplace_back();
    allTextures.emplace_back();
}

template <typename T>
void FillTextureArray(T& arr, const TextureKey key) {
    arr.reserve(key.id() + 1);

    while (arr.size() != arr.capacity()) {
        arr.emplace_back();
    }
}

void TextureResourceType::synchronize() {

    if (firstSync) {
        firstSync = false;
        const Image img = ImageLoader::empty(1, 1, PixelType::UNSIGNED_BYTE, ImageChannels::RGBA, DynamicPixel{0,0,0,1});

        static constexpr TextureCreateParams Params;


        Texture2D tex2D(img, Params);



        textures2D.emplace_back();

        textures2D[0] = Texture2DResource{std::move(tex2D), Params};

    }

    const size_t reserve = generator.next;

    if (allTextures.size() < reserve) {
        allTextures.resize(reserve);
    }

    for (auto& [createTexture2D, createEmptyCubemap, createTextureArray2D] : stagingBuffer.ops) {

        for (auto& [key, tex2Desc] : createTexture2D) {
            this->createTexture2D(key, tex2Desc);
        }

        for (auto& [key, cubemapDesc] : createEmptyCubemap) {
            this->createEmptyCubemap(key, cubemapDesc);
        }

        for (auto& [key, texArrayDesc] : createTextureArray2D) {
            this->createTextureArray2D(key, texArrayDesc);
        }
        createTexture2D.clear();
        createEmptyCubemap.clear();
        createTextureArray2D.clear();
    }

    for (auto& [key, tex2DMSDesc] : stagingBuffer.texture2DMS) {
        this->createTexture2DMS(key, tex2DMSDesc);
    }
    stagingBuffer.texture2DMS = {};

}

FramebufferAttachmentDescriptor TextureResourceType::createAttachmentParams(const RenderTexture& renderTexture) const {
    const TextureBase* base = nullptr;

    switch (renderTexture.texture.type()) {
        case TextureTarget::TEXTURE_2D: {
            base = &textures2D[getResourceID(renderTexture.texture)].texture;
            break;
        }
        case TextureTarget::TEXTURE_ARRAY_2D: {
            base = &textures2DArray[getResourceID(renderTexture.texture)].texture;
            break;
        }
        case TextureTarget::CUBEMAP: {
            base = &cubemaps[getResourceID(renderTexture.texture)].cubemap;
            break;
        }
        case TextureTarget::TEXTURE_2D_MSAA: {
            base = &textures2DMS[getResourceID(renderTexture.texture)].texture;
            break;
        }
        default: assert(false);
    }

    const int width = base->width();
    const int height = base->height();
    const unsigned gpuID = base->id();

    FramebufferAttachmentCreateParams params;
    params.texture = gpuID;
    params.face = renderTexture.face;
    params.arrayIndex = renderTexture.layer;
    params.type = renderTexture.texture.type();
    params.level = static_cast<int>(renderTexture.mipmap);
    params.attachmentIndex = renderTexture.attachment;

    return {params, width, height};
}

FrameBufferObject & TextureResourceType::createFramebuffer(const RenderTexture& renderTexture) {
    if (renderTexture.texture == DEFAULT_FRAMEBUFFER_SCREEN_TEXTURE_KEY) {
        assert(false);
    }

    auto [params, width, height] = createAttachmentParams(renderTexture);

    auto fbo = FrameBufferObject(width, height, params);

    return textureFramebuffers.emplace(renderTexture, std::move(fbo)).first->second;
}

FrameBufferObject & TextureResourceType::createFramebuffer(const MultiRenderTexture &renderTexture) {
    if (renderTexture.empty()) {
        assert(false);
    }

    if (renderTexture.size() == 1) {
        return createFramebuffer(renderTexture[0]);
    }

    int finalWidth = 0;
    int finalHeight = 0;

    FramebufferAttachmentCreateParams finalParams[8];

    size_t i = 0;
    for (const auto& texture : renderTexture.targets()) {
        auto [params, width, height] = createAttachmentParams(texture);

        if (finalWidth == 0) {
            finalWidth = width;
            finalHeight = height;
        } else {
            assert(finalWidth == width && finalHeight == height);
        }
        finalParams[i] = params;
        ++i;
    }

    const auto it = multiRenderTextureFramebuffers.emplace(renderTexture, FrameBufferObject(finalWidth, finalHeight, mem::make_range(finalParams, i)));
    return it.first->second;
}

TextureFactory::TextureFactory(Renderer &renderer) : textures(&renderer.getTextureStorage()) {
}

TextureKey TextureFactory::createTexture2D(TextureDescriptor2D &descriptor) const {
    TextureKey key = textures->newTexture(TextureTarget::TEXTURE_2D);
    textures->stagingBuffer.ops.local().createTexture.emplace_back(key, std::move(descriptor));
    return key;
}

TextureKey TextureFactory::createTexture2D(TextureDescriptor2D &&descriptor) const {
    return createTexture2D(descriptor);
}

TextureKey TextureFactory::createCubemap(const EmptyCubemapDescriptor2D &descriptor) const {
    TextureKey key = textures->newTexture(TextureTarget::CUBEMAP);
    textures->stagingBuffer.ops.local().createEmptyCubemap.emplace_back(key, descriptor);
    return key;
}

TextureKey TextureFactory::createTexture2DMS(const TextureDescriptor2DMS &descriptor) const {
    TextureKey key = textures->newTexture(TextureTarget::TEXTURE_2D_MSAA);
    textures->stagingBuffer.texture2DMS.emplace_back(key, descriptor);
    return key;
}

TextureKey TextureFactory::createTextureArray2D(TextureDescriptor2DArray &&descriptor) const {
    TextureKey key = textures->newTexture(TextureTarget::TEXTURE_ARRAY_2D);
    textures->stagingBuffer.ops.local().createTextureArray2D.emplace_back(key, std::move(descriptor));
    return key;
}

TextureQuery::TextureQuery(Renderer &renderer) : textures(&renderer.getTextureStorage()) {}

const Texture2D & TextureQuery::getTexture2D(const TextureKey texture) const {
    if (texture.type() != TextureTarget::TEXTURE_2D) {
        assert(false);
    }

    const auto resID = textures->allTextures[texture.id()].resID;

    if (resID > textures->textures2D.size()) {
        return textures->textures2D.front().texture;
    }
    return textures->textures2D[resID].texture;
}

const Cubemap & TextureQuery::getCubemap(TextureKey texture) const {
    if (texture.type() != TextureTarget::CUBEMAP) {
        assert(false);
    }

    const auto resID = textures->allTextures[texture.id()].resID;

    if (resID > textures->cubemaps.size()) {
        return textures->cubemaps.front().cubemap;
    }
    return textures->cubemaps[resID].cubemap;
}

const Texture2DMS & TextureQuery::getTexture2DMS(TextureKey texture) const {
    if (texture.type() != TextureTarget::TEXTURE_2D_MSAA) {
        assert(false);
    }
    return textures->textures2DMS[textures->getResourceID(texture)].texture;
}

BufferKey TextureQuery::getMaterial2DBuffer() const {
    return textures->resource2DMaterialBuffer;
}

FrameBufferObject & TextureQuery::getFrameBuffer(const RenderTexture &texture) const {
    auto it = textures->textureFramebuffers.find(texture);

    if (it == textures->textureFramebuffers.end()) {
        return textures->createFramebuffer(texture);
    }
    return it->second;
}

FrameBufferObject & TextureQuery::getFramebuffer(const MultiRenderTexture &texture) const {
    if (texture.size() == 1) return getFrameBuffer(texture[0]);

    auto it = textures->multiRenderTextureFramebuffers.find(texture);
    if (it == textures->multiRenderTextureFramebuffers.end()) {
        return textures->createFramebuffer(texture);
    }
    return it->second;
}

size_t TextureQuery::getTextureGPUHandle(const TextureKey texture) const {
    return textures->allTextures[texture.id()].handle;
}

bool TextureQuery::isValid(TextureKey texture) const {
    if (texture.type() == TextureTarget::INVALID) return false;
    return textures->allTextures.size() > texture.id();
}

bool TextureQuery::isValid(SamplerKey sampler) const {
    if (sampler.isTextureDefault()) return isValid(sampler.getTexture());
    assert(false); // TODO Implement
}

size_t TextureQuery::getTextureGPUHandle(SamplerKey sampler) const {
    if (sampler.isTextureDefault()) {
        return getTextureGPUHandle(sampler.getTexture());
    } else {
        assert(false); // TODO Implement
    }
}
