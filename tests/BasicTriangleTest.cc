#include "BasicTriangleTest.h"

namespace cc {

void BasicTriangle::onDestroy() {
    CC_SAFE_DESTROY(_vertexBuffer);
    CC_SAFE_DESTROY(_inputAssembler);
    CC_SAFE_DESTROY(_uniformBuffer);
    CC_SAFE_DESTROY(_uniformBufferMVP);
    CC_SAFE_DESTROY(_shader);
    CC_SAFE_DESTROY(_descriptorSet);
    CC_SAFE_DESTROY(_descriptorSetLayout);
    CC_SAFE_DESTROY(_pipelineLayout);
    CC_SAFE_DESTROY(_pipelineState);
    CC_SAFE_DESTROY(_indexBuffer);
    CC_SAFE_DESTROY(_indirectBuffer);
}

bool BasicTriangle::onInit() {
    createShader();
    createVertexBuffer();
    createInputAssembler();
    createPipeline();

    return true;
}

void BasicTriangle::createShader() {

    ShaderSources<StandardShaderSource> sources;
    sources.glsl4 = {
        R"(
            precision highp float;
            layout(location = 0) in vec2 a_position;
            layout(set = 0, binding = 1) uniform MVP { mat4 u_MVP; };

            void main() {
                gl_Position = u_MVP * vec4(a_position, 0.0, 1.0);
            }
        )",
        R"(
            precision highp float;
            layout(set = 0, binding = 0) uniform Color {
                vec4 u_color;
            };
            layout(location = 0) out vec4 o_color;

            void main() {
                o_color = u_color;
            }
        )",
    };

    sources.glsl3 = {
        R"(
            in vec2 a_position;
            layout(std140) uniform MVP { mat4 u_MVP; };

            void main() {
                gl_Position = u_MVP * vec4(a_position, 0.0, 1.0);
            }
        )",
        R"(
            precision mediump float;
            layout(std140) uniform Color {
                vec4 u_color;
            };

            out vec4 o_color;
            void main() {
                o_color = u_color;
            }
        )",
    };

    sources.glsl1 = {
        R"(
            attribute vec2 a_position;
            uniform mat4 u_MVP;

            void main() {
                gl_Position = u_MVP * vec4(a_position, 0.0, 1.0);
            }
        )",
        R"(
            precision mediump float;
            uniform vec4 u_color;
            void main() {
                gl_FragColor = u_color;
            }
        )",
    };

    StandardShaderSource &source = TestBaseI::getAppropriateShaderSource(sources);

    gfx::ShaderStageList shaderStageList;
    gfx::ShaderStage     vertexShaderStage;
    vertexShaderStage.stage  = gfx::ShaderStageFlagBit::VERTEX;
    vertexShaderStage.source = source.vert;
    shaderStageList.emplace_back(std::move(vertexShaderStage));

    gfx::ShaderStage fragmentShaderStage;
    fragmentShaderStage.stage  = gfx::ShaderStageFlagBit::FRAGMENT;
    fragmentShaderStage.source = source.frag;
    shaderStageList.emplace_back(std::move(fragmentShaderStage));

    gfx::UniformBlockList uniformBlockList = {
        {0, 0, "Color", {{"u_color", gfx::Type::FLOAT4, 1}}, 1},
        {0, 1, "MVP", {{"u_MVP", gfx::Type::MAT4, 1}}, 1},
    };
    gfx::AttributeList attributeList = {{"a_position", gfx::Format::RG32F, false, 0, false, 0}};

    gfx::ShaderInfo shaderInfo;
    shaderInfo.name       = "Basic Triangle";
    shaderInfo.stages     = std::move(shaderStageList);
    shaderInfo.attributes = std::move(attributeList);
    shaderInfo.blocks     = std::move(uniformBlockList);
    _shader               = device->createShader(shaderInfo);
}

void BasicTriangle::createVertexBuffer() {
    float vertexData[] = {-0.5f, 0.5f,
                          -0.5f, -0.5f,
                          0.5f, -0.5f,
                          0.0f, 0.5f,
                          0.5f, 0.5f};

    gfx::BufferInfo vertexBufferInfo = {
        gfx::BufferUsage::VERTEX,
        gfx::MemoryUsage::DEVICE,
        sizeof(vertexData),
        2 * sizeof(float),
        gfx::BufferFlagBit::NONE,
    };

    _vertexBuffer = device->createBuffer(vertexBufferInfo);
    _vertexBuffer->update(vertexData, sizeof(vertexData));

    gfx::BufferInfo uniformBufferInfo = {
        gfx::BufferUsage::UNIFORM,
        gfx::MemoryUsage::DEVICE | gfx::MemoryUsage::HOST,
        TestBaseI::getUBOSize(sizeof(gfx::Color)),
    };
    _uniformBuffer = device->createBuffer(uniformBufferInfo);

    gfx::BufferInfo uniformBufferMVPInfo = {
        gfx::BufferUsage::UNIFORM,
        gfx::MemoryUsage::DEVICE | gfx::MemoryUsage::HOST,
        TestBaseI::getUBOSize(sizeof(Mat4)),
    };
    _uniformBufferMVP = device->createBuffer(uniformBufferMVPInfo);

    unsigned short  indices[]       = {1, 3, 0, 1, 2, 3, 2, 4, 3};
    gfx::BufferInfo indexBufferInfo = {
        gfx::BufferUsageBit::INDEX,
        gfx::MemoryUsage::DEVICE,
        sizeof(indices),
        sizeof(unsigned short),
    };
    _indexBuffer = device->createBuffer(indexBufferInfo);
    _indexBuffer->update(indices, sizeof(indices));

    gfx::DrawInfo drawInfo;
    drawInfo.firstIndex = 3;
    drawInfo.indexCount = 3;

    gfx::BufferInfo indirectBufferInfo = {
        gfx::BufferUsageBit::INDIRECT,
        gfx::MemoryUsage::DEVICE | gfx::MemoryUsage::HOST,
        sizeof(gfx::DrawInfo),
        sizeof(gfx::DrawInfo),
    };
    _indirectBuffer = device->createBuffer(indirectBufferInfo);
    _indirectBuffer->update(&drawInfo, sizeof(gfx::DrawInfo));
}

