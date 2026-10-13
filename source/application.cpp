#include "application.hpp"

#include <imgui.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <fstream>
#include <vector>
#include <iostream>
#include <cstring>

namespace application {

struct Vertex {
    glm::vec3 pos;
    glm::vec3 color;
};

struct PushConstants {
    glm::mat4 mvp;
    glm::vec3 color;
};

std::vector<Vertex> vertices;
std::vector<uint16_t> indices;

void generateCylinder(float radius, float height, int segments) {
    vertices.clear();
    indices.clear();

    float halfHeight = height / 2.0f;

    for (int i = 0; i <= segments; ++i) {
        float theta = i * 2.0f * 3.14159265359f / segments;
        float x = radius * std::cos(theta);
        float z = radius * std::sin(theta);

        float u = static_cast<float>(i) / segments;

        Vertex vTop;
        vTop.pos = glm::vec3(x, halfHeight, z);
        vTop.color = glm::vec3(u, 1.0f, 0.5f + 0.5f * std::sin(theta));
        vertices.push_back(vTop);

        Vertex vBottom;
        vBottom.pos = glm::vec3(x, -halfHeight, z);
        vBottom.color = glm::vec3(u, 0.2f, 0.5f + 0.5f * std::cos(theta));
        vertices.push_back(vBottom);
    }

    for (int i = 0; i < segments; ++i) {
        uint32_t top1 = i * 2;
        uint32_t bottom1 = i * 2 + 1;
        uint32_t top2 = (i + 1) * 2;
        uint32_t bottom2 = (i + 1) * 2 + 1;

        indices.push_back(top1);
        indices.push_back(bottom1);
        indices.push_back(top2);

        indices.push_back(top2);
        indices.push_back(bottom1);
        indices.push_back(bottom2);
    }

    uint32_t topCenterIndex = static_cast<uint32_t>(vertices.size());
    Vertex topCenter;
    topCenter.pos = glm::vec3(0.0f, halfHeight, 0.0f);
    topCenter.color = glm::vec3(1.0f, 1.0f, 1.0f);
    vertices.push_back(topCenter);

    for (int i = 0; i < segments; ++i) {
        uint32_t currentTop = i * 2;
        uint32_t nextTop = (i + 1) * 2;

        indices.push_back(topCenterIndex);
        indices.push_back(nextTop);
        indices.push_back(currentTop);
    }

    uint32_t bottomCenterIndex = static_cast<uint32_t>(vertices.size());
    Vertex bottomCenter;
    bottomCenter.pos = glm::vec3(0.0f, -halfHeight, 0.0f);
    bottomCenter.color = glm::vec3(0.2f, 0.2f, 0.2f);
    vertices.push_back(bottomCenter);

    for (int i = 0; i < segments; ++i) {
        uint32_t currentBottom = i * 2 + 1;
        uint32_t nextBottom = (i + 1) * 2 + 1;

        indices.push_back(bottomCenterIndex);
        indices.push_back(currentBottom);
        indices.push_back(nextBottom);
    }
}

struct SceneState {
    glm::vec3 translation = glm::vec3(0.0f);
    glm::vec3 rotation = glm::vec3(0.0f);
    glm::vec3 scale = glm::vec3(1.0f);

    glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 3.0f);

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

static VkBuffer vertexBuffer = VK_NULL_HANDLE;
static VmaAllocation vertexAllocation = VK_NULL_HANDLE;

static VkBuffer indexBuffer = VK_NULL_HANDLE;
static VmaAllocation indexAllocation = VK_NULL_HANDLE;

static VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
static VkPipeline graphicsPipeline     = VK_NULL_HANDLE;

static glm::mat4 currentMVP = glm::mat4(1.0f);

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

bool initialize() {
    graphics::internal::resize(1280, 720);
    auto& context = graphics::internal::context;

    generateCylinder(0.8f, 1.5f, 50);

    if (vertices.empty() || indices.empty()) {
        std::cerr << "Vertices or indices vector is empty!\n";
        return false;
    }

    VkDeviceSize vertexSize = sizeof(vertices[0]) * vertices.size();
    if (!createBuffer(vertexSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VMA_MEMORY_USAGE_AUTO, vertexBuffer, vertexAllocation)) {
        std::cerr << "Failed to create vertex buffer!\n";
        return false;
    }
    void* mappedData = nullptr;
    vmaMapMemory(context.allocator, vertexAllocation, &mappedData);
    std::memcpy(mappedData, vertices.data(), vertexSize);
    vmaUnmapMemory(context.allocator, vertexAllocation);

    VkDeviceSize indexSize = sizeof(indices[0]) * indices.size();
    if (!createBuffer(indexSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VMA_MEMORY_USAGE_AUTO, indexBuffer, indexAllocation)) {
        std::cerr << "Failed to create index buffer!\n";
        return false;
    }
    vmaMapMemory(context.allocator, indexAllocation, &mappedData);
    std::memcpy(mappedData, indices.data(), indexSize);
    vmaUnmapMemory(context.allocator, indexAllocation);

    auto vertCode = readFile("shaders/shader.vert.spv");
    auto fragCode = readFile("shaders/shader.frag.spv");

    VkShaderModule vertModule = createShaderModule(context.device, vertCode);
    VkShaderModule fragModule = createShaderModule(context.device, fragCode);

    if (vertModule == VK_NULL_HANDLE || fragModule == VK_NULL_HANDLE) {
        return false;
    }

    VkPipelineShaderStageCreateInfo vertStageInfo{};
    vertStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertStageInfo.module = vertModule;
    vertStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragStageInfo{};
    fragStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragStageInfo.module = fragModule;
    fragStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = { vertStageInfo, fragStageInfo };

    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(Vertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    std::vector<VkVertexInputAttributeDescription> attributeDescriptions(2);
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(Vertex, pos);

    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(Vertex, color);

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    std::vector<VkDynamicState> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_NONE;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(PushConstants);

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

    if (vkCreatePipelineLayout(context.device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
        std::cerr << "Failed to create pipeline layout!\n";
        return false;
    }

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
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

    if (vkCreateGraphicsPipelines(context.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
        std::cerr << "Failed to create graphics pipeline!\n";
        return false;
    }

    vkDestroyShaderModule(context.device, fragModule, nullptr);
    vkDestroyShaderModule(context.device, vertModule, nullptr);

    return true;
}

void shutdown() {
    auto& context = graphics::internal::context;
    vkQueueWaitIdle(context.graphics_queue);

    if (graphicsPipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(context.device, graphicsPipeline, nullptr);
        graphicsPipeline = VK_NULL_HANDLE;
    }

    if (pipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(context.device, pipelineLayout, nullptr);
        pipelineLayout = VK_NULL_HANDLE;
    }

    if (vertexBuffer != VK_NULL_HANDLE) {
        vmaDestroyBuffer(context.allocator, vertexBuffer, vertexAllocation);
        vertexBuffer = VK_NULL_HANDLE;
    }

    if (indexBuffer != VK_NULL_HANDLE) {
        vmaDestroyBuffer(context.allocator, indexBuffer, indexAllocation);
        indexBuffer = VK_NULL_HANDLE;
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
    }

    ImGui::End();

    const float deltaTime =
        glm::clamp(static_cast<float>(time), 0.0f, 0.1f);

    if (state.isPlaying) {
        state.animationTime += deltaTime * state.speed;
    }

    glm::vec3 animatedTranslation(0.0f);
    glm::vec3 animatedRotation(0.0f);

    if (state.isPlaying) {
        animatedTranslation.x =
            state.radius * std::cos(state.animationTime);

        animatedTranslation.z =
            state.radius * std::sin(state.animationTime);

        animatedTranslation.y =
            std::sin(state.animationTime * 2.0f) * 0.5f;

        animatedRotation.y =
            glm::degrees(-state.animationTime);
    }

    const glm::vec3 finalTranslation =
        state.translation + animatedTranslation;

    const glm::vec3 finalRotation =
        state.rotation + animatedRotation;

    // Матрица модели
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

    // Матрица вида — положение камеры
    glm::mat4 view = glm::lookAt(
        state.cameraPos,
        glm::vec3(0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f));

    // Матрица проекции
    auto& context = graphics::internal::context;

    float aspect =
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
        float h = state.orthoSize;
        float w = h * aspect;

        proj = glm::orthoRH_ZO(
            -w, w,
            -h, h,
            state.nearPlane,
            state.farPlane
        );
    }

    proj[1][1] *= -1.0f;

    currentMVP = proj * view * model;
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

    PushConstants constants{ currentMVP, state.objectColor };
    vkCmdPushConstants(
        fd.command_buffer,
        pipelineLayout,
        VK_SHADER_STAGE_VERTEX_BIT,
        0,
        sizeof(PushConstants),
        &constants
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