#include "application.hpp"

#include <imgui.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/constants.hpp>

#include <fstream>
#include <vector>
#include <iostream>
#include <cstring>
#include <cmath>
#include <cstdint>
#include <array>
#include <limits>
#include <functional>

#define STB_IMAGE_IMPLEMENTATION
#include "../third_party/stb_image.h"

namespace application {

struct Vertex {
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec2 uv;
};

std::vector<Vertex> vertices;
std::vector<uint16_t> indices;


void generateCylinder(float radius, float height, int segments) {
    vertices.clear();
    indices.clear();

    const float halfHeight = height / 2.0f;
    const float pi = glm::pi<float>();

    for (int i = 0; i <= segments; ++i) {
        const float theta =
            static_cast<float>(i) * 2.0f * pi /
            static_cast<float>(segments);

        const float x = radius * std::cos(theta);
        const float z = radius * std::sin(theta);
        const float u =
            static_cast<float>(i) / static_cast<float>(segments);

        // Нормаль направлена радиально наружу
        const glm::vec3 normal =
            glm::normalize(glm::vec3(x, 0.0f, z));

        vertices.push_back({
            {x, halfHeight, z},
            normal,
            {u, 0.0f}
        });

        vertices.push_back({
            {x, -halfHeight, z},
            normal,
            {u, 1.0f}
        });
    }

    for (int i = 0; i < segments; ++i) {
        const uint16_t top1 =
            static_cast<uint16_t>(i * 2);
        const uint16_t bottom1 =
            static_cast<uint16_t>(i * 2 + 1);
        const uint16_t top2 =
            static_cast<uint16_t>((i + 1) * 2);
        const uint16_t bottom2 =
            static_cast<uint16_t>((i + 1) * 2 + 1);

        indices.push_back(top1);
        indices.push_back(bottom1);
        indices.push_back(top2);

        indices.push_back(top2);
        indices.push_back(bottom1);
        indices.push_back(bottom2);
    }

    const uint16_t topCenter =
        static_cast<uint16_t>(vertices.size());

    vertices.push_back({
        {0.0f, halfHeight, 0.0f},
        {0.0f, 1.0f, 0.0f},
        {0.5f, 0.5f}
    });

    const uint16_t topRing =
        static_cast<uint16_t>(vertices.size());

    for (int i = 0; i <= segments; ++i) {
        const float theta =
            static_cast<float>(i) * 2.0f * pi /
            static_cast<float>(segments);

        const float x = radius * std::cos(theta);
        const float z = radius * std::sin(theta);
        vertices.push_back({
            {x, halfHeight, z},
            {0.0f, 1.0f, 0.0f},
            {0.5f + 0.5f * std::cos(theta), 0.5f + 0.5f * std::sin(theta)}
        });
    }

    for (int i = 0; i < segments; ++i) {
        const uint16_t current =
            static_cast<uint16_t>(topRing + i);
        const uint16_t next =
            static_cast<uint16_t>(topRing + i + 1);

        indices.push_back(topCenter);
        indices.push_back(next);
        indices.push_back(current);
    }

    const uint16_t bottomCenter =
        static_cast<uint16_t>(vertices.size());

    vertices.push_back({
        {0.0f, -halfHeight, 0.0f},
        {0.0f, -1.0f, 0.0f},
        {0.5f, 0.5f}
    });

    const uint16_t bottomRing =
        static_cast<uint16_t>(vertices.size());

    for (int i = 0; i <= segments; ++i) {
        const float theta =
            static_cast<float>(i) * 2.0f * pi /
            static_cast<float>(segments);

        const float x = radius * std::cos(theta);
        const float z = radius * std::sin(theta);
        vertices.push_back({
            {x, -halfHeight, z},
            {0.0f, -1.0f, 0.0f},
            {0.5f + 0.5f * std::cos(theta), 0.5f + 0.5f * std::sin(theta)}
        });
    }

    for (int i = 0; i < segments; ++i) {
        const uint16_t current =
            static_cast<uint16_t>(bottomRing + i);
        const uint16_t next =
            static_cast<uint16_t>(bottomRing + i + 1);

        indices.push_back(bottomCenter);
        indices.push_back(current);
        indices.push_back(next);
    }
}

struct SceneState {
    glm::vec3 translation = glm::vec3(0.0f);
    glm::vec3 rotation = glm::vec3(0.0f);
    glm::vec3 scale = glm::vec3(1.0f);

    glm::vec3 cameraPos = glm::vec3(0.0f, 3.0f, 7.0f);
    // Углы камеры в градусах: yaw, pitch, roll.
    glm::vec3 cameraRotation = glm::vec3(0.0f, -20.0f, 0.0f);

    bool isPerspective = true;
    float fov = 45.0f;
    float orthoSize = 2.0f;
    float nearPlane = 0.1f;
    float farPlane = 100.0f;
    bool isPlaying = true;
    float animationTime = 0.0f;
    float speed = 1.0f;
    float radius = 1.5f;
} state;

struct LightingState {
    glm::vec3 direction = glm::vec3(-1.0f, -1.0f, -1.0f);
    glm::vec3 lightColor = glm::vec3(1.0f);
    float lightIntensity = 1.0f;

