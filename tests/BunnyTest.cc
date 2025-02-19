#include "BunnyTest.h"
#include "BunnyData.h"

namespace cc {

namespace {
enum class Binding : uint8_t { MVP,
                               COLOR };
}

void BunnyTest::onDestroy() {
    CC_SAFE_DESTROY(_shader);
    CC_SAFE_DESTROY(_vertexBuffer);
    CC_SAFE_DESTROY(_indexBuffer);
    CC_SAFE_DESTROY(_mvpMatrix);
    CC_SAFE_DESTROY(_color);
    CC_SAFE_DESTROY(_rootUBO);
    CC_SAFE_DESTROY(_inputAssembler);
    CC_SAFE_DESTROY(_descriptorSet);
    CC_SAFE_DESTROY(_descriptorSetLayout);
    CC_SAFE_DESTROY(_pipelineLayout);
    CC_SAFE_DESTROY(_pipelineState);
}

bool BunnyTest::onInit() {
    createShader();
    createBuffers();
    createInputAssembler();
    createPipelineState();
    return true;
}

void BunnyTest::createShader() {

    ShaderSources<StandardShaderSource> sources;
    sources.glsl4 = {
        R"(
            precision highp float;
            layout(location = 0) in vec3 a_position;

            layout(set = 0, binding = 0) uniform MVP_Matrix {
                mat4 u_model, u_view, u_projection;
            };

            layout(location = 0) out vec3 v_position;

            void main () {
                vec4 pos = u_projection * u_view * u_model * vec4(a_position, 1);
                v_position = a_position.xyz;
                gl_Position = pos;
            }
        )",
        R"(
            precision highp float;
            layout(set = 0, binding = 1) uniform Color {
                vec4 u_color;
            };
            layout(location = 0) in vec3 v_position;
            layout(location = 0) out vec4 o_color;
            void main () {
                o_color = u_color * vec4(v_position, 1);
            }
        )",
    };

    sources.glsl3 = {
        R"(
            in vec3 a_position;

            layout(std140) uniform MVP_Matrix {
                mat4 u_model, u_view, u_projection;
            };

            out vec3 v_position;

            void main () {
                vec4 pos = u_projection * u_view * u_model * vec4(a_position, 1);
                v_position = a_position.xyz;
                gl_Position = pos;
            }
        )",
        R"(
            precision mediump float;
            layout(std140) uniform Color {
                vec4 u_color;
            };
            in vec3 v_position;
            out vec4 o_color;
            void main () {
                o_color = u_color * vec4(v_position, 1);
            }
        )",
    };

    sources.glsl1 = {
        R"(
            attribute vec3 a_position;
            uniform mat4 u_model, u_view, u_projection;
            varying vec3 v_position;

            void main () {
                vec4 pos = u_projection * u_view * u_model * vec4(a_position, 1);
                v_position = a_position.xyz;
                gl_Position = pos;
            }
        )",
        R"(
            precision mediump float;
            uniform vec4 u_color;
            varying vec3 v_position;

            void main () {
                gl_FragColor = u_color * vec4(v_position, 1);
            }
        )",
    };

    StandardShaderSource &source = TestBaseI::getAppropriateShaderSource(sources);

    gfx::ShaderStageList shaderStageList;
    gfx::ShaderStage     vertexShaderStage;
    vertexShaderStage.stage  = gfx::ShaderStageFlagBit::VERTEX;
    vertexShaderStage.source = source.vert;
    shaderStageList.emplace_back(std::move(vertexShaderStage));

    // fragment shader
    gfx::ShaderStage fragmentShaderStage;
    fragmentShaderStage.stage  = gfx::ShaderStageFlagBit::FRAGMENT;
    fragmentShaderStage.source = source.frag;
    shaderStageList.emplace_back(std::move(fragmentShaderStage));

    gfx::AttributeList attributeList = {{"a_position", gfx::Format::RGB32F, false, 0, false, 0}};
    gfx::UniformList   mvpMatrix     = {
        {"u_model", gfx::Type::MAT4, 1},
        {"u_view", gfx::Type::MAT4, 1},
        {"u_projection", gfx::Type::MAT4, 1},
    };
    gfx::UniformList      color            = {{"u_color", gfx::Type::FLOAT4, 1}};
    gfx::UniformBlockList uniformBlockList = {
        {0, static_cast<uint>(Binding::MVP), "MVP_Matrix", mvpMatrix, 1},
        {0, static_cast<uint>(Binding::COLOR), "Color", color, 1},
    };

    gfx::ShaderInfo shaderInfo;
    shaderInfo.name       = "Bunny Test";
    shaderInfo.stages     = std::move(shaderStageList);
    shaderInfo.attributes = std::move(attributeList);
    shaderInfo.blocks     = std::move(uniformBlockList);
    _shader               = device->createShader(shaderInfo);
}

