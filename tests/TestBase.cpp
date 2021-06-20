#include "TestBase.h"

#include "gfx-base/GFXDef-common.h"
#include "tests/BasicTextureTest.h"
#include "tests/BasicTriangleTest.h"
#include "tests/BlendTest.h"
#include "tests/ClearScreenTest.h"
#include "tests/ComputeTest.h"
#include "tests/DepthTest.h"
#include "tests/FrameGraphTest.h"
#include "tests/ParticleTest.h"
#include "tests/ScriptTest.h"
#include "tests/StencilTest.h"
#include "tests/StressTest.h"
#include "tests/SubpassTest.h"
#include "tests/DeferredTest.h"

#include "cocos/base/AutoreleasePool.h"
#include "cocos/bindings/auto/jsb_gfx_auto.h"
#include "cocos/bindings/dop/jsb_dop.h"
#include "cocos/bindings/event/CustomEventTypes.h"
#include "cocos/bindings/event/EventDispatcher.h"
#include "cocos/bindings/jswrapper/SeApi.h"
#include "cocos/bindings/manual/jsb_classtype.h"
#include "cocos/bindings/manual/jsb_gfx_manual.h"
#include "cocos/bindings/manual/jsb_global_init.h"

#include "cocos/renderer/gfx-gles3/gles3context.h"

#include "common/xr_linear.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#define DEFAULT_MATRIX_MATH


#define XR_USE_PLATFORM_ANDROID // Force use Android platform

#include "EGL/egl.h" // For OpenXR
#define XR_USE_GRAPHICS_API_OPENGL_ES // USE GLES

#ifdef XR_USE_PLATFORM_ANDROID
#define XR_KHR_LOADER_INIT_SUPPORT
#endif

//#define XR_EXTENSION_PROTOTYPES // JAMIE

#include "android/jni/JniHelper.h"

#include "openxr/openxr.h"
#include "openxr/openxr_platform.h"
#include "openxr/openxr_platform_defines.h"

#undef CC_USE_VULKAN
#undef CC_USE_GLES3
//#undef CC_USE_GLES2
#include "renderer/GFXDeviceManager.h"

#include <android_native_app_glue.h>
#include <jni.h>
bool g_sessionRunning{false};



namespace cc {

WindowInfo TestBaseI::windowInfo;

int               TestBaseI::curTestIndex           = -1;
int               TestBaseI::nextDirection          = 0;
TestBaseI *       TestBaseI::test                   = nullptr;
const bool        TestBaseI::MANUAL_BARRIER         = true;
const uint        TestBaseI::NANOSECONDS_PER_SECOND = 1000000000;
gfx::Device *     TestBaseI::device                 = nullptr;
gfx::Framebuffer *TestBaseI::fbo                    = nullptr;
gfx::RenderPass * TestBaseI::renderPass             = nullptr;

std::vector<TestBaseI::createFunc> TestBaseI::tests = {
//    SubpassTest::create,
//    DeferredTest::create,
 //   ComputeTest::create,
 //   ScriptTest::create,
 //   FrameGraphTest::create,
 //   StressTest::create,
 //   ClearScreen::create,
 //   BasicTriangle::create,
    DepthTexture::create,
 //   BlendTest::create,
 //   ParticleTest::create,
// Need to fix lib jpeg on iOS
#if CC_PLATFORM != CC_PLATFORM_MAC_IOS
 //   BasicTexture::create,
 //   StencilTest::create,
#endif // CC_PLATFORM != CC_PLATFORM_MAC_IOS
};

FrameRate                                       TestBaseI::hostThread;
FrameRate                                       TestBaseI::deviceThread;
std::vector<gfx::CommandBuffer *>               TestBaseI::commandBuffers;
std::unordered_map<uint, gfx::GlobalBarrier *>  TestBaseI::globalBarrierMap;
std::unordered_map<uint, gfx::TextureBarrier *> TestBaseI::textureBarrierMap;

framegraph::FrameGraph TestBaseI::fg;
framegraph::Texture    TestBaseI::fgBackBuffer;
framegraph::Texture    TestBaseI::fgDepthStencilBackBuffer;

#include <strings.h>
#include <android/log.h>

#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

    std::vector<XrSpace> g_visualizedSpaces;
    XrSession g_session{XR_NULL_HANDLE};
    XrSystemId g_sysId{XR_NULL_SYSTEM_ID};
    XrInstance g_inst{XR_NULL_HANDLE};
    XrViewConfigurationType g_viewConfigType{XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO};
    XrGraphicsBindingOpenGLESAndroidKHR g_graphicsBinding{XR_TYPE_GRAPHICS_BINDING_OPENGL_ES_ANDROID_KHR};
    XrSpace g_appSpace{XR_NULL_HANDLE};
    XrEventDataBuffer g_eventDataBuffer{};
    XrSessionState g_sessionState{XR_SESSION_STATE_UNKNOWN};

    std::vector<XrViewConfigurationView> g_configViews;
    struct Swapchain {
        XrSwapchain handle;
        int32_t width;
        int32_t height;
    };
    std::vector<Swapchain> g_swapchains;
    std::map<XrSwapchain, std::vector<XrSwapchainImageBaseHeader*>> g_swapchainImages;
    std::vector<XrView> g_views;
    int64_t g_colorSwapchainFormat{-1};
    XrEnvironmentBlendMode g_environmentBlendMode{XR_ENVIRONMENT_BLEND_MODE_OPAQUE};
    std::list<std::vector<XrSwapchainImageOpenGLESKHR>> g_swapchainImageBuffers;

    // GLES Stuff
    GLuint g_swapchainFramebuffer{0};
    std::map<uint32_t, uint32_t> g_colorToDepthMap;
    GLuint g_program{0};
    GLuint g_vao{0};
    GLint g_modelViewProjectionUniformLocation{0};
    GLint g_vertexAttribCoords{0};
    GLint g_vertexAttribColor{0};
    GLuint g_cubeVertexBuffer{0};
    GLuint g_cubeIndexBuffer{0};

    struct Cube {
        XrPosef Pose;
        XrVector3f Scale;
    };

    namespace Geometry {
        struct Vertex {
            XrVector3f Position;
            XrVector3f Color;
        };

        constexpr XrVector3f Red{1, 0, 0};
        constexpr XrVector3f DarkRed{0.25f, 0, 0};
        constexpr XrVector3f Green{0, 1, 0};
        constexpr XrVector3f DarkGreen{0, 0.25f, 0};
        constexpr XrVector3f Blue{0, 0, 1};
        constexpr XrVector3f DarkBlue{0, 0, 0.25f};

// Vertices for a 1x1x1 meter cube. (Left/Right, Top/Bottom, Front/Back)
        constexpr XrVector3f LBB{-0.5f, -0.5f, -0.5f};
        constexpr XrVector3f LBF{-0.5f, -0.5f, 0.5f};
        constexpr XrVector3f LTB{-0.5f, 0.5f, -0.5f};
        constexpr XrVector3f LTF{-0.5f, 0.5f, 0.5f};
        constexpr XrVector3f RBB{0.5f, -0.5f, -0.5f};
        constexpr XrVector3f RBF{0.5f, -0.5f, 0.5f};
        constexpr XrVector3f RTB{0.5f, 0.5f, -0.5f};
        constexpr XrVector3f RTF{0.5f, 0.5f, 0.5f};

#define CUBE_SIDE(V1, V2, V3, V4, V5, V6, COLOR) {V1, COLOR}, {V2, COLOR}, {V3, COLOR}, {V4, COLOR}, {V5, COLOR}, {V6, COLOR},

