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
#include <cstddef>

namespace application {

struct Vertex {
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec3 normal;
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
            {u, 1.0f, 0.5f + 0.5f * std::sin(theta)},
            normal
        });

        vertices.push_back({
            {x, -halfHeight, z},
            {u, 0.2f, 0.5f + 0.5f * std::cos(theta)},
            normal
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
        {1.0f, 1.0f, 1.0f},
        {0.0f, 1.0f, 0.0f}
    });

    const uint16_t topRing =
        static_cast<uint16_t>(vertices.size());

    for (int i = 0; i <= segments; ++i) {
        const float theta =
            static_cast<float>(i) * 2.0f * pi /
            static_cast<float>(segments);

        const float x = radius * std::cos(theta);
        const float z = radius * std::sin(theta);
        const float u =
            static_cast<float>(i) / static_cast<float>(segments);

        vertices.push_back({
            {x, halfHeight, z},
            {u, 1.0f, 0.5f + 0.5f * std::sin(theta)},
            {0.0f, 1.0f, 0.0f}
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
        {0.2f, 0.2f, 0.2f},
        {0.0f, -1.0f, 0.0f}
    });

    const uint16_t bottomRing =
        static_cast<uint16_t>(vertices.size());

    for (int i = 0; i <= segments; ++i) {
        const float theta =
            static_cast<float>(i) * 2.0f * pi /
            static_cast<float>(segments);

        const float x = radius * std::cos(theta);
        const float z = radius * std::sin(theta);
        const float u =
            static_cast<float>(i) / static_cast<float>(segments);

        vertices.push_back({
            {x, -halfHeight, z},
            {u, 0.2f, 0.5f + 0.5f * std::cos(theta)},
            {0.0f, -1.0f, 0.0f}
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

    glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 4.0f);
    // Углы камеры в градусах: yaw, pitch, roll.
    glm::vec3 cameraRotation = glm::vec3(0.0f);

    bool isPerspective = true;
    float fov = 45.0f;
    float orthoSize = 2.0f;
    float nearPlane = 0.1f;
    float farPlane = 100.0f;
    bool isPlaying = true;
    float animationTime = 0.0f;
    float speed = 1.0f;
    float radius = 2.0f;
    glm::vec3 objectColor = glm::vec3(1.0f, 0.5f, 0.2f);
} state;

struct LightingState {
    glm::vec3 direction = glm::vec3(-1.0f, -1.0f, -1.0f);
    glm::vec3 lightColor = glm::vec3(1.0f);
    float lightIntensity = 1.0f;

    // Фоновое освещение.
    glm::vec3 ambientColor = glm::vec3(1.0f);
    float ambientIntensity = 0.15f;

};

LightingState lighting;

struct MaterialState {
    glm::vec3 specularColor{1.0f};
    float shininess = 32.0f;
} material;

struct alignas(16) Uniforms {
    glm::mat4 model{1.0f};
    glm::mat4 viewProjection{1.0f};
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

    glm::vec4 objectColor{1.0f, 0.5f, 0.2f, 1.0f};
    glm::vec4 specularColorShininess{1.0f, 1.0f, 1.0f, 32.0f};
};

static_assert(sizeof(Uniforms) % 16 == 0);


static VkBuffer vertexBuffer = VK_NULL_HANDLE;
static VmaAllocation vertexAllocation = VK_NULL_HANDLE;

static VkBuffer indexBuffer = VK_NULL_HANDLE;
static VmaAllocation indexAllocation = VK_NULL_HANDLE;

static VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
static VkPipeline graphicsPipeline     = VK_NULL_HANDLE;


struct MappedBuffer {
    VkBuffer buffer = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;
    void* mapped = nullptr;
};

static MappedBuffer uniformBuffer;

static VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
static VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
static VkDescriptorSet descriptorSet = VK_NULL_HANDLE;

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
                              VkDeviceSize size) {
    auto& context = graphics::internal::context;
    std::memcpy(target.mapped, data, static_cast<size_t>(size));
    vmaFlushAllocation(context.allocator, target.allocation, 0, size);
}

static void destroyMappedBuffer(MappedBuffer& target) {
    auto& context = graphics::internal::context;
    if (target.mapped) vmaUnmapMemory(context.allocator, target.allocation);
    if (target.buffer != VK_NULL_HANDLE)
        vmaDestroyBuffer(context.allocator, target.buffer, target.allocation);
    target = {};
}

static bool createDescriptorSet() {
    auto& context = graphics::internal::context;
    const VkDescriptorSetLayoutBinding binding = {
        0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1,
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, nullptr
    };
    VkDescriptorSetLayoutCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    info.bindingCount = 1;
    info.pBindings = &binding;
    if (vkCreateDescriptorSetLayout(context.device, &info, nullptr,
                                    &descriptorSetLayout) != VK_SUCCESS) return false;

    const VkDescriptorPoolSize poolSize = {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1};
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.maxSets = 1;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
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
    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = descriptorSet;
    write.dstBinding = 0;
    write.descriptorCount = 1;
    write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    write.pBufferInfo = &bufferInfo;
    vkUpdateDescriptorSets(context.device, 1, &write, 0, nullptr);
    return true;
}

bool initialize() {
    graphics::internal::resize(1280, 720);
    auto& context = graphics::internal::context;

    generateCylinder(0.8f, 1.5f, 50);

    if (vertices.empty() || indices.empty()) {
        std::cerr << "Vertices or indices vector is empty!\n";
        return false;
    }

    const VkDeviceSize vertexSize =
        sizeof(vertices[0]) * vertices.size();

    if (!createBuffer(
            vertexSize,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VMA_MEMORY_USAGE_AUTO,
            vertexBuffer,
            vertexAllocation)) {
        std::cerr << "Failed to create vertex buffer!\n";
        return false;
    }

    void* mappedData = nullptr;

    if (vmaMapMemory(
            context.allocator,
            vertexAllocation,
            &mappedData) != VK_SUCCESS) {
        std::cerr << "Failed to map vertex buffer memory!\n";
        return false;
    }

    std::memcpy(mappedData, vertices.data(), vertexSize);
    vmaUnmapMemory(context.allocator, vertexAllocation);

    const VkDeviceSize indexSize =
        sizeof(indices[0]) * indices.size();

    if (!createBuffer(
            indexSize,
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            VMA_MEMORY_USAGE_AUTO,
            indexBuffer,
            indexAllocation)) {
        std::cerr << "Failed to create index buffer!\n";
        return false;
    }

    if (vmaMapMemory(
            context.allocator,
            indexAllocation,
            &mappedData) != VK_SUCCESS) {
        std::cerr << "Failed to map index buffer memory!\n";
        return false;
    }

    std::memcpy(mappedData, indices.data(), indexSize);
    vmaUnmapMemory(context.allocator, indexAllocation);

    if (!createMappedBuffer(sizeof(Uniforms), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                            uniformBuffer) ||
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

    // формат вершин: position, color, normal
    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(Vertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    std::vector<VkVertexInputAttributeDescription>
        attributeDescriptions(3);

    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format =
        VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(Vertex, pos);

    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format =
        VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(Vertex, color);

    attributeDescriptions[2].binding = 0;
    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].format =
        VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[2].offset = offsetof(Vertex, normal);

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
        ImGui::ColorEdit3(
            "Object Color",
            glm::value_ptr(state.objectColor));

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

    }

    ImGui::End();

    const float deltaTime =
        glm::clamp(static_cast<float>(time), 0.0f, 0.1f);

    if (state.isPlaying) {
        state.animationTime += deltaTime * state.speed;
    }

    glm::vec3 animatedTranslation(
        state.radius * std::cos(state.animationTime),
        std::sin(state.animationTime * 2.0f) * 0.5f,
        state.radius * std::sin(state.animationTime));
    glm::vec3 animatedRotation(0.0f, glm::degrees(-state.animationTime), 0.0f);

    const glm::vec3 finalTranslation =
        state.translation + animatedTranslation;

    const glm::vec3 finalRotation =
        state.rotation + animatedRotation;

    // Матрица модели.
    glm::mat4 model(1.0f);

    model = glm::translate(model, finalTranslation);

    model = glm::rotate(
        model,
        glm::radians(finalRotation.x),
        glm::vec3(1.0f, 0.0f, 0.0f));

    model = glm::rotate(
        model,
        glm::radians(finalRotation.y),
        glm::vec3(0.0f, 1.0f, 0.0f));

    model = glm::rotate(
        model,
        glm::radians(finalRotation.z),
        glm::vec3(0.0f, 0.0f, 1.0f));

    model = glm::scale(model, state.scale);

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
    uniforms.model = model;
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
    uniforms.objectColor = glm::vec4(state.objectColor, 1.0f);
    uniforms.specularColorShininess =
        glm::vec4(material.specularColor, material.shininess);

    writeMappedBuffer(uniformBuffer, &uniforms, sizeof(uniforms));
}


void render(const graphics::internal::FrameData& fd) {
    if (graphicsPipeline == VK_NULL_HANDLE || fd.command_buffer == VK_NULL_HANDLE || fd.framebuffer == VK_NULL_HANDLE) {
        return;
    }

    auto& context = graphics::internal::context;

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(fd.command_buffer, &beginInfo);

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

    vkCmdBindDescriptorSets(
        fd.command_buffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        pipelineLayout,
        0,
        1,
        &descriptorSet,
        0,
        nullptr
    );

    VkBuffer buffers[] = { vertexBuffer };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(fd.command_buffer, 0, 1, buffers, offsets);
    vkCmdBindIndexBuffer(fd.command_buffer, indexBuffer, 0, VK_INDEX_TYPE_UINT16);

    vkCmdDrawIndexed(fd.command_buffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);

    vkCmdEndRenderPass(fd.command_buffer);
    vkEndCommandBuffer(fd.command_buffer);
}

} // namespace application