    // Фоновое освещение.
    glm::vec3 ambientColor = glm::vec3(1.0f);
    float ambientIntensity = 0.15f;
    float shadowArea = 8.0f;
    float depthBias = 1.25f;

};

LightingState lighting;

struct MaterialState {
    glm::vec3 specularColor{1.0f};
    float shininess = 32.0f;
} material;

struct alignas(16) Uniforms {
    glm::mat4 model{1.0f};
    glm::mat4 viewProjection{1.0f};
    glm::mat4 lightViewProjection{1.0f};
    glm::vec4 cameraPosition{0.0f, 0.0f, 4.0f, 1.0f};

    glm::vec4 lightDirection{
        -1.0f, -1.0f, -1.0f, 0.0f
    };

    glm::vec4 lightColorIntensity{
        1.0f, 1.0f, 1.0f, 1.0f
    };

    glm::vec4 ambientColorIntensity{
        1.0f, 1.0f, 1.0f, 0.15f
    };

    glm::vec4 specularColorShininess{1.0f, 1.0f, 1.0f, 32.0f};
};

static_assert(sizeof(Uniforms) % 16 == 0);


static VkBuffer vertexBuffer = VK_NULL_HANDLE;
static VmaAllocation vertexAllocation = VK_NULL_HANDLE;

static uint32_t indexCount = 0;
static VkBuffer indexBuffer = VK_NULL_HANDLE;
static VmaAllocation indexAllocation = VK_NULL_HANDLE;

static VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
static VkPipeline graphicsPipeline     = VK_NULL_HANDLE;


struct MappedBuffer {
    VkBuffer buffer = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;
    void* mapped = nullptr;
};

static constexpr uint32_t maxObjectCount = 3;
static uint32_t shadowMapSize = 1024;
static bool softwareRenderer = false;
static uint32_t objectCount = 1;
static MappedBuffer uniformBuffer;
static VkDeviceSize uniformStride = 0;
static VkImage textureImage = VK_NULL_HANDLE;
static VmaAllocation textureAllocation = VK_NULL_HANDLE;
static VkImageView textureView = VK_NULL_HANDLE;
static VkSampler textureSampler = VK_NULL_HANDLE;
static VkImage shadowImage = VK_NULL_HANDLE;
static VmaAllocation shadowAllocation = VK_NULL_HANDLE;
static VkImageView shadowView = VK_NULL_HANDLE;
static VkSampler shadowSampler = VK_NULL_HANDLE;
static VkPipeline shadowPipeline = VK_NULL_HANDLE;
static uint32_t floorFirstIndex = 0;
static uint32_t floorIndexCount = 0;
static bool shadowMapInitialized = false;

static VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
static VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
static VkDescriptorSet descriptorSet = VK_NULL_HANDLE;

static void addFloor() {
    const uint16_t base = static_cast<uint16_t>(vertices.size());
    floorFirstIndex = static_cast<uint32_t>(indices.size());
    vertices.push_back({{-7.0f, -1.2f, -7.0f}, {0, 1, 0}, {0, 0}});
    vertices.push_back({{ 7.0f, -1.2f, -7.0f}, {0, 1, 0}, {1, 0}});
    vertices.push_back({{ 7.0f, -1.2f,  7.0f}, {0, 1, 0}, {1, 1}});
    vertices.push_back({{-7.0f, -1.2f,  7.0f}, {0, 1, 0}, {0, 1}});
    indices.insert(indices.end(), {base, uint16_t(base + 2), uint16_t(base + 1),
                                   base, uint16_t(base + 3), uint16_t(base + 2)});
    floorIndexCount = 6;
}

static std::vector<char> readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    if (!file.is_open()) { 
        std::cerr << "Failed to open shader file: " << filename << std::endl;
        return {};
    }
    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();
    return buffer;
}

static VkShaderModule createShaderModule(VkDevice device, const std::vector<char>& code) {
    if (code.empty()) return VK_NULL_HANDLE;

    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        std::cerr << "Failed to create shader module!" << std::endl;
        return VK_NULL_HANDLE;
    }
    return shaderModule;
}

static bool createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage, VkBuffer& buffer, VmaAllocation& allocation) {
    auto& context = graphics::internal::context;

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = memoryUsage;
    allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

    return vmaCreateBuffer(context.allocator, &bufferInfo, &allocInfo, &buffer, &allocation, nullptr) == VK_SUCCESS;
}


static bool createMappedBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                               MappedBuffer& target) {
    auto& context = graphics::internal::context;
    if (!createBuffer(size, usage, VMA_MEMORY_USAGE_AUTO,
                      target.buffer, target.allocation)) return false;
    if (vmaMapMemory(context.allocator, target.allocation, &target.mapped) != VK_SUCCESS) {
        vmaDestroyBuffer(context.allocator, target.buffer, target.allocation);
        target = {};
        return false;
    }
    return true;
}

static void writeMappedBuffer(const MappedBuffer& target, const void* data,
                              VkDeviceSize size, VkDeviceSize offset = 0) {
    auto& context = graphics::internal::context;
    std::memcpy(static_cast<char*>(target.mapped) + offset, data,
                static_cast<size_t>(size));
    vmaFlushAllocation(context.allocator, target.allocation, offset, size);
}

static void destroyMappedBuffer(MappedBuffer& target) {
    auto& context = graphics::internal::context;
    if (target.mapped) vmaUnmapMemory(context.allocator, target.allocation);
    if (target.buffer != VK_NULL_HANDLE)
        vmaDestroyBuffer(context.allocator, target.buffer, target.allocation);
    target = {};
}