        constexpr Vertex c_cubeVertices[] = {
                CUBE_SIDE(LTB, LBF, LBB, LTB, LTF, LBF, DarkRed)    // -X
                CUBE_SIDE(RTB, RBB, RBF, RTB, RBF, RTF, Red)        // +X
                CUBE_SIDE(LBB, LBF, RBF, LBB, RBF, RBB, DarkGreen)  // -Y
                CUBE_SIDE(LTB, RTB, RTF, LTB, RTF, LTF, Green)      // +Y
                CUBE_SIDE(LBB, RBB, RTB, LBB, RTB, LTB, DarkBlue)   // -Z
                CUBE_SIDE(LBF, LTF, RTF, LBF, RTF, RBF, Blue)       // +Z
        };

// Winding order is clockwise. Each side uses a different color.
        constexpr unsigned short c_cubeIndices[] = {
                0,  1,  2,  3,  4,  5,   // -X
                6,  7,  8,  9,  10, 11,  // +X
                12, 13, 14, 15, 16, 17,  // -Y
                18, 19, 20, 21, 22, 23,  // +Y
                24, 25, 26, 27, 28, 29,  // -Z
                30, 31, 32, 33, 34, 35,  // +Z
        };

    }  // namespace Geometry

    static const char* VertexShaderGlsl = R"_(
    #version 320 es

    in vec3 VertexPos;
    in vec3 VertexColor;

    out vec3 PSVertexColor;

    uniform mat4 ModelViewProjection;