void BasicTriangle::createInputAssembler() {
    gfx::Attribute          position = {"a_position", gfx::Format::RG32F, false, 0, false};
    gfx::InputAssemblerInfo inputAssemblerInfo;
    inputAssemblerInfo.attributes.emplace_back(std::move(position));
    inputAssemblerInfo.vertexBuffers.emplace_back(_vertexBuffer);
    inputAssemblerInfo.indexBuffer    = _indexBuffer;
    inputAssemblerInfo.indirectBuffer = _indirectBuffer;
    _inputAssembler                   = device->createInputAssembler(inputAssemblerInfo);
}

void BasicTriangle::createPipeline() {
    gfx::DescriptorSetLayoutInfo dslInfo;
    dslInfo.bindings.push_back({0, gfx::DescriptorType::UNIFORM_BUFFER, 1, gfx::ShaderStageFlagBit::FRAGMENT});
    dslInfo.bindings.push_back({1, gfx::DescriptorType::UNIFORM_BUFFER, 1, gfx::ShaderStageFlagBit::VERTEX});
    _descriptorSetLayout = device->createDescriptorSetLayout(dslInfo);

    _pipelineLayout = device->createPipelineLayout({{_descriptorSetLayout}});

    _descriptorSet = device->createDescriptorSet({_descriptorSetLayout});

    _descriptorSet->bindBuffer(0, _uniformBuffer);
    _descriptorSet->bindBuffer(1, _uniformBufferMVP);
    _descriptorSet->update();

    gfx::PipelineStateInfo pipelineInfo;
    pipelineInfo.primitive      = gfx::PrimitiveMode::TRIANGLE_LIST;
    pipelineInfo.shader         = _shader;
    pipelineInfo.inputState     = {_inputAssembler->getAttributes()};
    pipelineInfo.renderPass     = fbo->getRenderPass();
    pipelineInfo.pipelineLayout = _pipelineLayout;

    _pipelineState = device->createPipelineState(pipelineInfo);

    _globalBarriers.push_back(TestBaseI::getGlobalBarrier({
        {
            gfx::AccessType::TRANSFER_WRITE,
        },
        {
            gfx::AccessType::VERTEX_SHADER_READ_UNIFORM_BUFFER,
            gfx::AccessType::FRAGMENT_SHADER_READ_UNIFORM_BUFFER,
            gfx::AccessType::INDIRECT_BUFFER,
            gfx::AccessType::VERTEX_BUFFER,
            gfx::AccessType::INDEX_BUFFER,
        },
    }));

    _globalBarriers.push_back(TestBaseI::getGlobalBarrier({
        {
            gfx::AccessType::TRANSFER_WRITE,
        },
        {
            gfx::AccessType::VERTEX_SHADER_READ_UNIFORM_BUFFER,
            gfx::AccessType::FRAGMENT_SHADER_READ_UNIFORM_BUFFER,
        },
    }));
}

void BasicTriangle::onTick() {
    uint globalBarrierIdx = _frameCount ? 1 : 0;

    gfx::Color clearColor = {1.0f, 0, 0, 1.0f};

    gfx::Color uniformColor;
    uniformColor.x = std::abs(std::sin(_time));
    uniformColor.y = 1.0f;
    uniformColor.z = 0.0f;
    uniformColor.w = 1.0f;

    Mat4 MVP;
    TestBaseI::createOrthographic(-1, 1, -1, 1, -1, 1, &MVP);

    device->acquire();

    _uniformBuffer->update(&uniformColor, sizeof(uniformColor));
    _uniformBufferMVP->update(MVP.m, sizeof(Mat4));

    gfx::Rect renderArea = {0, 0, device->getWidth(), device->getHeight()};

    auto commandBuffer = commandBuffers[0];
    commandBuffer->begin();

    if (TestBaseI::MANUAL_BARRIER)
        commandBuffer->pipelineBarrier(_globalBarriers[globalBarrierIdx]);

    commandBuffer->beginRenderPass(fbo->getRenderPass(), fbo, renderArea, &clearColor, 1.0f, 0);
    commandBuffer->bindInputAssembler(_inputAssembler);
    commandBuffer->bindPipelineState(_pipelineState);
    commandBuffer->bindDescriptorSet(0, _descriptorSet);
    commandBuffer->draw(_inputAssembler);
    commandBuffer->endRenderPass();
    commandBuffer->end();

    device->flushCommands(commandBuffers);
    device->getQueue()->submit(commandBuffers);
    device->present();
}

} // namespace cc