static bool createDeviceBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                               VkBuffer& buffer, VmaAllocation& allocation) {
    auto& context = graphics::internal::context;
    VkBufferCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    info.size = size;
    info.usage = usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VmaAllocationCreateInfo memory{};
    memory.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
    memory.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    return vmaCreateBuffer(context.allocator, &info, &memory,
                           &buffer, &allocation, nullptr) == VK_SUCCESS;
}

static bool submitTransfer(const std::function<void(VkCommandBuffer)>& record) {
    auto& context = graphics::internal::context;
    VkCommandPool pool = VK_NULL_HANDLE;
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = context.graphics_queue_index;
    if (vkCreateCommandPool(context.device, &poolInfo, nullptr, &pool) != VK_SUCCESS)
        return false;

    VkCommandBuffer cmd = VK_NULL_HANDLE;
    VkCommandBufferAllocateInfo allocate{};
    allocate.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocate.commandPool = pool;
    allocate.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocate.commandBufferCount = 1;
    bool ok = vkAllocateCommandBuffers(context.device, &allocate, &cmd) == VK_SUCCESS;
    if (ok) {
        VkCommandBufferBeginInfo begin{};
        begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        ok = vkBeginCommandBuffer(cmd, &begin) == VK_SUCCESS;
        if (ok) record(cmd);
        if (ok) ok = vkEndCommandBuffer(cmd) == VK_SUCCESS;
        if (ok) {
            VkSubmitInfo submit{};
            submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submit.commandBufferCount = 1;
            submit.pCommandBuffers = &cmd;
            ok = vkQueueSubmit(context.graphics_queue, 1, &submit,
                               VK_NULL_HANDLE) == VK_SUCCESS;
            if (ok) ok = vkQueueWaitIdle(context.graphics_queue) == VK_SUCCESS;
        }
    }
    vkDestroyCommandPool(context.device, pool, nullptr);
    return ok;
}

static bool uploadBuffer(const void* data, VkDeviceSize size,
                         VkBufferUsageFlags usage, VkBuffer& buffer,
                         VmaAllocation& allocation) {
    MappedBuffer staging;
    if (!createMappedBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, staging))
        return false;
    writeMappedBuffer(staging, data, size);
    bool ok = createDeviceBuffer(size, usage, buffer, allocation);
    if (ok) {
        ok = submitTransfer([&](VkCommandBuffer cmd) {
            VkBufferCopy region{};
            region.size = size;
            vkCmdCopyBuffer(cmd, staging.buffer, buffer, 1, &region);
        });
    }
    destroyMappedBuffer(staging);
    return ok;
}

static bool createTexture() {
    auto& context = graphics::internal::context;
    int width = 0, height = 0, channels = 0;
    stbi_uc* pixels = stbi_load("assets/cylinder.png", &width, &height,
                                &channels, STBI_rgb_alpha);
    if (!pixels) {
        std::cerr << "Failed to load assets/cylinder.png: "
                  << stbi_failure_reason() << '\n';
        return false;
    }
    if (width <= 0 || height <= 0) {
        stbi_image_free(pixels);
        return false;
    }
    const VkDeviceSize size = VkDeviceSize(width) * VkDeviceSize(height) * 4;
    MappedBuffer staging;
    bool ok = createMappedBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, staging);
    if (ok) writeMappedBuffer(staging, pixels, size);
    stbi_image_free(pixels);
    if (!ok) return false;

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    imageInfo.extent = {uint32_t(width), uint32_t(height), 1};
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    VmaAllocationCreateInfo memory{};
    memory.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
    memory.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    ok = vmaCreateImage(context.allocator, &imageInfo, &memory, &textureImage,
                        &textureAllocation, nullptr) == VK_SUCCESS;
    if (ok) {
        ok = submitTransfer([&](VkCommandBuffer cmd) {
            VkImageMemoryBarrier barrier{};
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.image = textureImage;
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.levelCount = 1;
            barrier.subresourceRange.layerCount = 1;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                 VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                                 0, nullptr, 0, nullptr, 1, &barrier);

            VkBufferImageCopy region{};
            region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            region.imageSubresource.layerCount = 1;
            region.imageExtent = {uint32_t(width), uint32_t(height), 1};
            vkCmdCopyBufferToImage(cmd, staging.buffer, textureImage,
                                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT,
                                 VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                                 0, nullptr, 0, nullptr, 1, &barrier);
        });
    }
    destroyMappedBuffer(staging);
    if (!ok) return false;

    VkImageViewCreateInfo view{};
    view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view.image = textureImage;
    view.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view.format = imageInfo.format;
    view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    view.subresourceRange.levelCount = 1;
    view.subresourceRange.layerCount = 1;
    if (vkCreateImageView(context.device, &view, nullptr, &textureView) != VK_SUCCESS)
        return false;

    VkSamplerCreateInfo sampler{};
    sampler.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler.magFilter = VK_FILTER_LINEAR;
    sampler.minFilter = VK_FILTER_LINEAR;
    sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler.maxLod = 0.0f;
    return vkCreateSampler(context.device, &sampler, nullptr,
                           &textureSampler) == VK_SUCCESS;
}