    void main() {
       gl_Position = ModelViewProjection * vec4(VertexPos, 1.0);
       PSVertexColor = VertexColor;
    }
    )_";

    static const char* FragmentShaderGlsl = R"_(
    #version 320 es

    in lowp vec3 PSVertexColor;
    out lowp vec4 FragColor;

    void main() {
       FragColor = vec4(PSVertexColor, 1);
    }
    )_";

    struct AndroidAppState {
        ANativeWindow* NativeWindow = nullptr;
        bool Resumed = false;
    };

    template <typename T, size_t Size>
    constexpr size_t ArraySize(const T (&/*unused*/)[Size]) noexcept {
        return Size;
    }

    void CheckShader(GLuint shader) {
        GLint r = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &r);
        if (r == GL_FALSE) {
            GLchar msg[4096] = {};
            GLsizei length;
            glGetShaderInfoLog(shader, sizeof(msg), &length, msg);
            LOGE("Compile shader failed: %s", msg);
        }
    }

    void CheckProgram(GLuint prog) {
        GLint r = 0;
        glGetProgramiv(prog, GL_LINK_STATUS, &r);
        if (r == GL_FALSE) {
            GLchar msg[4096] = {};
            GLsizei length;
            glGetProgramInfoLog(prog, sizeof(msg), &length, msg);
            LOGE("Link program failed: %s", msg);
        }
    }

    /**
 * Process the next main command.
 */
    static void app_handle_cmd(struct android_app* app, int32_t cmd) {
        AndroidAppState* appState = (AndroidAppState*)app->userData;

        switch (cmd) {
            // There is no APP_CMD_CREATE. The ANativeActivity creates the
            // application thread from onCreate(). The application thread
            // then calls android_main().
            case APP_CMD_START: {
                LOGI("    APP_CMD_START");
                LOGI("onStart()");
                break;
            }
            case APP_CMD_RESUME: {
                LOGI("onResume()");
                LOGI("    APP_CMD_RESUME");
                appState->Resumed = true;
                break;
            }
            case APP_CMD_PAUSE: {
                LOGI("onPause()");
                LOGI("    APP_CMD_PAUSE");
                appState->Resumed = false;
                break;
            }
            case APP_CMD_STOP: {
                LOGI("onStop()");
                LOGI("    APP_CMD_STOP");
                break;
            }
            case APP_CMD_DESTROY: {
                LOGI("onDestroy()");
                LOGI("    APP_CMD_DESTROY");
                appState->NativeWindow = NULL;
                break;
            }
            case APP_CMD_INIT_WINDOW: {
                LOGI("surfaceCreated()");
                LOGI("    APP_CMD_INIT_WINDOW");
                appState->NativeWindow = app->window;
                break;
            }
            case APP_CMD_TERM_WINDOW: {
                LOGI("surfaceDestroyed()");
                LOGI("    APP_CMD_TERM_WINDOW");
                appState->NativeWindow = NULL;
                break;
            }
        }
    }

    namespace Math {
        namespace Pose {
            XrPosef Identity() {
                XrPosef t{};
                t.orientation.w = 1;
                return t;
            }

            XrPosef Translation(const XrVector3f& translation) {
                XrPosef t = Identity();
                t.position = translation;
                return t;
            }

            XrPosef RotateCCWAboutYAxis(float radians, XrVector3f translation) {
                XrPosef t = Identity();
                t.orientation.x = 0.f;
                t.orientation.y = std::sin(radians * 0.5f);
                t.orientation.z = 0.f;
                t.orientation.w = std::cos(radians * 0.5f);
                t.position = translation;
                return t;
            }
        }  // namespace Pose
    }  // namespace Math

    inline bool EqualsIgnoreCase(const std::string& s1, const std::string& s2, const std::locale& loc = std::locale()) {
        const std::ctype<char>& ctype = std::use_facet<std::ctype<char>>(loc);
        const auto compareCharLower = [&](char c1, char c2) { return ctype.tolower(c1) == ctype.tolower(c2); };
        return s1.size() == s2.size() && std::equal(s1.begin(), s1.end(), s2.begin(), compareCharLower);
    }

    struct IgnoreCaseStringLess {
        bool operator()(const std::string& a, const std::string& b, const std::locale& loc = std::locale()) const noexcept {
            const std::ctype<char>& ctype = std::use_facet<std::ctype<char>>(loc);
            const auto ignoreCaseCharLess = [&](char c1, char c2) { return ctype.tolower(c1) < ctype.tolower(c2); };
            return std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end(), ignoreCaseCharLess);
        }
    };

    inline XrReferenceSpaceCreateInfo GetXrReferenceSpaceCreateInfo(const std::string& referenceSpaceTypeStr) {
        XrReferenceSpaceCreateInfo referenceSpaceCreateInfo{XR_TYPE_REFERENCE_SPACE_CREATE_INFO};
        referenceSpaceCreateInfo.poseInReferenceSpace = Math::Pose::Identity();
        if (EqualsIgnoreCase(referenceSpaceTypeStr, "View")) {
            referenceSpaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_VIEW;
        } else if (EqualsIgnoreCase(referenceSpaceTypeStr, "ViewFront")) {
            // Render head-locked 2m in front of device.
            referenceSpaceCreateInfo.poseInReferenceSpace = Math::Pose::Translation({0.f, 0.f, -2.f}),
                    referenceSpaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_VIEW;
        } else if (EqualsIgnoreCase(referenceSpaceTypeStr, "Local")) {
            referenceSpaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
        } else if (EqualsIgnoreCase(referenceSpaceTypeStr, "Stage")) {
            referenceSpaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_STAGE;
        } else if (EqualsIgnoreCase(referenceSpaceTypeStr, "StageLeft")) {
            referenceSpaceCreateInfo.poseInReferenceSpace = Math::Pose::RotateCCWAboutYAxis(0.f, {-2.f, 0.f, -2.f});
            referenceSpaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_STAGE;
        } else if (EqualsIgnoreCase(referenceSpaceTypeStr, "StageRight")) {
            referenceSpaceCreateInfo.poseInReferenceSpace = Math::Pose::RotateCCWAboutYAxis(0.f, {2.f, 0.f, -2.f});
            referenceSpaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_STAGE;
        } else if (EqualsIgnoreCase(referenceSpaceTypeStr, "StageLeftRotated")) {
            referenceSpaceCreateInfo.poseInReferenceSpace = Math::Pose::RotateCCWAboutYAxis(3.14f / 3.f, {-2.f, 0.5f, -2.f});
            referenceSpaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_STAGE;
        } else if (EqualsIgnoreCase(referenceSpaceTypeStr, "StageRightRotated")) {
            referenceSpaceCreateInfo.poseInReferenceSpace = Math::Pose::RotateCCWAboutYAxis(-3.14f / 3.f, {2.f, 0.5f, -2.f});
            referenceSpaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_STAGE;
        } else {
            //throw std::invalid_argument(Fmt("Unknown reference space type '%s'", referenceSpaceTypeStr.c_str()));
        }
        return referenceSpaceCreateInfo;
    }

    const XrEventDataBaseHeader *TryReadNextEvent() {
        // It is sufficient to clear the just the XrEventDataBuffer header to
        // XR_TYPE_EVENT_DATA_BUFFER
        XrEventDataBaseHeader* baseHeader = reinterpret_cast<XrEventDataBaseHeader*>(&g_eventDataBuffer);
        *baseHeader = {XR_TYPE_EVENT_DATA_BUFFER};
        const XrResult xr = xrPollEvent(g_inst, &g_eventDataBuffer);
        if (xr == XR_SUCCESS) {
            if (baseHeader->type == XR_TYPE_EVENT_DATA_EVENTS_LOST) {
                const XrEventDataEventsLost* const eventsLost = reinterpret_cast<const XrEventDataEventsLost*>(baseHeader);
                //Log::Write(Log::Level::Warning, Fmt("%d events lost", eventsLost));
            }

            return baseHeader;
        }
        if (xr == XR_EVENT_UNAVAILABLE) {
            return nullptr;
        }

        // ERROR HERE ----------------------------------------

    }

    // Internal test ************************************

    static std::vector<XrSwapchainImageBaseHeader*> AllocateSwapchainImageStructs(
            uint32_t capacity, const XrSwapchainCreateInfo& /*swapchainCreateInfo*/) {
    // Allocate and initialize the buffer of image structs (must be sequential in memory for xrEnumerateSwapchainImages).
    // Return back an array of pointers to each swapchain image struct so the consumer doesn't need to know the type/size.
        std::vector<XrSwapchainImageOpenGLESKHR> swapchainImageBuffer(capacity);
        std::vector<XrSwapchainImageBaseHeader*> swapchainImageBase;
        for (XrSwapchainImageOpenGLESKHR& image : swapchainImageBuffer) {
            image.type = XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_ES_KHR;
            swapchainImageBase.push_back(reinterpret_cast<XrSwapchainImageBaseHeader*>(&image));
        }

        // Keep the buffer alive by moving it into the list of buffers.
        g_swapchainImageBuffers.push_back(std::move(swapchainImageBuffer));

        return swapchainImageBase;
    }

    #define GL_RGBA8                          0x8058
    static void CreateSwapchains() {
        XrResult res;
        assert(g_session != XR_NULL_HANDLE);
        assert(g_swapchains.empty());
        assert(g_configViews.empty());

        // Read graphics properties for preferred swapchain length and logging.
        XrSystemProperties systemProperties{XR_TYPE_SYSTEM_PROPERTIES};
        res = xrGetSystemProperties(g_inst, g_sysId, &systemProperties);

        // Query and cache view configuration views.
        uint32_t viewCount;
        res = xrEnumerateViewConfigurationViews(g_inst, g_sysId, g_viewConfigType, 0, &viewCount, nullptr);
        g_configViews.resize(viewCount, {XR_TYPE_VIEW_CONFIGURATION_VIEW});
        res = xrEnumerateViewConfigurationViews(g_inst, g_sysId, g_viewConfigType, viewCount, &viewCount,
                                              g_configViews.data());

        // Create and cache view buffer for xrLocateViews later.
        g_views.resize(viewCount, {XR_TYPE_VIEW});

        // Create the swapchain and get the images.
        if (viewCount > 0)
        {
            // Select a swapchain format.
            uint32_t swapchainFormatCount;
            res = xrEnumerateSwapchainFormats(g_session, 0, &swapchainFormatCount, nullptr);
            std::vector<int64_t> swapchainFormats(swapchainFormatCount);
            res = xrEnumerateSwapchainFormats(g_session, (uint32_t)swapchainFormats.size(), &swapchainFormatCount,
                                        swapchainFormats.data());
            assert(swapchainFormatCount == swapchainFormats.size());
            g_colorSwapchainFormat = 0x8058;

            // Print swapchain formats and the selected one.
            {
                std::string swapchainFormatsString;
                for (int64_t format : swapchainFormats) {
                    const bool selected = format == g_colorSwapchainFormat;
                    swapchainFormatsString += " ";
                    if (selected) {
                        swapchainFormatsString += "[";
                    }
                    swapchainFormatsString += std::to_string(format);
                    if (selected) {
                        swapchainFormatsString += "]";
                    }
                }
        //Log::Write(Log::Level::Verbose, Fmt("Swapchain Formats: %s", swapchainFormatsString.c_str()));
            }

            // Create a swapchain for each view.
            for (uint32_t i = 0; i < viewCount; i++)
            {
                const XrViewConfigurationView& vp = g_configViews[i];
        //    Log::Write(Log::Level::Info,
        //Fmt("Creating swapchain for view %d with dimensions Width=%d Height=%d SampleCount=%d", i,
         //   vp.recommendedImageRectWidth, vp.recommendedImageRectHeight, vp.recommendedSwapchainSampleCount));

                // Create the swapchain.
                XrSwapchainCreateInfo swapchainCreateInfo{XR_TYPE_SWAPCHAIN_CREATE_INFO};
                swapchainCreateInfo.arraySize = 1;
                swapchainCreateInfo.format = g_colorSwapchainFormat;
                swapchainCreateInfo.width = vp.recommendedImageRectWidth;
                swapchainCreateInfo.height = vp.recommendedImageRectHeight;
                swapchainCreateInfo.mipCount = 1;
                swapchainCreateInfo.faceCount = 1;
                swapchainCreateInfo.sampleCount = vp.recommendedSwapchainSampleCount;
                swapchainCreateInfo.usageFlags = XR_SWAPCHAIN_USAGE_SAMPLED_BIT | XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;

                Swapchain swapchain;
                swapchain.width = swapchainCreateInfo.width;
                swapchain.height = swapchainCreateInfo.height;
                res = xrCreateSwapchain(g_session, &swapchainCreateInfo, &swapchain.handle);

                g_swapchains.push_back(swapchain);

                uint32_t imageCount;
                res = xrEnumerateSwapchainImages(swapchain.handle, 0, &imageCount, nullptr);

                // XXX This should really just return XrSwapchainImageBaseHeader*
                std::vector<XrSwapchainImageBaseHeader*> swapchainImages = AllocateSwapchainImageStructs(imageCount, swapchainCreateInfo);
                res = xrEnumerateSwapchainImages(swapchain.handle, imageCount, &imageCount, swapchainImages[0]);
                g_swapchainImages.insert(std::make_pair(swapchain.handle, std::move(swapchainImages)));
            }
        }
    }

    static char *sessionStates[] = {"XR_SESSION_STATE_UNKNOWN",
                                    "XR_SESSION_STATE_IDLE",
                                    "XR_SESSION_STATE_READY",
                                    "XR_SESSION_STATE_SYNCHRONIZED",
                                    "XR_SESSION_STATE_VISIBLE",
                                    "XR_SESSION_STATE_FOCUSED",
                                    "XR_SESSION_STATE_STOPPING",
                                    "XR_SESSION_STATE_LOSS_PENDING",
                                    "XR_SESSION_STATE_EXITING"};

    static char *getSessionStateString(XrSessionState sessionState)
    {
        uint32_t index = (uint32_t) sessionState;
        if(index >= XR_SESSION_STATE_MAX_ENUM)
            return NULL;

        return sessionStates[index];
    }

    static uint32_t GetDepthTexture(uint32_t colorTexture, uint32_t width, uint32_t height) {
        // If a depth-stencil view has already been created for this back-buffer, use it.
        auto depthBufferIt = g_colorToDepthMap.find(colorTexture);
        if (depthBufferIt != g_colorToDepthMap.end()) {
            return depthBufferIt->second;
        }
#if 0
        // This back-buffer has no corresponding depth-stencil texture, so create one with matching dimensions.
        GLint width;
        GLint height;
        glBindTexture(GL_TEXTURE_2D, colorTexture);
        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);