void BunnyTest::createBuffers() {
    // vertex buffer
    _vertexBuffer = device->createBuffer({
        gfx::BufferUsage::VERTEX,
        gfx::MemoryUsage::DEVICE,
        sizeof(bunny_positions),
        3 * sizeof(float),
    });
    _vertexBuffer->update((void *)&bunny_positions[0][0], sizeof(bunny_positions));

    // index buffer
    _indexBuffer = device->createBuffer({
        gfx::BufferUsage::INDEX,
        gfx::MemoryUsage::DEVICE,
        sizeof(bunny_cells),
        sizeof(uint16_t),
    });
    _indexBuffer->update((void *)&bunny_cells[0], sizeof(bunny_cells));

    // root UBO
    uint offset = TestBaseI::getAlignedUBOStride(3 * sizeof(Mat4));
    uint size   = offset + 4 * sizeof(float);
    _rootUBO    = device->createBuffer({
        gfx::BufferUsage::UNIFORM,
        gfx::MemoryUsage::DEVICE | gfx::MemoryUsage::HOST,
        TestBaseI::getUBOSize(size),
    });
    _rootBuffer.resize(size / sizeof(float));

    // mvp matrix uniform
    _mvpMatrix = device->createBuffer({
        _rootUBO,
        0,
        3 * sizeof(Mat4),
    });
    // color uniform
    _color = device->createBuffer({
        _rootUBO,
        offset,
        4 * sizeof(float),
    });

    Mat4 model;
    std::copy(model.m, model.m + 16, &_rootBuffer[0]);

    float color[4] = {0.5f, 0.5f, 0.5f, 1.0f};
    std::copy(color, color + 4, &_rootBuffer[offset / sizeof(float)]);
}

void BunnyTest::createInputAssembler() {
    gfx::Attribute          position = {"a_position", gfx::Format::RGB32F, false, 0, false};
    gfx::InputAssemblerInfo inputAssemblerInfo;
    inputAssemblerInfo.attributes.emplace_back(std::move(position));
    inputAssemblerInfo.vertexBuffers.emplace_back(_vertexBuffer);
    inputAssemblerInfo.indexBuffer = _indexBuffer;
    _inputAssembler                = device->createInputAssembler(inputAssemblerInfo);
}

void BunnyTest::createPipelineState() {
    gfx::DescriptorSetLayoutInfo dslInfo;
    dslInfo.bindings.push_back({0, gfx::DescriptorType::UNIFORM_BUFFER, 1, gfx::ShaderStageFlagBit::VERTEX});
    dslInfo.bindings.push_back({1, gfx::DescriptorType::UNIFORM_BUFFER, 1, gfx::ShaderStageFlagBit::FRAGMENT});
    _descriptorSetLayout = device->createDescriptorSetLayout(dslInfo);

    _pipelineLayout = device->createPipelineLayout({{_descriptorSetLayout}});

    _descriptorSet = device->createDescriptorSet({_descriptorSetLayout});

    _descriptorSet->bindBuffer(static_cast<uint>(Binding::MVP), _mvpMatrix);
    _descriptorSet->bindBuffer(static_cast<uint>(Binding::COLOR), _color);
    _descriptorSet->update();

    gfx::PipelineStateInfo pipelineStateInfo;
    pipelineStateInfo.primitive                    = gfx::PrimitiveMode::TRIANGLE_LIST;
    pipelineStateInfo.shader                       = _shader;
    pipelineStateInfo.inputState                   = {_inputAssembler->getAttributes()};
    pipelineStateInfo.renderPass                   = fbo->getRenderPass();
    pipelineStateInfo.depthStencilState.depthTest  = true;
    pipelineStateInfo.depthStencilState.depthWrite = true;
    pipelineStateInfo.depthStencilState.depthFunc  = gfx::ComparisonFunc::LESS;
    pipelineStateInfo.pipelineLayout               = _pipelineLayout;
    _pipelineState                                 = device->createPipelineState(pipelineStateInfo);

    _globalBarriers.push_back(TestBaseI::getGlobalBarrier({
        {
            gfx::AccessType::TRANSFER_WRITE,
        },
        {
            gfx::AccessType::VERTEX_SHADER_READ_UNIFORM_BUFFER,
            gfx::AccessType::FRAGMENT_SHADER_READ_UNIFORM_BUFFER,
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

    _textureBarriers.push_back(TestBaseI::getTextureBarrier({
        {
            gfx::AccessType::TRANSFER_WRITE,
        },
        {
            gfx::AccessType::FRAGMENT_SHADER_READ_TEXTURE,
        },
        false,
    }));
}

void BunnyTest::onTick() {
    uint globalBarrierIdx = _frameCount ? 1 : 0;

    Mat4::createLookAt(Vec3(30.0f * std::cos(_time), 20.0f, 30.0f * std::sin(_time)),
                       Vec3(0.0f, 2.5f, 0.0f), Vec3(0.0f, 1.0f, 0.f), &_view);
    std::copy(_view.m, _view.m + 16, &_rootBuffer[16]);

    Mat4        projection;
    gfx::Extent orientedSize = TestBaseI::getOrientedSurfaceSize();
    TestBaseI::createPerspective(60.0f, 1.0f * orientedSize.width / orientedSize.height, 0.01f, 1000.0f, &projection);
    std::copy(projection.m, projection.m + 16, &_rootBuffer[32]);

    gfx::Color clearColor = {0.0f, 0, 0, 1.0f};

    device->acquire();

    _rootUBO->update(_rootBuffer.data(), _rootBuffer.size() * sizeof(float));
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