static bool createShadowMap() {
    auto& context = graphics::internal::context;
    VkFormatProperties properties{};
    vkGetPhysicalDeviceFormatProperties(context.physical_device, VK_FORMAT_D32_SFLOAT, &properties);
    const VkFormatFeatureFlags required = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT |
                                          VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT;
    if ((properties.optimalTilingFeatures & required) != required) return false;

    VkImageCreateInfo image{};
    image.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image.imageType = VK_IMAGE_TYPE_2D;
    image.format = VK_FORMAT_D32_SFLOAT;
    image.extent = {shadowMapSize, shadowMapSize, 1};
    image.mipLevels = 1;
    image.arrayLayers = 1;
    image.samples = VK_SAMPLE_COUNT_1_BIT;
    image.tiling = VK_IMAGE_TILING_OPTIMAL;
    image.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    image.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VmaAllocationCreateInfo allocation{};
    allocation.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
    if (vmaCreateImage(context.allocator, &image, &allocation,
                       &shadowImage, &shadowAllocation, nullptr) != VK_SUCCESS) return false;

    VkImageViewCreateInfo view{};
    view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view.image = shadowImage;
    view.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view.format = image.format;
    view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    view.subresourceRange.levelCount = 1;
    view.subresourceRange.layerCount = 1;
    if (vkCreateImageView(context.device, &view, nullptr, &shadowView) != VK_SUCCESS) return false;

    VkSamplerCreateInfo sampler{};
    sampler.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler.magFilter = VK_FILTER_NEAREST;
    sampler.minFilter = VK_FILTER_NEAREST;
    sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    sampler.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    sampler.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    sampler.compareEnable = VK_TRUE;
    sampler.compareOp = VK_COMPARE_OP_LESS;
    sampler.maxLod = 0.0f;
    return vkCreateSampler(context.device, &sampler, nullptr, &shadowSampler) == VK_SUCCESS;
}