#endif

        uint32_t depthTexture;
        glGenTextures(1, &depthTexture);
        glBindTexture(GL_TEXTURE_2D, depthTexture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, width, height, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, nullptr);

        g_colorToDepthMap.insert(std::make_pair(colorTexture, depthTexture));

        return depthTexture;
    }

    void InitializeResources() {
        glGenFramebuffers(1, &g_swapchainFramebuffer);

        EGLContext test = eglGetCurrentContext();

        GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertexShader, 1, &VertexShaderGlsl, nullptr);
        glCompileShader(vertexShader);
        CheckShader(vertexShader);

        GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragmentShader, 1, &FragmentShaderGlsl, nullptr);
        glCompileShader(fragmentShader);
        CheckShader(fragmentShader);

        g_program = glCreateProgram();
        glAttachShader(g_program, vertexShader);
        glAttachShader(g_program, fragmentShader);
        glLinkProgram(g_program);
        CheckProgram(g_program);

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);

        g_modelViewProjectionUniformLocation = glGetUniformLocation(g_program, "ModelViewProjection");

        g_vertexAttribCoords = glGetAttribLocation(g_program, "VertexPos");
        g_vertexAttribColor = glGetAttribLocation(g_program, "VertexColor");

        glGenBuffers(1, &g_cubeVertexBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, g_cubeVertexBuffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(Geometry::c_cubeVertices), Geometry::c_cubeVertices, GL_STATIC_DRAW);

        glGenBuffers(1, &g_cubeIndexBuffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_cubeIndexBuffer);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(Geometry::c_cubeIndices), Geometry::c_cubeIndices, GL_STATIC_DRAW);

        glGenVertexArraysOES(1, &g_vao);
        glBindVertexArrayOES(g_vao);
        glEnableVertexAttribArray(g_vertexAttribCoords);
        glEnableVertexAttribArray(g_vertexAttribColor);
        glBindBuffer(GL_ARRAY_BUFFER, g_cubeVertexBuffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_cubeIndexBuffer);
        glVertexAttribPointer(g_vertexAttribCoords, 3, GL_FLOAT, GL_FALSE, sizeof(Geometry::Vertex), nullptr);
        glVertexAttribPointer(g_vertexAttribColor, 3, GL_FLOAT, GL_FALSE, sizeof(Geometry::Vertex),
                              reinterpret_cast<const void*>(sizeof(XrVector3f)));
    }

    static void RenderView(const XrCompositionLayerProjectionView& layerView, const XrSwapchainImageBaseHeader* swapchainImage,
                    int64_t swapchainFormat, const std::vector<Cube>& cubes)  {

        glBindFramebuffer(GL_FRAMEBUFFER, g_swapchainFramebuffer);

        const uint32_t colorTexture = reinterpret_cast<const XrSwapchainImageOpenGLESKHR*>(swapchainImage)->image;

        glViewport(static_cast<GLint>(layerView.subImage.imageRect.offset.x),
                   static_cast<GLint>(layerView.subImage.imageRect.offset.y),
                   static_cast<GLsizei>(layerView.subImage.imageRect.extent.width),
                   static_cast<GLsizei>(layerView.subImage.imageRect.extent.height));

        glFrontFace(GL_CW);
        glCullFace(GL_BACK);
        glEnable(GL_CULL_FACE);
     //   glEnable(GL_DEPTH_TEST);

      //  const uint32_t depthTexture = GetDepthTexture(colorTexture, layerView.subImage.imageRect.extent.width, layerView.subImage.imageRect.extent.height);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTexture, 0);
  //      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTexture, 0);

        // Clear swapchain and depth buffer.
        float DarkSlateGray[] = {0.184313729f, 0.309803933f, 0.309803933f, 1.0f};
        glClearColor(DarkSlateGray[0], DarkSlateGray[1], DarkSlateGray[2], DarkSlateGray[3]);
        glClearDepthf(1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        // Set shaders and uniform variables.
        glUseProgram(g_program);

        const auto& pose = layerView.pose;
        XrMatrix4x4f proj;
        XrMatrix4x4f_CreateProjectionFov(&proj, GRAPHICS_OPENGL_ES, layerView.fov, 0.05f, 100.0f);
        XrMatrix4x4f toView;
        XrVector3f scale{1.f, 1.f, 1.f};
        XrMatrix4x4f_CreateTranslationRotationScale(&toView, &pose.position, &pose.orientation, &scale);
        XrMatrix4x4f view;
        XrMatrix4x4f_InvertRigidBody(&view, &toView);
        XrMatrix4x4f vp;
        XrMatrix4x4f_Multiply(&vp, &proj, &view);

        // Set cube primitive data.
        glBindVertexArrayOES(g_vao);

        // Render each cube
        for (const Cube& cube : cubes) {
            // Compute the model-view-projection transform and set it..
            XrMatrix4x4f model;
            XrMatrix4x4f_CreateTranslationRotationScale(&model, &cube.Pose.position, &cube.Pose.orientation, &cube.Scale);
            XrMatrix4x4f mvp;
            XrMatrix4x4f_Multiply(&mvp, &vp, &model);
            glUniformMatrix4fv(g_modelViewProjectionUniformLocation, 1, GL_FALSE, reinterpret_cast<const GLfloat*>(&mvp));

            // Draw the cube.
            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(ArraySize(Geometry::c_cubeIndices)), GL_UNSIGNED_SHORT, nullptr);
        }

        glBindVertexArrayOES(0);
        glUseProgram(0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    bool RenderLayer(XrTime predictedDisplayTime, std::vector<XrCompositionLayerProjectionView>& projectionLayerViews,
                     XrCompositionLayerProjection& layer) {
        XrResult res;

        XrViewState viewState{XR_TYPE_VIEW_STATE};
        uint32_t viewCapacityInput = (uint32_t)g_views.size();
        uint32_t viewCountOutput;

        XrViewLocateInfo viewLocateInfo{XR_TYPE_VIEW_LOCATE_INFO};
        viewLocateInfo.viewConfigurationType = g_viewConfigType;
        viewLocateInfo.displayTime = predictedDisplayTime;
        viewLocateInfo.space = g_appSpace;

        res = xrLocateViews(g_session, &viewLocateInfo, &viewState, viewCapacityInput, &viewCountOutput, g_views.data());

        if ((viewState.viewStateFlags & XR_VIEW_STATE_POSITION_VALID_BIT) == 0 ||
            (viewState.viewStateFlags & XR_VIEW_STATE_ORIENTATION_VALID_BIT) == 0) {
            return false;  // There is no valid tracking poses for the views.
        }

        assert(viewCountOutput == viewCapacityInput);
        assert(viewCountOutput == g_configViews.size());
        assert(viewCountOutput == g_swapchains.size());

        projectionLayerViews.resize(viewCountOutput);

        // For each locatable space that we want to visualize, render a 25cm cube.
        std::vector<Cube> cubes;

        float height = 0.0f;
        static float rotations[3] = {0};
        int index = 0;
        for (XrSpace visualizedSpace : g_visualizedSpaces) {
            XrSpaceLocation spaceLocation{XR_TYPE_SPACE_LOCATION};
            res = xrLocateSpace(visualizedSpace, g_appSpace, predictedDisplayTime, &spaceLocation);

            if (XR_UNQUALIFIED_SUCCESS(res)) {
                if ((spaceLocation.locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT) != 0 &&
                    (spaceLocation.locationFlags & XR_SPACE_LOCATION_ORIENTATION_VALID_BIT) != 0) {

                    XrPosef pose = Math::Pose::RotateCCWAboutYAxis(rotations[index], {spaceLocation.pose.position.x, spaceLocation.pose.position.y+height, spaceLocation.pose.position.z});
                    cubes.push_back(Cube{pose, {0.3f, 0.3f, 0.3f}});
                    height+=0.32f;

                    float speed = 0.0f;
                    switch(index){
                        case 0: speed = 0.01f; break;
                        case 1: speed = 0.002f; break;
                        case 2: speed = -0.02f; break;
                    }
                    rotations[index++]+=speed;
                }
            }
        }

        // Render view to the appropriate part of the swapchain image.
        for (uint32_t i = 0; i < viewCountOutput; i++) {
            // Each view has a separate swapchain which is acquired, rendered to, and released.
            const Swapchain viewSwapchain = g_swapchains[i];

            XrSwapchainImageAcquireInfo acquireInfo{XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};

            uint32_t swapchainImageIndex;
            xrAcquireSwapchainImage(viewSwapchain.handle, &acquireInfo, &swapchainImageIndex);

            XrSwapchainImageWaitInfo waitInfo{XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};
            waitInfo.timeout = XR_INFINITE_DURATION;
            xrWaitSwapchainImage(viewSwapchain.handle, &waitInfo);

            projectionLayerViews[i] = {XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW};
            projectionLayerViews[i].pose = g_views[i].pose;
            projectionLayerViews[i].fov = g_views[i].fov;
            projectionLayerViews[i].subImage.swapchain = viewSwapchain.handle;
            projectionLayerViews[i].subImage.imageRect.offset = {0, 0};
            projectionLayerViews[i].subImage.imageRect.extent = {viewSwapchain.width, viewSwapchain.height};

            const XrSwapchainImageBaseHeader* const swapchainImage = g_swapchainImages[viewSwapchain.handle][swapchainImageIndex];

            // GLES Render
            RenderView(projectionLayerViews[i], swapchainImage, g_colorSwapchainFormat, cubes);

            XrSwapchainImageReleaseInfo releaseInfo{XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
            xrReleaseSwapchainImage(viewSwapchain.handle, &releaseInfo);
        }

        layer.space = g_appSpace;
        layer.viewCount = (uint32_t)projectionLayerViews.size();
        layer.views = projectionLayerViews.data();
        return true;
    }

    void RenderFrame()
    {
        XrFrameWaitInfo frameWaitInfo{XR_TYPE_FRAME_WAIT_INFO};
        XrFrameState frameState{XR_TYPE_FRAME_STATE};
        xrWaitFrame(g_session, &frameWaitInfo, &frameState);

        XrFrameBeginInfo frameBeginInfo{XR_TYPE_FRAME_BEGIN_INFO};
        xrBeginFrame(g_session, &frameBeginInfo);

        std::vector<XrCompositionLayerBaseHeader*> layers;
        XrCompositionLayerProjection layer{XR_TYPE_COMPOSITION_LAYER_PROJECTION};
        std::vector<XrCompositionLayerProjectionView> projectionLayerViews;
        if (frameState.shouldRender == XR_TRUE) {
            if (RenderLayer(frameState.predictedDisplayTime, projectionLayerViews, layer)) {
                layers.push_back(reinterpret_cast<XrCompositionLayerBaseHeader*>(&layer));
            }
        }

        XrFrameEndInfo frameEndInfo{XR_TYPE_FRAME_END_INFO};
        frameEndInfo.displayTime = frameState.predictedDisplayTime;
        frameEndInfo.environmentBlendMode = g_environmentBlendMode;
        frameEndInfo.layerCount = (uint32_t)layers.size();
        frameEndInfo.layers = layers.data();
        xrEndFrame(g_session, &frameEndInfo);
    }

    void HandleSessionStateChangedEvent(const XrEventDataSessionStateChanged& stateChangedEvent, bool* exitRenderLoop,
                                        bool* requestRestart) {
        const XrSessionState oldState = g_sessionState;
        g_sessionState = stateChangedEvent.state;

        //Log::Write(Log::Level::Info, Fmt("XrEventDataSessionStateChanged: state %s->%s session=%lld time=%lld", to_string(oldState),
        //                                 to_string(m_sessionState), stateChangedEvent.session, stateChangedEvent.time));

        if ((stateChangedEvent.session != XR_NULL_HANDLE) && (stateChangedEvent.session != g_session)) {
            //Log::Write(Log::Level::Error, "XrEventDataSessionStateChanged for unknown session");
            return;
        }

        switch (g_sessionState) {
            case XR_SESSION_STATE_READY: {
                assert(g_session != XR_NULL_HANDLE);
                XrSessionBeginInfo sessionBeginInfo{XR_TYPE_SESSION_BEGIN_INFO};
                sessionBeginInfo.primaryViewConfigurationType = g_viewConfigType;
                xrBeginSession(g_session, &sessionBeginInfo);
                g_sessionRunning = true;
                break;
            }
            case XR_SESSION_STATE_STOPPING: {
                assert(g_session != XR_NULL_HANDLE);
                g_sessionRunning = false;
                xrEndSession(g_session);
                break;
            }
            case XR_SESSION_STATE_EXITING: {
                *exitRenderLoop = true;
                // Do not attempt to restart because user closed this session.
                *requestRestart = false;
                break;
            }
            case XR_SESSION_STATE_LOSS_PENDING: {
                *exitRenderLoop = true;
                // Poll for a new instance.
                *requestRestart = true;
                break;
            }
            default:
                break;
        }
    }

    void PollEvents(bool* exitRenderLoop, bool* requestRestart)
    {
        *exitRenderLoop = *requestRestart = false;

        // Process all pending messages.
        while (const XrEventDataBaseHeader* event = TryReadNextEvent())
        {
            switch (event->type)
            {
                case XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING:
                {
                    const auto& instanceLossPending = *reinterpret_cast<const XrEventDataInstanceLossPending*>(event);

                    *exitRenderLoop = true;
                    *requestRestart = true;
                    return;
                }

                case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED:
                {
                    auto sessionStateChangedEvent = *reinterpret_cast<const XrEventDataSessionStateChanged*>(event);
                    HandleSessionStateChangedEvent(sessionStateChangedEvent, exitRenderLoop, requestRestart);
                    LOGI("SESSION IS %s........................................", getSessionStateString(sessionStateChangedEvent.state));
                    break;
                }

                case XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED:
                    //LogActionSourceName(m_input.grabAction, "Grab");
                    //LogActionSourceName(m_input.quitAction, "Quit");
                    //LogActionSourceName(m_input.poseAction, "Pose");
                    //LogActionSourceName(m_input.vibrateAction, "Vibrate");
                break;

                case XR_TYPE_EVENT_DATA_REFERENCE_SPACE_CHANGE_PENDING:
                default: {
                    //Log::Write(Log::Level::Verbose, Fmt("Ignoring event type %d", event->type));
                    break;
                }
            }
    }
}





TestBaseI::TestBaseI(const WindowInfo &info) {
    if (!device) {

        jsb_init_file_operation_delegate();

#if defined(CC_DEBUG) && (CC_DEBUG > 0)
        // Enable debugger here
        jsb_enable_debugger("0.0.0.0", 6080, false);
#endif

        gfx::DeviceInfo deviceInfo;
        // deviceInfo.isAntiAlias  = true;
        deviceInfo.windowHandle = info.windowHandle;
        deviceInfo.width        = info.screen.width;
        deviceInfo.height       = info.screen.height;
        deviceInfo.nativeWidth  = info.physicalWidth;
        deviceInfo.nativeHeight = info.physicalHeight;

        device = gfx::DeviceManager::create(deviceInfo);
    }

    if (!renderPass) {
        gfx::RenderPassInfo  renderPassInfo;
        gfx::ColorAttachment colorAttachment;
        colorAttachment.format = device->getColorFormat();
        renderPassInfo.colorAttachments.emplace_back(colorAttachment);

        gfx::DepthStencilAttachment &depthStencilAttachment = renderPassInfo.depthStencilAttachment;
        depthStencilAttachment.format                       = device->getDepthStencilFormat();

        renderPass = device->createRenderPass(renderPassInfo);
    }

    if (!fbo) {
        gfx::FramebufferInfo fboInfo;
        fboInfo.colorTextures.resize(1);
        fboInfo.renderPass = renderPass;
        fbo                = device->createFramebuffer(fboInfo);
    }

    if (commandBuffers.empty()) {
        commandBuffers.push_back(device->getCommandBuffer());
    }

    hostThread.prevTime   = std::chrono::steady_clock::now();
    deviceThread.prevTime = std::chrono::steady_clock::now();
    deviceThread.prevTime = std::chrono::steady_clock::now();
}

void TestBaseI::tickScript() {
    EventDispatcher::dispatchTickEvent(0.F);
}

void TestBaseI::destroyGlobal() {
    CC_SAFE_DESTROY(test)
    CC_SAFE_DESTROY(fbo)
    CC_SAFE_DESTROY(renderPass)
    framegraph::FrameGraph::gc(0);

    for (auto textureBarrier : textureBarrierMap) {
        CC_SAFE_DELETE(textureBarrier.second)
    }
    textureBarrierMap.clear();

    for (auto globalBarrier : globalBarrierMap) {
        CC_SAFE_DELETE(globalBarrier.second)
    }
    globalBarrierMap.clear();

    se::ScriptEngine::getInstance()->cleanup();

    CC_SAFE_DESTROY(device)
}

bool TestBaseI::setupOpenXr() {

    gfx::GLES3Context *context = (gfx::GLES3Context*)device->getContext();

    assert( eglGetCurrentDisplay() != NULL);

#if 1
    typedef XrResult (XRAPI_PTR *PFN_xrInitializeLoaderKHR)(const XrLoaderInitInfoBaseHeaderKHR* loaderInitInfo);

    // Init OpenXR
    PFN_xrInitializeLoaderKHR initializeLoader = nullptr;
    if (XR_SUCCEEDED(
            xrGetInstanceProcAddr(XR_NULL_HANDLE, "xrInitializeLoaderKHR", (PFN_xrVoidFunction*)(&initializeLoader)))) {
        XrLoaderInitInfoAndroidKHR loaderInitInfoAndroid;
        memset(&loaderInitInfoAndroid, 0, sizeof(loaderInitInfoAndroid));
        loaderInitInfoAndroid.type = XR_TYPE_LOADER_INIT_INFO_ANDROID_KHR;
        loaderInitInfoAndroid.next = NULL;
        loaderInitInfoAndroid.applicationVM = JniHelper::getJavaVM();
        loaderInitInfoAndroid.applicationContext = JniHelper::getActivity();

        // Do loader init
        initializeLoader((const XrLoaderInitInfoBaseHeaderKHR*)&loaderInitInfoAndroid);

        // Extensions to enable
        std::vector<const char*> extensions;
        extensions.push_back(XR_KHR_OPENGL_ES_ENABLE_EXTENSION_NAME);
        extensions.push_back(XR_KHR_ANDROID_CREATE_INSTANCE_EXTENSION_NAME);

        XrInstanceCreateInfoAndroidKHR createInfoAndroid{XR_TYPE_INSTANCE_CREATE_INFO_ANDROID_KHR };
        createInfoAndroid.next = NULL;
        createInfoAndroid.applicationActivity = JniHelper::getActivity();
        createInfoAndroid.applicationVM = JniHelper::getJavaVM();

        // Create XrInstance
        XrInstanceCreateInfo createInfo{XR_TYPE_INSTANCE_CREATE_INFO };
        createInfo.next = &createInfoAndroid;
        createInfo.createFlags = 0;
        createInfo.enabledExtensionCount = (uint32_t)extensions.size();
        createInfo.enabledExtensionNames = extensions.data();

        strcpy(createInfo.applicationInfo.applicationName, "JamieDemo");
        createInfo.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;
        xrCreateInstance(&createInfo, &g_inst);

        XrInstanceProperties instanceProperties{XR_TYPE_INSTANCE_PROPERTIES};
        xrGetInstanceProperties(g_inst, &instanceProperties);

        // Init system
        XrSystemGetInfo systemInfo{XR_TYPE_SYSTEM_GET_INFO};
        systemInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
        XrResult res = xrGetSystem(g_inst, &systemInfo, &g_sysId);

#if 1
        PFN_xrGetOpenGLESGraphicsRequirementsKHR pfnGetOpenGLESGraphicsRequirementsKHR = nullptr;
        xrGetInstanceProcAddr(g_inst, "xrGetOpenGLESGraphicsRequirementsKHR",
                              reinterpret_cast<PFN_xrVoidFunction*>(&pfnGetOpenGLESGraphicsRequirementsKHR));

        XrGraphicsRequirementsOpenGLESKHR graphicsRequirements{XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_ES_KHR};
        pfnGetOpenGLESGraphicsRequirementsKHR(g_inst, g_sysId, &graphicsRequirements);

        int foo = 0;

#endif

#if 1 // Check views
        uint32_t viewConfigTypeCount;
        xrEnumerateViewConfigurations(g_inst, g_sysId, 0, &viewConfigTypeCount, nullptr);
        std::vector<XrViewConfigurationType> viewConfigTypes(viewConfigTypeCount);
        xrEnumerateViewConfigurations(g_inst, g_sysId, viewConfigTypeCount, &viewConfigTypeCount,
                                      viewConfigTypes.data());
        assert((uint32_t)viewConfigTypes.size() == viewConfigTypeCount);

        XrViewConfigurationProperties viewConfigProperties{XR_TYPE_VIEW_CONFIGURATION_PROPERTIES};
        xrGetViewConfigurationProperties(g_inst, g_sysId, viewConfigTypes[0], &viewConfigProperties);
        uint32_t viewCount;
        xrEnumerateViewConfigurationViews(g_inst, g_sysId, viewConfigTypes[0], 0, &viewCount, nullptr);

        if (viewCount > 0) {
            std::vector<XrViewConfigurationView> views(viewCount,
                                                       {XR_TYPE_VIEW_CONFIGURATION_VIEW});
            xrEnumerateViewConfigurationViews(g_inst, g_sysId, viewConfigTypes[0], viewCount,
                                              &viewCount, views.data());
            int yyy = 900;
        }
#endif

        android_app* app = JniHelper::getAPP();
#if 1 // Setup bindings required to create xrSession object

        // Get GFX layer information
        g_graphicsBinding.display = context->eglDisplay();
        g_graphicsBinding.config = context->eglConfig();
        g_graphicsBinding.context = context->eglContext();
#endif

#if 1 // Create xrSession object
        XrSessionCreateInfo createInfoForSession{XR_TYPE_SESSION_CREATE_INFO};
        createInfoForSession.next = reinterpret_cast<const XrBaseInStructure*>(&g_graphicsBinding);
        createInfoForSession.systemId = g_sysId;
        XrResult rest = xrCreateSession(g_inst, &createInfoForSession, &g_session);
#endif

        {
            std::string visualizedSpaces[] = {"Stage", "Stage", "Stage"};//, "Stage", "StageLeft", "StageRight", "StageLeftRotated",
                                              //"StageRightRotated"};

            for (const auto& visualizedSpace : visualizedSpaces) {
                XrReferenceSpaceCreateInfo referenceSpaceCreateInfo = GetXrReferenceSpaceCreateInfo(visualizedSpace);
                XrSpace space;
                XrResult res = xrCreateReferenceSpace(g_session, &referenceSpaceCreateInfo, &space);
                if (XR_SUCCEEDED(res)) {
                    g_visualizedSpaces.push_back(space);
                } else {
                    //Log::Write(Log::Level::Warning,
                    //           Fmt("Failed to create reference space %s with error %d", visualizedSpace.c_str(), res));
                }
            }
        }

        XrReferenceSpaceCreateInfo referenceSpaceCreateInfo = GetXrReferenceSpaceCreateInfo("Local");
        xrCreateReferenceSpace(g_session, &referenceSpaceCreateInfo, &g_appSpace);

        // GLES init
        InitializeResources();

        CreateSwapchains();

        //JNIEnv* Env;
        //app->activity->vm->AttachCurrentThread(&Env, nullptr);

        //AndroidAppState appState = {};

        //app->userData = &appState;
        //app->onAppCmd = app_handle_cmd;

        //bool requestRestart = false;
        //bool exitRenderLoop = false;


        return true;

#if 0
        // Poll events for session
        while(app->destroyRequested == 0)
        {
            // Read all pending events.
            for (;;) {
                int events;
                struct android_poll_source* source;
                // If the timeout is zero, returns immediately without blocking.
                // If the timeout is negative, waits indefinitely until an event appears.
                const int timeoutMilliseconds =
                        (!appState.Resumed && !g_sessionRunning && app->destroyRequested == 0) ? -1 : 0;
                if (ALooper_pollAll(timeoutMilliseconds, nullptr, &events, (void**)&source) < 0) {
                    break;
                }

                // Process this event.
                if (source != nullptr) {
                    source->process(app, source);
                }
            }

            PollEvents(&exitRenderLoop, &requestRestart);
            if (!g_sessionRunning) {
                continue;
            }

            RenderFrame();
        }

        int kkk = 0;
#elif 0
        bool quitKeyPressed = false;
        while (!quitKeyPressed) {
            bool exitRenderLoop = false;
            PollEvents(&exitRenderLoop, &requestRestart);
            if (exitRenderLoop) {
                break;
            }

            if (g_sessionRunning) {
                RenderFrame();
            } else {
                // Throttle loop since xrWaitFrame won't be called.
                std::this_thread::sleep_for(std::chrono::milliseconds(250));
            }
        }
#endif
    }
#endif
}

void TestBaseI::nextTest(bool backward) {
    nextDirection = backward ? -1 : 1;
}

void TestBaseI::toggleMultithread() {
    static bool multithreaded = true;
    auto *      deviceAgent   = gfx::DeviceAgent::getInstance();
    if (deviceAgent) {
        multithreaded = !multithreaded;
        deviceAgent->setMultithreaded(multithreaded);
    } else {
        multithreaded = false;
    }
}

void TestBaseI::onTouchEnd() {
    nextTest();
    // toggleMultithread();
}

void TestBaseI::update() {

    if (nextDirection) {
        CC_SAFE_DESTROY(test)
        if (nextDirection < 0) curTestIndex += tests.size();
        curTestIndex  = (curTestIndex + nextDirection) % static_cast<int>(tests.size());
        test          = tests[curTestIndex](windowInfo);
        nextDirection = 0;
    }
    if (test) {
        test->tick();
    }
}

void TestBaseI::evalString(const std::string &code) {
    se::AutoHandleScope hs;
    se::ScriptEngine::getInstance()->evalString(code.c_str());
}

void TestBaseI::runScript(const std::string &file) {
    se::AutoHandleScope hs;
    se::ScriptEngine::getInstance()->runScript(file);
}

unsigned char *TestBaseI::rgb2rgba(Image *img) {
    int                  size    = img->getWidth() * img->getHeight();
    const unsigned char *srcData = img->getData();
    auto *               dstData = new unsigned char[size * 4];
    for (int i = 0; i < size; i++) {
        dstData[i * 4]     = srcData[i * 3];
        dstData[i * 4 + 1] = srcData[i * 3 + 1];
        dstData[i * 4 + 2] = srcData[i * 3 + 2];
        dstData[i * 4 + 3] = 255U;
    }
    return dstData;
}

gfx::Texture *TestBaseI::createTextureWithFile(const gfx::TextureInfo &partialInfo, const std::string &imageFile) {
    auto *img = new cc::Image();
    img->autorelease();
    if (!img->initWithImageFile(imageFile)) return nullptr;

    unsigned char *imgData = nullptr;
    if (img->getRenderFormat() == gfx::Format::RGB8) {
        imgData = TestBaseI::rgb2rgba(img);
    } else {
        img->takeData(&imgData);
    }

    gfx::TextureInfo textureInfo = partialInfo;
    textureInfo.width  = img->getWidth();
    textureInfo.height = img->getHeight();
    textureInfo.format = gfx::Format::RGBA8;
    if (hasFlag(textureInfo.flags, gfx::TextureFlagBit::GEN_MIPMAP)) {
        textureInfo.levelCount = TestBaseI::getMipmapLevelCounts(textureInfo.width, textureInfo.height);
    }
    auto *texture = device->createTexture(textureInfo);

    gfx::BufferTextureCopy textureRegion;
    textureRegion.texExtent.width  = img->getWidth();
    textureRegion.texExtent.height = img->getHeight();
    device->copyBuffersToTexture({imgData}, texture, {textureRegion});
    delete[] imgData;
    return texture;
}

void TestBaseI::modifyProjectionBasedOnDevice(Mat4 *projection) {
    float minZ        = device->getCapabilities().clipSpaceMinZ;
    float signY       = device->getCapabilities().clipSpaceSignY;
    auto  orientation = static_cast<float>(device->getSurfaceTransform());

    Mat4 trans;
    Mat4 scale;
    Mat4 rot;
    Mat4::createTranslation(0.0F, 0.0F, 1.0F + minZ, &trans);
    Mat4::createScale(1.0F, signY, 0.5F - 0.5F * minZ, &scale);
    Mat4::createRotationZ(orientation * MATH_PIOVER2, &rot);
    *projection = rot * scale * trans * (*projection);
}

#ifndef DEFAULT_MATRIX_MATH
constexpr float preTransforms[4][4] = {
    {1, 0, 0, 1},   // GFXSurfaceTransform.IDENTITY
    {0, 1, -1, 0},  // GFXSurfaceTransform.ROTATE_90
    {-1, 0, 0, -1}, // GFXSurfaceTransform.ROTATE_180
    {0, -1, 1, 0},  // GFXSurfaceTransform.ROTATE_270
};
#endif

void TestBaseI::createOrthographic(float left, float right, float bottom, float top, float zNear, float zFar, Mat4 *dst) {
#ifdef DEFAULT_MATRIX_MATH
    Mat4::createOrthographic(left, right, bottom, top, zNear, zFar, dst);
    TestBaseI::modifyProjectionBasedOnDevice(dst);
#else
    float                 minZ         = device->getCapabilities().clipSpaceMinZ;
    float                 signY        = device->getCapabilities().clipSpaceSignY;
    gfx::SurfaceTransform orientation  = device->getSurfaceTransform();
    const float *         preTransform = preTransforms[(uint)orientation];

    memset(dst, 0, MATRIX_SIZE);

    float x  = 2.f / (right - left);
    float y  = 2.f / (top - bottom) * signY;
    float dx = (left + right) / (left - right);
    float dy = (bottom + top) / (bottom - top) * signY;

    dst->m[0]  = x * preTransform[0];
    dst->m[1]  = x * preTransform[1];
    dst->m[4]  = y * preTransform[2];
    dst->m[5]  = y * preTransform[3];
    dst->m[10] = (1.0f - minZ) / (ZNear - ZFar);
    dst->m[12] = dx * preTransform[0] + dy * preTransform[2];
    dst->m[13] = dx * preTransform[1] + dy * preTransform[3];
    dst->m[14] = (ZNear - minZ * ZFar) / (ZNear - ZFar);
    dst->m[15] = 1.0f;
#endif
}

void TestBaseI::createPerspective(float fov, float aspect, float zNear, float zFar, Mat4 *dst) {
#ifdef DEFAULT_MATRIX_MATH
    Mat4::createPerspective(MATH_DEG_TO_RAD(fov), aspect, zNear, zFar, dst);
    TestBaseI::modifyProjectionBasedOnDevice(dst);
#else
    float                 minZ         = device->getCapabilities().clipSpaceMinZ;
    float                 signY        = device->getCapabilities().clipSpaceSignY;
    gfx::SurfaceTransform orientation  = device->getSurfaceTransform();
    const float *         preTransform = preTransforms[(uint)orientation];

    memset(dst, 0, MATRIX_SIZE);

    float f  = 1.0f / std::tan(MATH_DEG_TO_RAD(fov * 0.5f));
    float nf = 1.0f / (zNear - ZFar);

    float x = f / aspect;
    float y = f * signY;

    dst->m[0]  = x * preTransform[0];
    dst->m[1]  = x * preTransform[1];
    dst->m[4]  = y * preTransform[2];
    dst->m[5]  = y * preTransform[3];
    dst->m[10] = (ZFar - minZ * zNear) * nf;
    dst->m[11] = -1.0f;
    dst->m[14] = ZFar * zNear * nf * (1.0f - minZ);
#endif
}

gfx::Extent TestBaseI::getOrientedSurfaceSize() {
    switch (device->getSurfaceTransform()) {
        case gfx::SurfaceTransform::ROTATE_90:
        case gfx::SurfaceTransform::ROTATE_270:
            return {device->getHeight(), device->getWidth()};
        case gfx::SurfaceTransform::IDENTITY:
        case gfx::SurfaceTransform::ROTATE_180:
        default:
            return {device->getWidth(), device->getHeight()};
    }
}

gfx::Viewport TestBaseI::getViewportBasedOnDevice(const Vec4 &relativeArea) {
    float x = relativeArea.x;
    float y = device->getCapabilities().screenSpaceSignY < 0.0F ? 1.F - relativeArea.y - relativeArea.w : relativeArea.y;
    float w = relativeArea.z;
    float h = relativeArea.w;

    gfx::Extent   size{device->getWidth(), device->getHeight()};
    gfx::Viewport viewport;

    switch (device->getSurfaceTransform()) {
        case gfx::SurfaceTransform::ROTATE_90:
            viewport.left   = uint((1.F - y - h) * size.width);
            viewport.top    = uint(x * size.height);
            viewport.width  = uint(h * size.width);
            viewport.height = uint(w * size.height);
            break;
        case gfx::SurfaceTransform::ROTATE_180:
            viewport.left   = uint((1.F - x - w) * size.width);
            viewport.top    = uint((1.F - y - h) * size.height);
            viewport.width  = uint(w * size.width);
            viewport.height = uint(h * size.height);
            break;
        case gfx::SurfaceTransform::ROTATE_270:
            viewport.left   = uint(y * size.width);
            viewport.top    = uint((1.F - x - w) * size.height);
            viewport.width  = uint(h * size.width);
            viewport.height = uint(w * size.height);
            break;
        case gfx::SurfaceTransform::IDENTITY:
            viewport.left   = uint(x * size.width);
            viewport.top    = uint(y * size.height);
            viewport.width  = uint(w * size.width);
            viewport.height = uint(h * size.height);
            break;
    }

    return viewport;
}

uint TestBaseI::getMipmapLevelCounts(uint width, uint height) {
    return static_cast<uint>(std::floor(std::log2(std::max(width, height)))) + 1;
}

uint TestBaseI::getUBOSize(uint size) {
    return (size + 15) / 16 * 16; // https://bugs.chromium.org/p/chromium/issues/detail?id=988988
}

uint TestBaseI::getAlignedUBOStride(uint stride) {
    uint alignment = device->getCapabilities().uboOffsetAlignment;
    return (stride + alignment - 1) / alignment * alignment;
}

void TestBaseI::createUberBuffer(const vector<uint> &sizes, gfx::Buffer **pBuffer,
                                 vector<gfx::Buffer *> *pBufferViews, vector<uint> *pBufferViewOffsets) {
    pBufferViewOffsets->assign(sizes.size() + 1, 0);
    for (uint i = 0U; i < sizes.size(); ++i) {
        uint alignedSize              = i == sizes.size() - 1 ? sizes[i] : getAlignedUBOStride(sizes[i]);
        pBufferViewOffsets->at(i + 1) = pBufferViewOffsets->at(i) + alignedSize;
    }
    *pBuffer = device->createBuffer({
        gfx::BufferUsage::UNIFORM,
        gfx::MemoryUsage::DEVICE | gfx::MemoryUsage::HOST,
        TestBaseI::getUBOSize(pBufferViewOffsets->back()),
    });
    for (uint i = 0U; i < sizes.size(); ++i) {
        pBufferViews->push_back(device->createBuffer({
            *pBuffer,
            pBufferViewOffsets->at(i),
            sizes[i],
        }));
    }
}

tinyobj::ObjReader TestBaseI::loadOBJ(const String & /*path*/) {
    String objFile = FileUtils::getInstance()->getStringFromFile("bunny.obj");
    String mtlFile;

    tinyobj::ObjReader       obj;
    tinyobj::ObjReaderConfig config;
    config.vertex_color = false;
    obj.ParseFromString(objFile, mtlFile, config);
    CCASSERT(obj.Valid(), "file failed to load");

    return obj;
}

gfx::GlobalBarrier *TestBaseI::getGlobalBarrier(const gfx::GlobalBarrierInfo &info) {
    uint hash = gfx::GlobalBarrier::computeHash(info);
    if (!globalBarrierMap.count(hash)) {
        globalBarrierMap[hash] = device->createGlobalBarrier(info);
    }
    return globalBarrierMap[hash];
}

gfx::TextureBarrier *TestBaseI::getTextureBarrier(const gfx::TextureBarrierInfo &info) {
    uint hash = gfx::TextureBarrier::computeHash(info);
    if (!textureBarrierMap.count(hash)) {
        textureBarrierMap[hash] = device->createTextureBarrier(info);
    }
    return textureBarrierMap[hash];
}

} // namespac   