static bool createDescriptorSet() {
    auto& context = graphics::internal::context;
    const VkDescriptorSetLayoutBinding bindings[] = {
        {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1,
         VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
        {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
         VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
        {2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
         VK_SHADER_STAGE_FRAGMENT_BIT, nullptr}
    };
    VkDescriptorSetLayoutCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    info.bindingCount = 3;
    info.pBindings = bindings;
    if (vkCreateDescriptorSetLayout(context.device, &info, nullptr,
                                    &descriptorSetLayout) != VK_SUCCESS) return false;

    const VkDescriptorPoolSize poolSizes[] = {
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2}
    };
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.maxSets = 1;
    poolInfo.poolSizeCount = 2;
    poolInfo.pPoolSizes = poolSizes;
    if (vkCreateDescriptorPool(context.device, &poolInfo, nullptr,
                               &descriptorPool) != VK_SUCCESS) return false;

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &descriptorSetLayout;
    if (vkAllocateDescriptorSets(context.device, &allocInfo, &descriptorSet) != VK_SUCCESS)
        return false;

    const VkDescriptorBufferInfo bufferInfo = {
        uniformBuffer.buffer, 0, sizeof(Uniforms)
    };
    const VkDescriptorImageInfo imageInfo = {
        textureSampler, textureView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    };
    const VkDescriptorImageInfo shadowInfo = {
        shadowSampler, shadowView, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
    };
    VkWriteDescriptorSet writes[3]{};
    writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[0].dstSet = descriptorSet;
    writes[0].dstBinding = 0;
    writes[0].descriptorCount = 1;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    writes[0].pBufferInfo = &bufferInfo;
    writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[1].dstSet = descriptorSet;
    writes[1].dstBinding = 1;
    writes[1].descriptorCount = 1;
    writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[1].pImageInfo = &imageInfo;
    writes[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[2].dstSet = descriptorSet;
    writes[2].dstBinding = 2;
    writes[2].descriptorCount = 1;
    writes[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[2].pImageInfo = &shadowInfo;
    vkUpdateDescriptorSets(context.device, 3, writes, 0, nullptr);
    return true;
}

bool initialize() {
    auto& context = graphics::internal::context;

    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(context.physical_device, &properties);
    softwareRenderer = properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_CPU;
    shadowMapSize = softwareRenderer ? 512 : 1024;

    generateCylinder(0.8f, 1.5f, 50);
    addFloor();

    if (vertices.empty() || indices.empty()) {
        std::cerr << "Vertices or indices vector is empty!\n";
        return false;
    }

    const VkDeviceSize vertexSize =
        sizeof(vertices[0]) * vertices.size();

    const VkDeviceSize indexSize = sizeof(indices[0]) * indices.size();
    if (!uploadBuffer(vertices.data(), vertexSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                      vertexBuffer, vertexAllocation) ||
        !uploadBuffer(indices.data(), indexSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                      indexBuffer, indexAllocation)) {
        std::cerr << "Failed to upload geometry to device-local memory!\n";
        return false;
    }

    indexCount = floorFirstIndex;
    vertices.clear();
    vertices.shrink_to_fit();
    indices.clear();
    indices.shrink_to_fit();

    if (!createTexture() || !createShadowMap()) {
        std::cerr << "Failed to create texture or shadow map!\n";
        return false;
    }

    const VkDeviceSize alignment =
        properties.limits.minUniformBufferOffsetAlignment;
    uniformStride = alignment == 0 ? sizeof(Uniforms) :
        (sizeof(Uniforms) + alignment - 1) / alignment * alignment;
    if (uniformStride * maxObjectCount > std::numeric_limits<uint32_t>::max()) {
        std::cerr << "Dynamic uniform offsets exceed uint32 range!\n";
        return false;
    }
    if (!createMappedBuffer(uniformStride * (maxObjectCount + 1),
                            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, uniformBuffer) ||
        !createDescriptorSet()) {
        std::cerr << "Failed to initialize uniform buffer or descriptor set!\n";
        return false;
    }

    auto vertCode = readFile("shaders/shader.vert.spv");
    auto fragCode = readFile("shaders/shader.frag.spv");

    if (vertCode.empty() || fragCode.empty()) {
        std::cerr << "Failed to read shader files!\n";
        return false;
    }

    VkShaderModule vertModule =
        createShaderModule(context.device, vertCode);

    VkShaderModule fragModule =
        createShaderModule(context.device, fragCode);

    if (vertModule == VK_NULL_HANDLE ||
        fragModule == VK_NULL_HANDLE) {
        std::cerr << "Failed to create shader modules!\n";

        if (vertModule != VK_NULL_HANDLE) {
            vkDestroyShaderModule(
                context.device, vertModule, nullptr);
        }

        if (fragModule != VK_NULL_HANDLE) {
            vkDestroyShaderModule(
                context.device, fragModule, nullptr);
        }

        return false;
    }

    VkPipelineShaderStageCreateInfo vertStageInfo{};
    vertStageInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertStageInfo.module = vertModule;
    vertStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragStageInfo{};
    fragStageInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragStageInfo.module = fragModule;
    fragStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {
        vertStageInfo,
        fragStageInfo
    };

    // Position, normal and texture coordinates.
    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(Vertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{{
        {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos)},
        {1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal)},
        {2, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv)}
    }};

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions =
        &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount =
        static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions =
        attributeDescriptions.data();

    // Сборка примитивов
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType =
        VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology =
        VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    // Viewport и scissor задаются динамически при рендеринге
    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType =
        VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    std::vector<VkDynamicState> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType =
        VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount =
        static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType =
        VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_NONE;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType =
        VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples =
        VK_SAMPLE_COUNT_1_BIT;

    // Тест глубины
    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType =
        VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;

    // Смешивание цветов
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT |
        VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT |
        VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType =
        VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    // Один набор с параметрами камеры, света и материала.
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;

    if (vkCreatePipelineLayout(
            context.device,
            &pipelineLayoutInfo,
            nullptr,
            &pipelineLayout) != VK_SUCCESS) {
        std::cerr << "Failed to create pipeline layout!\n";

        vkDestroyShaderModule(
            context.device, fragModule, nullptr);
        vkDestroyShaderModule(
            context.device, vertModule, nullptr);

        return false;
    }

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType =
        VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.renderPass = context.render_pass;
    pipelineInfo.subpass = 0;

    if (vkCreateGraphicsPipelines(
            context.device,
            VK_NULL_HANDLE,
            1,
            &pipelineInfo,
            nullptr,
            &graphicsPipeline) != VK_SUCCESS) {
        std::cerr << "Failed to create graphics pipeline!\n";

        vkDestroyShaderModule(
            context.device, fragModule, nullptr);
        vkDestroyShaderModule(
            context.device, vertModule, nullptr);

        return false;
    }

    auto shadowCode = readFile("shaders/shadow.vert.spv");
    VkShaderModule shadowModule = createShaderModule(context.device, shadowCode);
    if (shadowModule == VK_NULL_HANDLE) return false;
    VkPipelineShaderStageCreateInfo shadowStage = vertStageInfo;
    shadowStage.module = shadowModule;
    const VkDynamicState shadowStates[] = {
        VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_DEPTH_BIAS
    };
    dynamicState.dynamicStateCount = 3;
    dynamicState.pDynamicStates = shadowStates;
    rasterizer.depthBiasEnable = VK_TRUE;
    colorBlending.attachmentCount = 0;
    colorBlending.pAttachments = nullptr;
    VkPipelineRenderingCreateInfo rendering{};
    rendering.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    rendering.depthAttachmentFormat = VK_FORMAT_D32_SFLOAT;
    pipelineInfo.pNext = &rendering;
    pipelineInfo.stageCount = 1;
    pipelineInfo.pStages = &shadowStage;
    pipelineInfo.renderPass = VK_NULL_HANDLE;
    const bool shadowOk = vkCreateGraphicsPipelines(context.device, VK_NULL_HANDLE, 1,
                                                    &pipelineInfo, nullptr, &shadowPipeline) == VK_SUCCESS;
    vkDestroyShaderModule(context.device, shadowModule, nullptr);
    if (!shadowOk) {
        std::cerr << "Failed to create shadow pipeline!\n";
        vkDestroyShaderModule(context.device, fragModule, nullptr);
        vkDestroyShaderModule(context.device, vertModule, nullptr);
        return false;
    }

    vkDestroyShaderModule(
        context.device, fragModule, nullptr);
    vkDestroyShaderModule(
        context.device, vertModule, nullptr);

    return true;
}

void shutdown() {
    auto& context = graphics::internal::context;

    if (context.graphics_queue != VK_NULL_HANDLE) {
        vkQueueWaitIdle(context.graphics_queue);
    }

    if (descriptorPool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(
            context.device, descriptorPool, nullptr);
        descriptorPool = VK_NULL_HANDLE;
        descriptorSet = VK_NULL_HANDLE;
    }

    if (graphicsPipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(
            context.device, graphicsPipeline, nullptr);
        graphicsPipeline = VK_NULL_HANDLE;
    }
    if (shadowPipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(context.device, shadowPipeline, nullptr);
        shadowPipeline = VK_NULL_HANDLE;
    }

    if (pipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(
            context.device, pipelineLayout, nullptr);
        pipelineLayout = VK_NULL_HANDLE;
    }

    if (descriptorSetLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(
            context.device, descriptorSetLayout, nullptr);
        descriptorSetLayout = VK_NULL_HANDLE;
    }
    if (textureSampler != VK_NULL_HANDLE)
        vkDestroySampler(context.device, textureSampler, nullptr);
    if (shadowSampler != VK_NULL_HANDLE)
        vkDestroySampler(context.device, shadowSampler, nullptr);
    if (shadowView != VK_NULL_HANDLE)
        vkDestroyImageView(context.device, shadowView, nullptr);
    if (shadowImage != VK_NULL_HANDLE)
        vmaDestroyImage(context.allocator, shadowImage, shadowAllocation);
    shadowSampler = VK_NULL_HANDLE;
    shadowView = VK_NULL_HANDLE;
    shadowImage = VK_NULL_HANDLE;
    shadowAllocation = VK_NULL_HANDLE;
    if (textureView != VK_NULL_HANDLE)
        vkDestroyImageView(context.device, textureView, nullptr);
    if (textureImage != VK_NULL_HANDLE)
        vmaDestroyImage(context.allocator, textureImage, textureAllocation);
    textureSampler = VK_NULL_HANDLE;
    textureView = VK_NULL_HANDLE;
    textureImage = VK_NULL_HANDLE;
    textureAllocation = VK_NULL_HANDLE;
    destroyMappedBuffer(uniformBuffer);

    if (vertexBuffer != VK_NULL_HANDLE) {
        vmaDestroyBuffer(
            context.allocator,
            vertexBuffer,
            vertexAllocation);

        vertexBuffer = VK_NULL_HANDLE;
        vertexAllocation = VK_NULL_HANDLE;
    }

    if (indexBuffer != VK_NULL_HANDLE) {
        vmaDestroyBuffer(
            context.allocator,
            indexBuffer,
            indexAllocation);

        indexBuffer = VK_NULL_HANDLE;
        indexAllocation = VK_NULL_HANDLE;
    }
}



void update(double time) {
    ImGui::Begin("3D Transformations & Projection");

    if (ImGui::CollapsingHeader(
            "Animation Controls",
            ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Checkbox("Play / Pause", &state.isPlaying);
        ImGui::SliderFloat("Speed", &state.speed, 0.0f, 5.0f);
        ImGui::SliderFloat(
            "Trajectory Radius", &state.radius, 0.5f, 5.0f);
    }

    if (ImGui::CollapsingHeader(
            "Scene Objects",
            ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::Button("Add cylinder") && objectCount < maxObjectCount)
            ++objectCount;
        ImGui::SameLine();
        if (ImGui::Button("Remove cylinder") && objectCount > 1)
            --objectCount;
        ImGui::Text("Cylinders: %u / %u", objectCount, maxObjectCount);
    }

    if (ImGui::CollapsingHeader(
            "Model Transformations",
            ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::DragFloat3(
            "Position",
            glm::value_ptr(state.translation),
            0.05f);

        ImGui::DragFloat3(
            "Rotation",
            glm::value_ptr(state.rotation),
            1.0f);

        ImGui::DragFloat3(
            "Scale",
            glm::value_ptr(state.scale),
            0.05f,
            0.01f,
            10.0f);
    }

    if (ImGui::CollapsingHeader(
            "Camera (View)",
            ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::DragFloat3(
            "Camera Pos",
            glm::value_ptr(state.cameraPos),
            0.05f);

        ImGui::DragFloat("Yaw", &state.cameraRotation.x, 1.0f);
        ImGui::DragFloat("Pitch", &state.cameraRotation.y, 1.0f);
        ImGui::DragFloat("Roll", &state.cameraRotation.z, 1.0f);

        state.cameraRotation.y = glm::clamp(
            state.cameraRotation.y, -89.0f, 89.0f
        );
    }

    if (ImGui::CollapsingHeader(
            "Projection Matrix",
            ImGuiTreeNodeFlags_DefaultOpen)) {
        int projChoice = state.isPerspective ? 0 : 1;

        if (ImGui::RadioButton("Perspective", &projChoice, 0))
            state.isPerspective = true;

        ImGui::SameLine();

        if (ImGui::RadioButton("Orthographic", &projChoice, 1))
            state.isPerspective = false;

        if (state.isPerspective) {
            ImGui::SliderFloat(
                "FOV", &state.fov, 10.0f, 120.0f);
        } else {
            ImGui::SliderFloat(
                "Ortho Size", &state.orthoSize, 0.5f, 10.0f);
        }

        ImGui::SliderFloat(
            "Near Plane", &state.nearPlane, 0.01f, 5.0f);

        ImGui::SliderFloat(
            "Far Plane", &state.farPlane, 10.0f, 200.0f);
    }

    if (ImGui::CollapsingHeader(
            "Material & Color",
            ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::TextUnformatted("Albedo: assets/cylinder.png");

        ImGui::ColorEdit3(
            "Specular Color",
            glm::value_ptr(material.specularColor));

        ImGui::SliderFloat(
            "Shininess",
            &material.shininess,
            1.0f, 256.0f);
    }

    if (ImGui::CollapsingHeader(
            "Lighting",
            ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::ColorEdit3(
            "Light Color",
            glm::value_ptr(lighting.lightColor));

        ImGui::SliderFloat(
            "Light Intensity",
            &lighting.lightIntensity,
            0.0f, 5.0f);

        ImGui::DragFloat3(
            "Light Direction",
            glm::value_ptr(lighting.direction),
            0.05f);

        ImGui::ColorEdit3(
            "Ambient Color",
            glm::value_ptr(lighting.ambientColor));

        ImGui::SliderFloat(
            "Ambient Intensity",
            &lighting.ambientIntensity,
            0.0f, 1.0f);

        ImGui::SliderFloat("Shadow area (half-width)", &lighting.shadowArea,
                           3.0f, 20.0f);
        ImGui::SliderFloat("Depth bias", &lighting.depthBias, 0.0f, 4.0f);

    }

    ImGui::End();

    const float deltaTime =
        glm::clamp(static_cast<float>(time), 0.0f, 0.1f);

    if (state.isPlaying) {
        state.animationTime += deltaTime * state.speed;
    }

    // Матрица вида.
    const float yaw = glm::radians(state.cameraRotation.x);
    const float pitch = glm::radians(state.cameraRotation.y);
    const float roll = glm::radians(state.cameraRotation.z);

    glm::vec3 cameraDirection{
        std::cos(pitch) * std::sin(yaw),
        std::sin(pitch),
        -std::cos(pitch) * std::cos(yaw)
    };

    // Roll наклоняет локальную ось «вверх» вокруг направления взгляда.
    const glm::vec3 cameraRight = glm::normalize(
        glm::cross(cameraDirection, glm::vec3(0.0f, 1.0f, 0.0f)));
    const glm::vec3 cameraUp = glm::normalize(
        glm::cross(cameraRight, cameraDirection));
    const glm::vec3 rolledUp =
        std::cos(roll) * cameraUp + std::sin(roll) * cameraRight;

    glm::mat4 view = glm::lookAt(
        state.cameraPos,
        state.cameraPos + cameraDirection,
        rolledUp
    );

    auto& context = graphics::internal::context;

    // Защита от деления на ноль при нулевой высоте swapchain.
    if (context.swapchain_extent.height == 0 ||
        context.swapchain_extent.width == 0) {
        return;
    }

    const float aspect =
        static_cast<float>(context.swapchain_extent.width) /
        static_cast<float>(context.swapchain_extent.height);

    glm::mat4 proj(1.0f);

    if (state.isPerspective) {
        proj = glm::perspective(
            glm::radians(state.fov),
            aspect,
            state.nearPlane,
            state.farPlane);
    } else {
        const float h = state.orthoSize;
        const float w = h * aspect;

        proj = glm::orthoRH_ZO(
            -w, w,
            -h, h,
            state.nearPlane,
            state.farPlane);
    }

    proj[1][1] *= -1.0f;

    Uniforms uniforms{};
    uniforms.viewProjection = proj * view;

    // Нормализуем направление света
    const float directionLength = glm::length(lighting.direction);

    const glm::vec3 lightDirection =
        directionLength > 0.0001f
            ? lighting.direction / directionLength
            : glm::vec3(-1.0f, -1.0f, -1.0f) /
                  glm::sqrt(3.0f);

    uniforms.lightDirection =
        glm::vec4(lightDirection, 0.0f);

    const float area = lighting.shadowArea;
    glm::mat4 lightProjection = glm::orthoRH_ZO(-area, area, -area, area,
                                                0.1f, 30.0f);
    lightProjection[1][1] *= -1.0f;
    const glm::vec3 lightUp = std::abs(lightDirection.y) > 0.99f
        ? glm::vec3(0.0f, 0.0f, 1.0f) : glm::vec3(0.0f, 1.0f, 0.0f);
    uniforms.lightViewProjection = lightProjection *
        glm::lookAt(-lightDirection * 10.0f, glm::vec3(0.0f), lightUp);

    uniforms.lightColorIntensity =
        glm::vec4(
            lighting.lightColor,
            lighting.lightIntensity);

    uniforms.ambientColorIntensity =
        glm::vec4(
            lighting.ambientColor,
            lighting.ambientIntensity);

    uniforms.cameraPosition =
        glm::vec4(state.cameraPos, 1.0f);
    uniforms.specularColorShininess =
        glm::vec4(material.specularColor, material.shininess);

    for (uint32_t i = 0; i < objectCount; ++i) {
        const float phase = glm::two_pi<float>() * float(i) / float(maxObjectCount);
        const float angle = state.animationTime + phase;
        const glm::vec3 translation = state.translation + glm::vec3(
            state.radius * std::cos(angle),
            0.3f * std::sin(2.0f * angle),
            state.radius * std::sin(angle));
        const glm::vec3 rotation = state.rotation +
            glm::vec3(0.0f, glm::degrees(-angle), 0.0f);
        glm::mat4 model(1.0f);
        model = glm::translate(model, translation);
        model = glm::rotate(model, glm::radians(rotation.x), glm::vec3(1, 0, 0));
        model = glm::rotate(model, glm::radians(rotation.y), glm::vec3(0, 1, 0));
        model = glm::rotate(model, glm::radians(rotation.z), glm::vec3(0, 0, 1));
        uniforms.model = glm::scale(model, state.scale);
        writeMappedBuffer(uniformBuffer, &uniforms, sizeof(uniforms),
                          uniformStride * i);
    }
    uniforms.model = glm::mat4(1.0f);
    writeMappedBuffer(uniformBuffer, &uniforms, sizeof(uniforms),
                      uniformStride * maxObjectCount);
}


void render(const graphics::internal::FrameData& fd) {
    if (graphicsPipeline == VK_NULL_HANDLE || shadowPipeline == VK_NULL_HANDLE ||
        fd.command_buffer == VK_NULL_HANDLE || fd.framebuffer == VK_NULL_HANDLE) {
        return;
    }

    auto& context = graphics::internal::context;

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(fd.command_buffer, &beginInfo);

    VkImageMemoryBarrier shadowBarrier{};
    shadowBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    shadowBarrier.oldLayout = shadowMapInitialized
        ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_UNDEFINED;
    shadowBarrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    shadowBarrier.srcAccessMask = shadowMapInitialized ? VK_ACCESS_SHADER_READ_BIT : 0;
    shadowBarrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    shadowBarrier.image = shadowImage;
    shadowBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    shadowBarrier.subresourceRange.levelCount = 1;
    shadowBarrier.subresourceRange.layerCount = 1;
    vkCmdPipelineBarrier(fd.command_buffer,
                         shadowMapInitialized ? VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
                                              : VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                         VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
                         VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, 0,
                         0, nullptr, 0, nullptr, 1, &shadowBarrier);

    VkRenderingAttachmentInfo depthAttachment{};
    depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    depthAttachment.imageView = shadowView;
    depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depthAttachment.clearValue.depthStencil = {1.0f, 0};
    VkRenderingInfo rendering{};
    rendering.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    rendering.renderArea.extent = {shadowMapSize, shadowMapSize};
    rendering.layerCount = 1;
    rendering.pDepthAttachment = &depthAttachment;
    vkCmdBeginRendering(fd.command_buffer, &rendering);

    VkViewport shadowViewport{0.0f, 0.0f, float(shadowMapSize), float(shadowMapSize), 0.0f, 1.0f};
    VkRect2D shadowScissor{{0, 0}, {shadowMapSize, shadowMapSize}};
    vkCmdSetViewport(fd.command_buffer, 0, 1, &shadowViewport);
    vkCmdSetScissor(fd.command_buffer, 0, 1, &shadowScissor);
    vkCmdSetDepthBias(fd.command_buffer, lighting.depthBias, 0.0f,
                      lighting.depthBias);
    vkCmdBindPipeline(fd.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, shadowPipeline);
    VkBuffer buffers[] = {vertexBuffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(fd.command_buffer, 0, 1, buffers, offsets);
    vkCmdBindIndexBuffer(fd.command_buffer, indexBuffer, 0, VK_INDEX_TYPE_UINT16);
    for (uint32_t i = 0; i < objectCount; ++i) {
        const uint32_t dynamicOffset = static_cast<uint32_t>(uniformStride * i);
        vkCmdBindDescriptorSets(fd.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                pipelineLayout, 0, 1, &descriptorSet, 1, &dynamicOffset);
        vkCmdDrawIndexed(fd.command_buffer, indexCount, 1, 0, 0, 0);
    }
    const uint32_t floorOffset = static_cast<uint32_t>(uniformStride * maxObjectCount);
    vkCmdBindDescriptorSets(fd.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            pipelineLayout, 0, 1, &descriptorSet, 1, &floorOffset);
    vkCmdDrawIndexed(fd.command_buffer, floorIndexCount, 1, floorFirstIndex, 0, 0);
    vkCmdEndRendering(fd.command_buffer);

    shadowBarrier.oldLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    shadowBarrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
    shadowBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    shadowBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    vkCmdPipelineBarrier(fd.command_buffer,
                         VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
                         VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                         VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                         0, nullptr, 0, nullptr, 1, &shadowBarrier);
    shadowMapInitialized = true;

    VkClearValue clearValues[2]{};
    clearValues[0].color = {{ 0.1f, 0.1f, 0.12f, 1.0f }};
    clearValues[1].depthStencil = { 1.0f, 0 };

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = context.render_pass;
    renderPassInfo.framebuffer = fd.framebuffer;
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = context.swapchain_extent;
    renderPassInfo.clearValueCount = 2;
    renderPassInfo.pClearValues = clearValues;

    vkCmdBeginRenderPass(fd.command_buffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(context.swapchain_extent.width);
    viewport.height = static_cast<float>(context.swapchain_extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(fd.command_buffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = context.swapchain_extent;
    vkCmdSetScissor(fd.command_buffer, 0, 1, &scissor);

    vkCmdBindPipeline(fd.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

    vkCmdBindVertexBuffers(fd.command_buffer, 0, 1, buffers, offsets);
    vkCmdBindIndexBuffer(fd.command_buffer, indexBuffer, 0, VK_INDEX_TYPE_UINT16);

    for (uint32_t i = 0; i < objectCount; ++i) {
        const uint32_t dynamicOffset = static_cast<uint32_t>(uniformStride * i);
        vkCmdBindDescriptorSets(fd.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                pipelineLayout, 0, 1, &descriptorSet,
                                1, &dynamicOffset);
        vkCmdDrawIndexed(fd.command_buffer, indexCount, 1, 0, 0, 0);
    }
    vkCmdBindDescriptorSets(fd.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            pipelineLayout, 0, 1, &descriptorSet, 1, &floorOffset);
    vkCmdDrawIndexed(fd.command_buffer, floorIndexCount, 1, floorFirstIndex, 0, 0);

    vkCmdEndRenderPass(fd.command_buffer);
    vkEndCommandBuffer(fd.command_buffer);
}

} // namespace application
