#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#if __linux__
    #include <X11/Xlib.h>
    #include <X11/Xmu/WinUtil.h>
    #define OS 1
#elif _WIN32
    #define OS 2
#elif __APPLE__
    #define OS 3
#else
    #define OS 0
#endif

#include <set>
#include <array>
#include <chrono>
#include <time.h>
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <functional>
//#include <cstdlib>
#include <cstring>
#include <fstream>
#include <optional>
#include <vulkan/vk_sdk_platform.h>
#include <vulkan/vulkan.hpp>

#include "../include/vkWarpConfig.h"

const int WIDTH = 1920;
const int HEIGHT = 1080;

const int MAX_FRAMES_IN_FLIGHT = 2;

const std::vector<const char*> validationLayers = {"VK_LAYER_KHRONOS_validation"};
const std::vector<const char*> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

#ifdef NDEBUG
#define VERIFY(x) assert(x)
    const bool enableValidationLayers = false;
#else
#define VERIFY(x) ((void) x)
    const bool enableValidationLayers = true;
#endif

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

#ifdef _WIN32
#define ERR_EXIT(err_msg, err_class)                                          \
    do {                                                                      \
        if (!suppress_popups) MessageBox(nullptr, err_msg, err_class, MB_OK); \
        exit(1);                                                              \
    } while (0)
#else
#define ERR_EXIT(err_msg, err_class) \
    do {                             \
        printf("%s\n", err_msg);     \
        fflush(stdout);              \
        exit(1);                     \
    } while (0)
#endif

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator,
                                        VkDebugUtilsMessengerEXT* pDebugMessenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

VkResult DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() {
        return graphicsFamily.has_value()
            && presentFamily.has_value();
    } 
};

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

struct Vertex {
    glm::vec2 pos;
    glm::vec3 color;
    glm::vec2 texCoord;

    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription bindingDescr = {};
        bindingDescr.binding = 0;
        bindingDescr.stride = sizeof(Vertex);
        bindingDescr.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescr;
    }

    static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 3> attrDesrc = {};
        
        attrDesrc[0].binding = 0;
        attrDesrc[0].location = 0;
        attrDesrc[0].format = VK_FORMAT_R32G32_SFLOAT;
        attrDesrc[0].offset = offsetof(Vertex, pos);

        attrDesrc[1].binding = 0;
        attrDesrc[1].location = 1;
        attrDesrc[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attrDesrc[1].offset = offsetof(Vertex, color);

        attrDesrc[2].binding = 0;
        attrDesrc[2].location = 2;
        attrDesrc[2].format = VK_FORMAT_R32G32_SFLOAT;
        attrDesrc[2].offset = offsetof(Vertex, texCoord);

        return attrDesrc;
    } 
};

struct UniformBufferObject {
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
};

// ###VERTICES INFORMATION###
const std::vector<Vertex> verticesQuad = {
    {{-0.5625f, -1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
    {{ 0.5625f, -1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
    {{ 0.5625f,  1.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
    {{-0.5625f,  1.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}
};

const std::vector<Vertex> verticesFull = {
    {{-1.0f, -1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
    {{ 1.0f, -1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
    {{ 1.0f,  1.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
    {{-1.0f,  1.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}
};

const std::vector<uint16_t> indices = {
    0, 1, 2, 2, 3, 0
};

class VkWarpApp {
private:
    GLFWwindow* window;
    
    // for real-time screen capturing
    #if __linux__
        Display *display;
        Window root_window;
        XImage *screenCapture;
    #endif
    VkInstance instance = 0;
    VkDebugUtilsMessengerEXT debugMessenger;
    VkSurfaceKHR surface;
    
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice logicalDevice;
    
    VkQueue graphicsQueue;
    VkQueue presentQueue;
    
    VkSwapchainKHR swapChain;
    std::vector<VkImage> swapChainImages;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
    std::vector<VkImageView> swapChainImageViews;
    std::vector<VkFramebuffer> swapChainFramebuffers;
    
    VkRenderPass renderPass;
    VkDescriptorSetLayout descriptorSetLayout;
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;
    
    VkCommandPool commandPool;

    VkImage uvMSTextureImage;
    VkDeviceMemory uvMSTextureImageMemory;
    VkImageView uvMSTextureImageView;

    VkImage uvLSTextureImage;
    VkDeviceMemory uvLSTextureImageMemory;
    VkImageView uvLSTextureImageView;

    VkImage colorTextureImage;
    VkDeviceMemory colorTextureImageMemory;
    VkImageView colorTextureImageView;
    VkFormat colorTexFormat;
    VkBuffer colorStagingBuffer;
    VkDeviceMemory colorStagingBufferMemory;
    
    VkSampler textureSampler;

    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;

    std::vector<VkBuffer> uniformBuffers;
    std::vector<VkDeviceMemory> uniformBuffersMemory;

    VkDescriptorPool descriptorPool;
    std::vector<VkDescriptorSet> descriptorSets;
    std::array<VkWriteDescriptorSet, 4> writeDescriptorSets = {};
    VkDescriptorImageInfo descriptorColorImageInfo = {};
    
    std::vector<VkCommandBuffer> commandBuffers;
    
    std::vector<VkSemaphore> imgAvailSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    size_t currentFrame = 0;

    bool framebufferResized = false;
    bool capture = false;
    time_t globalStartTime;
    int frameNumber = 0;
    int warpType;
    //const char* idMS = "texture/identityUVMS.png";
    //const char* idLS = "texture/identityUVLS.png";
    //const char* swMS = "texture/SimpleWarpUVMS.png";
    //const char* swLS = "texture/SimpleWarpUVLS.png";
    //const char* swiMS = "texture/SimpleWarpUVIdentityMS.png";
    //const char* swiLS = "texture/SimpleWarpUVIdentityLS.png";
    //const char* warpMS = "texture/WarpUVMS.png";
    //const char* warpLS = "texture/WarpUVLS.png";


    // Initialising GLFW instance, attributes and creating window
    void initWindow() {
        #if __linux__
            display = XOpenDisplay(nullptr);
            root_window = DefaultRootWindow(display);
        #endif

        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        window = glfwCreateWindow(WIDTH, HEIGHT, "vkWarp", nullptr, nullptr);
        glfwSetWindowUserPointer(window, this);
        glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
    }

    static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
        auto app = reinterpret_cast<VkWarpApp*>(glfwGetWindowUserPointer(window));
        app->framebufferResized = true;
    }

    void initVulkan(bool fullscreen, const char* uvMSFilename, const char* uvLSFilename) {
        createInstance();
        std::cout << "Instance Created" << std::endl;
        setupDebugCallback();
        std::cout << "Debug Callback Setup Successful" << std::endl;
        createSurface();
        std::cout << "Surface Created" << std::endl;
        pickPhysicalDevice();
        std::cout << "Physical Device Picked" << std::endl;
        createLogicalDevice();
        std::cout << "Logical Device Created" << std::endl;
        createSwapChain();
        std::cout << "Swap Chain Created" << std::endl;
        createImageViews();
        std::cout << "Image Views Created" << std::endl;
        createRenderPass();
        std::cout << "Render Pass Created" << std::endl;
        createDescriptorSetLayout();
        std::cout << "Descriptor Set Layout Created" << std::endl;
        createGraphicsPipeline();
        std::cout << "Graphics Pipeline Created" << std::endl;
        createFramebuffers();
        std::cout << "Framebuffers Created" << std::endl;
        createCommandPool();
        std::cout << "Command Pool Created" << std::endl;
        createTextureImage(uvMSFilename, uvLSFilename);
        std::cout << "Texture Image Created" << std::endl;
        createTextureImageView();
        std::cout << "Texture Image View Created" << std::endl;
        createTextureSampler();
        std::cout << "Texture Image Sampler" << std::endl;
        if (fullscreen){
            createVertexBuffer(verticesFull);
        } else {
            createVertexBuffer(verticesQuad);
        }
        std::cout << "Vertex Buffer Created" << std::endl;
        createIndexBuffer();
        std::cout << "Index Buffer Created" << std::endl;
        createUniformBuffer();
        std::cout << "Uniform Buffer Created" << std::endl;
        createDescriptorPool();
        std::cout << "Descriptor Pool Created" << std::endl;
        createDescriptorSets();
        std::cout << "Descriptor Sets Created" << std::endl;
        createCommandBuffers();
        std::cout << "Command Buffers Created" << std::endl;
        createSyncObjs();
        std::cout << "Semaphores Created" << std::endl;
    }

    void mainLoop() {
       time(&globalStartTime);

        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
            drawFrame();
        }
        
        vkDeviceWaitIdle(logicalDevice);
    }

    void cleanupSwapChain() {
        for (auto framebuffer : swapChainFramebuffers) {
            vkDestroyFramebuffer(logicalDevice, framebuffer, nullptr);
            std::cout << "Framebuffer Destroyed" << std::endl;
        }

        vkFreeCommandBuffers(logicalDevice, commandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());

        vkDestroyPipeline(logicalDevice, graphicsPipeline, nullptr);
        std::cout << "Graphics Pipeline Destroyed" << std::endl;
        vkDestroyPipelineLayout(logicalDevice, pipelineLayout, nullptr);
        std::cout << "Pipeline Layout Destroyed" << std::endl;
        vkDestroyRenderPass(logicalDevice ,renderPass, nullptr);
        std::cout << "Render Pass Destroyed" << std::endl;

        for (auto imgView : swapChainImageViews) {
            vkDestroyImageView(logicalDevice, imgView, nullptr);
            std::cout << "Image View Destroyed" << std::endl;
        }

        vkDestroySwapchainKHR(logicalDevice, swapChain, nullptr);
        std::cout << "Swapchain Destroyed" << std::endl;

        for (size_t i = 0; i < swapChainImages.size(); i++) {
            vkDestroyBuffer(logicalDevice, uniformBuffers[i], nullptr);
            vkFreeMemory(logicalDevice, uniformBuffersMemory[i], nullptr);
        }

        vkDestroyBuffer(logicalDevice, colorStagingBuffer, nullptr);
        vkFreeMemory(logicalDevice, colorStagingBufferMemory, nullptr);

        vkDestroyDescriptorPool(logicalDevice, descriptorPool, nullptr);
    }

    // Destroying Vulkan and GLFW instances before exit
    void cleanup() {
        cleanupSwapChain();

        vkDestroySampler(logicalDevice, textureSampler, nullptr);
        
        vkDestroyImageView(logicalDevice, colorTextureImageView, nullptr);
        vkDestroyImage(logicalDevice, colorTextureImage, nullptr);
        vkFreeMemory(logicalDevice, colorTextureImageMemory, nullptr);

        vkDestroyImageView(logicalDevice, uvLSTextureImageView, nullptr);
        vkDestroyImage(logicalDevice, uvLSTextureImage, nullptr);
        vkFreeMemory(logicalDevice, uvLSTextureImageMemory, nullptr);

        vkDestroyImageView(logicalDevice, uvMSTextureImageView, nullptr);
        vkDestroyImage(logicalDevice, uvMSTextureImage, nullptr);
        vkFreeMemory(logicalDevice, uvMSTextureImageMemory, nullptr);

        vkDestroyDescriptorSetLayout(logicalDevice, descriptorSetLayout, nullptr);

        vkDestroyBuffer(logicalDevice, indexBuffer, nullptr);
        vkFreeMemory(logicalDevice, indexBufferMemory, nullptr);

        vkDestroyBuffer(logicalDevice, vertexBuffer, nullptr);
        vkFreeMemory(logicalDevice, vertexBufferMemory, nullptr);

        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroySemaphore(logicalDevice ,renderFinishedSemaphores[i], nullptr);
            vkDestroySemaphore(logicalDevice, imgAvailSemaphores[i], nullptr);
            vkDestroyFence(logicalDevice, inFlightFences[i], nullptr);
            std::cout << "Sync Objects Destroyed" << std::endl;
        }

        vkDestroyCommandPool(logicalDevice, commandPool, nullptr);
        std::cout << "Command Pool Destroyed" << std::endl;

        //cleanupSwapChain();

        vkDestroyDevice(logicalDevice, nullptr);
        std::cout << "Logical Device Destroyed" << std::endl;

        if(enableValidationLayers) {
            DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
            std::cout << "Debug Messenger Destroyed" << std::endl;
        }

        vkDestroySurfaceKHR(instance, surface, nullptr);
        std::cout << "Surface Destroyed" << std::endl;
        vkDestroyInstance(instance, nullptr);
        std::cout << "Instance Destroyed" << std::endl;
        #if __linux__
            XCloseDisplay(display);
        #endif
        glfwDestroyWindow(window);
        std::cout << "Window Destroyed" << std::endl;
        glfwTerminate();
    }

    void recreateSwapChain() {
        int width = 0, height = 0;
        while (width == 0 || height == 0) {
            glfwGetFramebufferSize(window, &width , &height);
            glfwWaitEvents;
        }
        
        vkDeviceWaitIdle(logicalDevice);

        createSwapChain();
        createImageViews();
        createRenderPass();
        createGraphicsPipeline();
        createFramebuffers();
        createUniformBuffer();
        createDescriptorPool();
        createDescriptorSets();
        createCommandBuffers();
    }

    // Initialising Application Metadata, checking supported extensions and initialising Vulkan instance
    void createInstance(){
        if (enableValidationLayers && !checkValidationLayerSupport()) {
            throw std::runtime_error("validation layers requested, but not available!");
        }

        VkApplicationInfo appInfo = {};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "My First Vulkan";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0 , 0);
        appInfo.pEngineName = "No Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0 , 0);
        appInfo.apiVersion = VK_API_VERSION_1_0;

        VkInstanceCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        auto extensions = getRequiredExtensions();
        createInfo.enabledExtensionCount = (uint32_t) extensions.size();
        createInfo.ppEnabledExtensionNames = extensions.data();

        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
        if (enableValidationLayers) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();

            populateDebugMessengerCreateInfo(debugCreateInfo);
            createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
        } else {
            createInfo.enabledLayerCount = 0;
            createInfo.pNext = nullptr;
        }
        

        //std::cout << "creating instance" << std::endl;
        VkResult res = vkCreateInstance(&createInfo, nullptr, &instance);
        //std::cout << (int32_t) res << std::endl;
        if (res != VK_SUCCESS) {
            throw std::runtime_error("failed to create instance!");
        }
        /*std::cout << "available extensions:" << std::endl;
        for (const auto& extension : extensions) {
            std::cout << "\t" << extensions.extensionName << std::endl;
        }*/
    }

    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
        createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                    VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = debugCallback;
    }

    void setupDebugCallback() {
        if (!enableValidationLayers) return;

        VkDebugUtilsMessengerCreateInfoEXT createInfo;
        populateDebugMessengerCreateInfo(createInfo);

        if(CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
            throw std::runtime_error("failed to set up debug messenger!");
        }
    }

    void createSurface() {
        std::cout << "...creating Window Surface..." << std::endl;
        VkResult res = glfwCreateWindowSurface(instance, window, nullptr, &surface);
        if (res != VK_SUCCESS){
            throw std::runtime_error("failed to create window surface!");
        }
    }

    void pickPhysicalDevice() {
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

        if (deviceCount == 0) {
            throw std::runtime_error("failed to find GPUs with Vulkan support!");
        }

        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
        //std::cout << "Number of Found Physical Devices: " << devices.size() << std::endl;

        for (const auto& device : devices) {
            if (isDeviceSuitable(device)) {
                std::cout << "this device is suitable!" << std::endl;
                physicalDevice = device;
                break;
            }
        }

        if (physicalDevice == VK_NULL_HANDLE) {
            throw std::runtime_error("failed to find a suitable GPU");
        }
    }

    void createLogicalDevice() {
        QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(), indices.presentFamily.value()};

        float queuePriority = 1.0f;
        for (uint32_t queueFamily : uniqueQueueFamilies) {
            VkDeviceQueueCreateInfo queueCreateInfo = {};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }

        VkPhysicalDeviceFeatures deviceFeatures = {}; // to be populated later (if special capabilities are required)
        deviceFeatures.samplerAnisotropy = VK_TRUE;

        VkDeviceCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        createInfo.pQueueCreateInfos = queueCreateInfos.data();
        createInfo.pEnabledFeatures = &deviceFeatures;
        createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
        createInfo.ppEnabledExtensionNames = deviceExtensions.data();

        if (enableValidationLayers) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
        } else {
            createInfo.enabledLayerCount = 0;
        }

        std::cout << "...creating Logical Device..." << std::endl;
        VkResult res = vkCreateDevice(physicalDevice, &createInfo, nullptr, &logicalDevice);
        if (res != VK_SUCCESS){
            throw std::runtime_error("failed to create logical device!");
        }

        vkGetDeviceQueue(logicalDevice, indices.graphicsFamily.value(), 0, &graphicsQueue);
        vkGetDeviceQueue(logicalDevice, indices.presentFamily.value(), 0, &presentQueue);
    }

    void createSwapChain() {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

        VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
        VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
        VkExtent2D extent = chooseSwapExtent(swapChainSupport.surfaceCapabilities);

        // defining swap chain size
        uint32_t imageCount = swapChainSupport.surfaceCapabilities.minImageCount + 1;
        if (swapChainSupport.surfaceCapabilities.maxImageCount > 0 &&
            imageCount > swapChainSupport.surfaceCapabilities.maxImageCount) {
            imageCount = swapChainSupport.surfaceCapabilities.maxImageCount;
        } // maxImageCount == 0 means that there are NO specified limits

        VkSwapchainCreateInfoKHR scCreateInfo = {};
        scCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        scCreateInfo.surface = surface;
        scCreateInfo.minImageCount = imageCount;
        scCreateInfo.imageFormat = surfaceFormat.format;
        scCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
        scCreateInfo.imageExtent = extent;
        scCreateInfo.imageArrayLayers = 1; // for 3D application it is higher
        scCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; // images used as Color Attachments (rendering targets)

        QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
        uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

        if (indices.graphicsFamily != indices.presentFamily) { // graphics and presentation are handled by different Queue Families
            scCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            scCreateInfo.queueFamilyIndexCount = 2;
            scCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
        } else {
            scCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            scCreateInfo.queueFamilyIndexCount = 0;
            scCreateInfo.pQueueFamilyIndices = nullptr;
        }

        scCreateInfo.preTransform = swapChainSupport.surfaceCapabilities.currentTransform; // it can be modified to force one transformation at HW level (if supported)
        scCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; // specifies if the alpha should be considered when blending with other windows (now ignored)
        scCreateInfo.presentMode = presentMode;
        scCreateInfo.clipped = VK_TRUE; // ignores colour of hidden window pixels
        scCreateInfo.oldSwapchain = VK_NULL_HANDLE; // sometimes (e.g. on resizing) the swap chain should be created again and the the handle of the old one have to be placed here (not used yet)

        std::cout << "...creating Swap Chain..." << std::endl;
        VkResult res = vkCreateSwapchainKHR(logicalDevice, &scCreateInfo, nullptr, &swapChain);
        if (res != VK_SUCCESS){
            throw std::runtime_error("failed to create swap chain!");
        }

        vkGetSwapchainImagesKHR(logicalDevice, swapChain, &imageCount, nullptr);
        swapChainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(logicalDevice, swapChain, &imageCount, swapChainImages.data());

        swapChainImageFormat = surfaceFormat.format;
        swapChainExtent = extent;
    }

    void createImageViews() {
        swapChainImageViews.resize(swapChainImages.size());

        for (size_t i = 0; i < swapChainImageViews.size(); i++) {
            swapChainImageViews[i] = createImageView(swapChainImages[i], swapChainImageFormat);
        }
    }

    void createRenderPass() {
        VkAttachmentDescription colorAttachment = {};
        colorAttachment.format = swapChainImageFormat;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // what to do to the attachment data before displaying (keep old data/clear to black/don't care)
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // what to do to the attachment data after displaying (store old data/don't care)
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; // same as before but for stencil data (and not colour and alpha data)
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // found attachment layout 
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // how to use the attachment (color attachment/swap chain image/ destination for memory copy)

        VkAttachmentReference colorAttachmentRef = {};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass = {};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef; // this field store the pointer where the output of the fragment shader will be written

        VkSubpassDependency subpassDependency = {};
        subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        subpassDependency.dstSubpass = 0;
        subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        subpassDependency.srcAccessMask = 0;
        subpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        subpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        VkRenderPassCreateInfo renderPassCreateInfo = {};
        renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassCreateInfo.attachmentCount = 1;
        renderPassCreateInfo.pAttachments = &colorAttachment;
        renderPassCreateInfo.subpassCount = 1;
        renderPassCreateInfo.pSubpasses = &subpass;
        renderPassCreateInfo.dependencyCount = 1;
        renderPassCreateInfo.pDependencies = &subpassDependency;

        std::cout << "...creating an render pass..." << std::endl;
        VkResult res = vkCreateRenderPass(logicalDevice, &renderPassCreateInfo, nullptr, &renderPass);
        if (res != VK_SUCCESS){
            throw std::runtime_error("failed to create render pass!");
        }
    }

    void createDescriptorSetLayout() {
        VkDescriptorSetLayoutBinding uboLayoutBinding = {};
        uboLayoutBinding.binding = 0;
        uboLayoutBinding.descriptorCount = 1;
        uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboLayoutBinding.pImmutableSamplers = nullptr;
        uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

        VkDescriptorSetLayoutBinding uvMSSamplerLayoutBinding = {};
        uvMSSamplerLayoutBinding.binding = 1;
        uvMSSamplerLayoutBinding.descriptorCount = 1;
        uvMSSamplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        uvMSSamplerLayoutBinding.pImmutableSamplers = nullptr;
        uvMSSamplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutBinding uvLSSamplerLayoutBinding = {};
        uvLSSamplerLayoutBinding.binding = 2;
        uvLSSamplerLayoutBinding.descriptorCount = 1;
        uvLSSamplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        uvLSSamplerLayoutBinding.pImmutableSamplers = nullptr;
        uvLSSamplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutBinding colorSamplerLayoutBinding = {};
        colorSamplerLayoutBinding.binding = 3;
        colorSamplerLayoutBinding.descriptorCount = 1;
        colorSamplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        colorSamplerLayoutBinding.pImmutableSamplers = nullptr;
        colorSamplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        std::array<VkDescriptorSetLayoutBinding, 4> bindings = {
            uboLayoutBinding,
            uvMSSamplerLayoutBinding,
            uvLSSamplerLayoutBinding,
            colorSamplerLayoutBinding
        };
        VkDescriptorSetLayoutCreateInfo dsLayoutCreateInfo = {};
        dsLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        dsLayoutCreateInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        dsLayoutCreateInfo.pBindings = bindings.data();

        std::cout << "...creating descriptor set layout..." << std::endl;
        VkResult res = vkCreateDescriptorSetLayout(logicalDevice, &dsLayoutCreateInfo, nullptr, &descriptorSetLayout);
        if (res != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor set layout!");
        }
    }

    void createGraphicsPipeline() {
        auto vertShaderCode = readFile("shaders/vert.spv");
        auto fragShaderCode = readFile("shaders/frag.spv");

        std::cout << "...creating Vertex Shader Module..." << std::endl;
        VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
        std::cout << "...creating Fragment Shader Module..." << std::endl;
        VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

        VkPipelineShaderStageCreateInfo vertssCreateInfo = {};
        vertssCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertssCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertssCreateInfo.module = vertShaderModule;
        vertssCreateInfo.pName = "main";

        VkPipelineShaderStageCreateInfo fragssCreateInfo = {};
        fragssCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragssCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragssCreateInfo.module = fragShaderModule;
        fragssCreateInfo.pName = "main";

        VkPipelineShaderStageCreateInfo shaderStages[] = {vertssCreateInfo, fragssCreateInfo};

        VkPipelineVertexInputStateCreateInfo vertInputCreateInfo = {};
        vertInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

        auto bindingDescr = Vertex::getBindingDescription();
        auto attrDescr = Vertex::getAttributeDescriptions();

        vertInputCreateInfo.vertexBindingDescriptionCount = 1;
        vertInputCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attrDescr.size());
        vertInputCreateInfo.pVertexBindingDescriptions = &bindingDescr;
        vertInputCreateInfo.pVertexAttributeDescriptions = attrDescr.data();

        VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo = {};
        inputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssemblyStateCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; // how the vertices are read
        inputAssemblyStateCreateInfo.primitiveRestartEnable = VK_FALSE;

        VkViewport viewport = {};
        // viewport center position wrt framebuffer
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        // viewport size
        viewport.width  = (float) swapChainExtent.width;
        viewport.height = (float) swapChainExtent.height;
        // view depth
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        // rectangle defining which part of the framebuffer is rendered
        VkRect2D scissor = {};
        scissor.offset = {0,0};
        scissor.extent = swapChainExtent;

        VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {};
        viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportStateCreateInfo.viewportCount = 1;
        viewportStateCreateInfo.pViewports = &viewport;
        viewportStateCreateInfo.scissorCount = 1;
        viewportStateCreateInfo.pScissors = &scissor;

        VkPipelineRasterizationStateCreateInfo rasterizerStateCreateInfo = {};
        rasterizerStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizerStateCreateInfo.depthClampEnable = VK_FALSE; // discarding fragments out of the depth limits
        rasterizerStateCreateInfo.rasterizerDiscardEnable = VK_FALSE; // if enabled rasterizer is ignored
        rasterizerStateCreateInfo.polygonMode = VK_POLYGON_MODE_FILL; // how to render polygon: filling/only edges/only vertices
        rasterizerStateCreateInfo.lineWidth = 1.0f; // for lineWidths thicker then 1.0f, a wideLines GPU features has to be enabled
        rasterizerStateCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT; // which pixel has to be culled (none/only front/only back/both)
        rasterizerStateCreateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE; // how to read the pixel for the front face (clockwise/counterclockwise)
        rasterizerStateCreateInfo.depthBiasEnable = VK_FALSE; // enabling modification (biasing) of depth values (sometimes used for shadow mapping)
        //rasterizerStateCreateInfo.depthBiasConstantFactor = 0.0f; // initial offset
        //rasterizerStateCreateInfo.depthBiasClamp = 0.0f;
        //rasterizerStateCreateInfo.depthBiasSlopeFactor = 0.0f;

        VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo = {};
        multisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampleStateCreateInfo.sampleShadingEnable = VK_FALSE;
        multisampleStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        //multisampleStateCreateInfo.minSampleShading = 1.0f;
        //multisampleStateCreateInfo.pSampleMask = nullptr;
        //multisampleStateCreateInfo.alphaToCoverageEnable = VK_FALSE;
        //multisampleStateCreateInfo.alphaToOneEnable = VK_FALSE;

        //VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo = {};

        // setting for a specific colour attachment (texture)
        VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE; // opaque blending
        //colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // blending factor for source colour
        //colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // blending factor for destination colour
        //colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // colour blending operation 
        //colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // blending factor for source alpha
        //colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // blending factor for destination alpha
        //colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // alpha blending operation

        // GLOBAL setting for blending
        VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo = {};
        colorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlendStateCreateInfo.logicOpEnable = VK_FALSE;
        //colorBlendStateCreateInfo.logicOp = nullptr;
        colorBlendStateCreateInfo.attachmentCount = 1;
        colorBlendStateCreateInfo.pAttachments = &colorBlendAttachment;
        //colorBlendStateCreateInfo.blendConstants[0] = 0.0f;
        //colorBlendStateCreateInfo.blendConstants[1] = 0.0f;
        //colorBlendStateCreateInfo.blendConstants[2] = 0.0f;
        //colorBlendStateCreateInfo.blendConstants[3] = 0.0f;

        // Dynamic states to be handled here

        VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
        pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutCreateInfo.setLayoutCount = 1;
        pipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayout;

        std::cout << "...creating pipeline layout..." << std::endl;
        VkResult res = vkCreatePipelineLayout(logicalDevice, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout);
        if (res != VK_SUCCESS){
            throw std::runtime_error("failed to create pipeline layout!");
        }

        VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo = {};
        graphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        graphicsPipelineCreateInfo.stageCount = 2;
        graphicsPipelineCreateInfo.pStages = shaderStages;
        graphicsPipelineCreateInfo.pVertexInputState = &vertInputCreateInfo;
        graphicsPipelineCreateInfo.pInputAssemblyState = &inputAssemblyStateCreateInfo;
        graphicsPipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
        graphicsPipelineCreateInfo.pRasterizationState = &rasterizerStateCreateInfo;
        graphicsPipelineCreateInfo.pMultisampleState = &multisampleStateCreateInfo;
        //graphicsPipelineCreateInfo.pDepthStencilState = nullptr;
        graphicsPipelineCreateInfo.pColorBlendState = &colorBlendStateCreateInfo;
        //graphicsPipelineCreateInfo.pDynamicState = nullptr;
        graphicsPipelineCreateInfo.layout = pipelineLayout;
        graphicsPipelineCreateInfo.renderPass = renderPass;
        graphicsPipelineCreateInfo.subpass = 0;
        //graphicsPipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE; // advanced option
        //graphicsPipelineCreateInfo.basePipelineIndex = 0; // advanced option

        std::cout << "...creating pipeline..." << std::endl;
        res = vkCreateGraphicsPipelines(logicalDevice, VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo, nullptr, &graphicsPipeline);
        if (res != VK_SUCCESS){
            throw std::runtime_error("failed to create pipeline!");
        }

        vkDestroyShaderModule(logicalDevice, fragShaderModule, nullptr);
        std::cout << "Fragment Shader Destroyed" << std::endl;
        vkDestroyShaderModule(logicalDevice, vertShaderModule, nullptr);
        std::cout << "Vertex Shader Destroyed" << std::endl;
    }

    void createFramebuffers() {
        swapChainFramebuffers.resize(swapChainImageViews.size());

        for (size_t i = 0; i < swapChainImageViews.size(); i++) {
            VkImageView attachments[] = {swapChainImageViews[i]};

            VkFramebufferCreateInfo framebufferCreateInfo = {};
            framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferCreateInfo.renderPass = renderPass;
            framebufferCreateInfo.attachmentCount = 1;
            framebufferCreateInfo.pAttachments = attachments;
            framebufferCreateInfo.width = swapChainExtent.width;
            framebufferCreateInfo.height = swapChainExtent.height;
            framebufferCreateInfo.layers = 1;

            std::cout << "...creating a framebuffer..." << std::endl;
            VkResult res = vkCreateFramebuffer(logicalDevice, &framebufferCreateInfo, nullptr, &swapChainFramebuffers[i]);
            if (res != VK_SUCCESS) {
                throw std::runtime_error("failed to create a framebuffer!");
            }
        }
    }

    void createCommandPool() {
        QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);

        VkCommandPoolCreateInfo commandPoolCreateInfo = {};
        commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        commandPoolCreateInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
        //commandPoolCreateInfo.flags = 0;

        std::cout << "...creating command pool..." << std::endl;
        VkResult res = vkCreateCommandPool(logicalDevice, &commandPoolCreateInfo, nullptr, &commandPool);
        if (res != VK_SUCCESS) {
            throw std::runtime_error("failed to create command pool!");
        }
    }

    // TODO: REFACTORING!!!
    void createTextureImage(const char* uvMSFilename, const char* uvLSFilename) {
        //loadTexture("textures/identityUVMS.png", uvMSTextureImage, uvMSTextureImageMemory);
        int uvMSTexWidth, uvMSTexHeight, uvMSTexChannels;
        //!!! Modify down here for changing warping effect
        stbi_uc* uvMSPixels = stbi_load(uvMSFilename, &uvMSTexWidth, &uvMSTexHeight, &uvMSTexChannels, STBI_rgb_alpha);
        VkDeviceSize uvMSImageSize = uvMSTexWidth * uvMSTexHeight * 4;

        if (!uvMSPixels) {
            //throw std::runtime_error(strcat(strcat("failed to load ", filename), "texture image!"));
            throw std::runtime_error("failed to load uv MS texture image!");
        }

        VkBuffer uvMSStagingBuffer;
        VkDeviceMemory uvMSStagingBufferMemory;
        createBuffer(uvMSImageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     uvMSStagingBuffer, uvMSStagingBufferMemory);
        
        void* uvMSData;
        vkMapMemory(logicalDevice, uvMSStagingBufferMemory, 0, uvMSImageSize, 0, &uvMSData);
            memcpy(uvMSData ,uvMSPixels, static_cast<size_t>(uvMSImageSize));
        vkUnmapMemory(logicalDevice, uvMSStagingBufferMemory);

        stbi_image_free(uvMSPixels);

        createImage(uvMSTexWidth, uvMSTexHeight, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, uvMSTextureImage, uvMSTextureImageMemory);
        
        transitionImageLayout(uvMSTextureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
            copyBufferToImage(uvMSStagingBuffer, uvMSTextureImage, static_cast<uint32_t>(uvMSTexWidth), static_cast<uint32_t>(uvMSTexHeight));
        transitionImageLayout(uvMSTextureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        vkDestroyBuffer(logicalDevice, uvMSStagingBuffer, nullptr);
        vkFreeMemory(logicalDevice, uvMSStagingBufferMemory, nullptr);

        //loadTexture("textures/identityUVLS.png", uvLSTextureImage, uvLSTextureImageMemory);##################################################################
        int uvLSTexWidth, uvLSTexHeight, uvLSTexChannels;
        //!!! Modify down here for changing warping effect
        stbi_uc* uvLSPixels = stbi_load(uvLSFilename, &uvLSTexWidth, &uvLSTexHeight, &uvLSTexChannels, STBI_rgb_alpha);
        VkDeviceSize uvLSImageSize = uvLSTexWidth * uvLSTexHeight * 4;

        if (!uvLSPixels) {
            //throw std::runtime_error(strcat(strcat("failed to load ", filename), "texture image!"));
            std::cout << uvLSFilename << std::endl;
            throw std::runtime_error("failed to load uv LS texture image!");
        }

        VkBuffer uvLSStagingBuffer;
        VkDeviceMemory uvLSStagingBufferMemory;
        createBuffer(uvLSImageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     uvLSStagingBuffer, uvLSStagingBufferMemory);
        
        void* uvLSData;
        vkMapMemory(logicalDevice, uvLSStagingBufferMemory, 0, uvLSImageSize, 0, &uvLSData);
            memcpy(uvLSData, uvLSPixels, static_cast<size_t>(uvLSImageSize));
        vkUnmapMemory(logicalDevice, uvLSStagingBufferMemory);

        stbi_image_free(uvLSPixels);

        createImage(uvLSTexWidth, uvLSTexHeight, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, uvLSTextureImage, uvLSTextureImageMemory);
        
        transitionImageLayout(uvLSTextureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
            copyBufferToImage(uvLSStagingBuffer, uvLSTextureImage, static_cast<uint32_t>(uvLSTexWidth), static_cast<uint32_t>(uvLSTexHeight));
        transitionImageLayout(uvLSTextureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        vkDestroyBuffer(logicalDevice, uvLSStagingBuffer, nullptr);
        vkFreeMemory(logicalDevice, uvLSStagingBufferMemory, nullptr);

        //loadTexture("/home/eldomo/Desktop/domeCalibration1k3.jpg", colorTextureImage, colorTextureImageMemory);##############################################
        int colorTexWidth, colorTexHeight, colorTexChannels;
        //VkFormat format;
        stbi_uc* colorPixels;
        //!!! Modify down here for changing warping effect
        if (capture) {
            std::cout << "...screen capture..." << std::endl;
            #if __linux__
                screenCapture = XGetImage(display, root_window, 420, 0, HEIGHT, HEIGHT, AllPlanes, ZPixmap);
                std::cout << "Screen Capture Initialised!" << std::endl;
                colorTexWidth = screenCapture->width;
                colorTexHeight = screenCapture->height;
                colorTexChannels = 4;
                colorPixels = (stbi_uc*) screenCapture->data;
                colorTexFormat = VK_FORMAT_B8G8R8A8_UNORM;
            #endif
        } else {
            colorPixels = stbi_load("/home/eldomo/Desktop/domeCalibration1k3.jpg", &colorTexWidth, &colorTexHeight, &colorTexChannels, STBI_rgb_alpha);
            colorTexFormat = VK_FORMAT_R8G8B8A8_UNORM;
        }
        VkDeviceSize colorImageSize = colorTexWidth * colorTexHeight * 4;

        if (!colorPixels) {
            throw std::runtime_error("failed to load colour texture image!");
        }

        //VkBuffer colorStagingBuffer;
        //VkDeviceMemory colorStagingBufferMemory;
        createBuffer(colorImageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                    colorStagingBuffer, colorStagingBufferMemory);
        
        void* colorData;
        vkMapMemory(logicalDevice, colorStagingBufferMemory, 0, colorImageSize, 0, &colorData);
            memcpy(colorData, colorPixels, static_cast<size_t>(colorImageSize));
        vkUnmapMemory(logicalDevice, colorStagingBufferMemory);

        stbi_image_free(colorPixels);

        createImage(colorTexWidth, colorTexHeight, colorTexFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, colorTextureImage, colorTextureImageMemory);
        
        transitionImageLayout(colorTextureImage, colorTexFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
            copyBufferToImage(colorStagingBuffer, colorTextureImage, static_cast<uint32_t>(colorTexWidth), static_cast<uint32_t>(colorTexHeight));
        transitionImageLayout(colorTextureImage, colorTexFormat, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        //vkDestroyBuffer(logicalDevice, colorStagingBuffer, nullptr);
        //vkFreeMemory(logicalDevice, colorStagingBufferMemory, nullptr);
    }

    void createTextureImageView() {
        uvMSTextureImageView = createImageView(uvMSTextureImage, VK_FORMAT_R8G8B8A8_UNORM);
        uvLSTextureImageView = createImageView(uvLSTextureImage, VK_FORMAT_R8G8B8A8_UNORM);
        colorTextureImageView = createImageView(colorTextureImage, colorTexFormat);
    }

    void createTextureSampler() {
        VkSamplerCreateInfo samplerCreateInfo = {};
        samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerCreateInfo.magFilter = VK_FILTER_LINEAR; // linear or nearest filtering
        samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
        samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT; // repeat/mirrored repeat/clamp to edge/mirrored clamp to edge/clamp to border texture mapping
        samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerCreateInfo.anisotropyEnable = VK_FALSE; // not needed in this application
        samplerCreateInfo.maxAnisotropy = 1;
        samplerCreateInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK; // int or float transparent/opaque black/opaque white
        samplerCreateInfo.unnormalizedCoordinates = VK_FALSE; // when TRUE uv-coords [0, texDim), when FALSE uv-coords [0, 1)
        samplerCreateInfo.compareEnable = VK_FALSE; // for filtering purposes (not useful now)
        samplerCreateInfo.compareOp = VK_COMPARE_OP_ALWAYS; // not used here
        samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR; // not actually useful for this application
        samplerCreateInfo.mipLodBias = 0.0f;
        samplerCreateInfo.minLod = 0.0f;
        samplerCreateInfo.maxLod = 0.0f;

        std::cout << "...creating a texture sampler..." << std::endl;
        VkResult res = vkCreateSampler(logicalDevice, &samplerCreateInfo, nullptr, &textureSampler);
        if (res != VK_SUCCESS){
            throw std::runtime_error("failed to create texture sampler!");
        }
    }

    VkImageView createImageView(VkImage image, VkFormat format) {
        VkImageViewCreateInfo ivCreateInfo = {};
        ivCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        ivCreateInfo.image = image;
        ivCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D; // 1D-/2D-/3D-textures/cube maps
        ivCreateInfo.format = format;
        ivCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        ivCreateInfo.subresourceRange.baseMipLevel = 0;
        ivCreateInfo.subresourceRange.levelCount = 1;
        ivCreateInfo.subresourceRange.baseArrayLayer = 0;
        ivCreateInfo.subresourceRange.layerCount = 1;

        VkImageView imageView;
        std::cout << "...creating an image view..." << std::endl;
        VkResult res = vkCreateImageView(logicalDevice, &ivCreateInfo, nullptr, &imageView);
        if (res != VK_SUCCESS){
            throw std::runtime_error("failed to create image view!");
        }

        return imageView;
    }

    void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties,
                     VkImage& image, VkDeviceMemory& imageMemory) {
        VkImageCreateInfo imageCreateInfo = {};
        imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
        imageCreateInfo.extent.width = width;
        imageCreateInfo.extent.height = height;
        imageCreateInfo.extent.depth = 1;
        imageCreateInfo.mipLevels = 1;
        imageCreateInfo.arrayLayers = 1;
        imageCreateInfo.format = format;
        imageCreateInfo.tiling = tiling;
        imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageCreateInfo.usage = usage;
        imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        //imageCreateInfo.flags = 0;
        imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        std::cout << "...creating image..." << std::endl;
        VkResult res = vkCreateImage(logicalDevice, &imageCreateInfo, nullptr, &image);
        if (res != VK_SUCCESS) {
            throw std::runtime_error("failed to create image!");
        }

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(logicalDevice, image, &memRequirements);

        VkMemoryAllocateInfo memAllocInfo = {};
        memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        memAllocInfo.allocationSize = memRequirements.size;
        memAllocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

        std::cout << "...allocating memory for texture..." << std::endl;
        res = vkAllocateMemory(logicalDevice, &memAllocInfo, nullptr, &imageMemory);
        if (res != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate memory!");
        }

        vkBindImageMemory(logicalDevice, image, imageMemory, 0);
    }

    void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) {
        VkCommandBuffer commandBuffer = beginSingleTimeCommands();

        VkImageMemoryBarrier barrier = {};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        VkPipelineStageFlags srcStage;
        VkPipelineStageFlags dstStage;

        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            
            srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        } else {
            throw std::invalid_argument("unsupported layout transition!");
        }

        vkCmdPipelineBarrier(commandBuffer, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

        endSingleTimeCommands(commandBuffer);
    }

    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
        VkCommandBuffer commandBuffer = beginSingleTimeCommands();

        VkBufferImageCopy region = {};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = {0, 0, 0};
        region.imageExtent = {width, height, 1};

        vkCmdCopyBufferToImage(commandBuffer, buffer ,image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        endSingleTimeCommands(commandBuffer);
    }

    void createVertexBuffer(const std::vector<Vertex> vertices) {
        VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
        std::cout << bufferSize << std::endl;
        
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

        void* data;
        vkMapMemory(logicalDevice, stagingBufferMemory ,0, bufferSize, 0, &data);
            memcpy(data, vertices.data(), (size_t) bufferSize);
        vkUnmapMemory(logicalDevice, stagingBufferMemory);

        createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);
        copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

        vkDestroyBuffer(logicalDevice, stagingBuffer, nullptr);
        vkFreeMemory(logicalDevice, stagingBufferMemory, nullptr);
    }

    void createIndexBuffer() {
        VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();
        
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

        void* data;
        vkMapMemory(logicalDevice, stagingBufferMemory ,0, bufferSize, 0, &data);
            memcpy(data, indices.data(), (size_t) bufferSize);
        vkUnmapMemory(logicalDevice, stagingBufferMemory);

        createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory);
        copyBuffer(stagingBuffer, indexBuffer, bufferSize);

        vkDestroyBuffer(logicalDevice, stagingBuffer, nullptr);
        vkFreeMemory(logicalDevice, stagingBufferMemory, nullptr);
    }

    void createUniformBuffer() {
        VkDeviceSize bufferSize = sizeof(UniformBufferObject);

        uniformBuffers.resize(swapChainImages.size());
        uniformBuffersMemory.resize(swapChainImages.size());

        for (size_t i = 0; i < swapChainImages.size(); i++) {
            createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                         uniformBuffers[i], uniformBuffersMemory[i]);
        }
    }

    void createDescriptorPool() {
        std::array<VkDescriptorPoolSize, 3> poolSizes = {};
        poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[0].descriptorCount = static_cast<uint32_t>(swapChainImages.size());
        poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizes[1].descriptorCount = static_cast<uint32_t>(swapChainImages.size());
        poolSizes[2].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizes[2].descriptorCount = static_cast<uint32_t>(swapChainImages.size());

        VkDescriptorPoolCreateInfo poolCreateInfo = {};
        poolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolCreateInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolCreateInfo.pPoolSizes = poolSizes.data();
        poolCreateInfo.maxSets = static_cast<uint32_t>(swapChainImages.size());

        std::cout << "...creating descriptor pool..." << std::endl;
        VkResult res = vkCreateDescriptorPool(logicalDevice, &poolCreateInfo, nullptr, &descriptorPool);
        if (res != VK_SUCCESS) {
            throw std::runtime_error("failed to create descripto pool!");
        }
    }

    void createDescriptorSets() {
        std::vector<VkDescriptorSetLayout>layouts(swapChainImages.size(), descriptorSetLayout);
        VkDescriptorSetAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = descriptorPool;
        allocInfo.descriptorSetCount = static_cast<uint32_t>(swapChainImages.size());
        allocInfo.pSetLayouts = layouts.data();

        descriptorSets.resize(swapChainImages.size());
        std::cout << "allocating descriptor sets..." << std::endl;
        VkResult res = vkAllocateDescriptorSets(logicalDevice, &allocInfo, &descriptorSets[0]);
        if (res != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate descriptor sets!");
        }

        for (size_t i = 0; i < swapChainImages.size(); i++) {
            VkDescriptorBufferInfo descriptorBufferInfo = {};
            descriptorBufferInfo.buffer = uniformBuffers[i];
            descriptorBufferInfo.offset = 0;
            descriptorBufferInfo.range = sizeof(UniformBufferObject);

            VkDescriptorImageInfo descriptorUVMSImageInfo = {};
            descriptorUVMSImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            descriptorUVMSImageInfo.imageView = uvMSTextureImageView;
            descriptorUVMSImageInfo.sampler = textureSampler;

            VkDescriptorImageInfo descriptorUVLSImageInfo = {};
            descriptorUVLSImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            descriptorUVLSImageInfo.imageView = uvLSTextureImageView;
            descriptorUVLSImageInfo.sampler = textureSampler;
            
            //VkDescriptorImageInfo descriptorColorImageInfo = {};
            descriptorColorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            descriptorColorImageInfo.imageView = colorTextureImageView;
            descriptorColorImageInfo.sampler = textureSampler;

            //std::array<VkWriteDescriptorSet, 4> writeDescriptorSets = {};
            
            writeDescriptorSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writeDescriptorSets[0].dstSet = descriptorSets[i];
            writeDescriptorSets[0].dstBinding = 0;
            writeDescriptorSets[0].dstArrayElement = 0;
            writeDescriptorSets[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            writeDescriptorSets[0].descriptorCount = 1;
            writeDescriptorSets[0].pBufferInfo = &descriptorBufferInfo;
            //writeDescriptorSets[0].pImageInfo = nullptr;
            //writeDescriptorSets[0].pTexelBufferView = nullptr;

            writeDescriptorSets[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writeDescriptorSets[1].dstSet = descriptorSets[i];
            writeDescriptorSets[1].dstBinding = 1;
            writeDescriptorSets[1].dstArrayElement = 0;
            writeDescriptorSets[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            writeDescriptorSets[1].descriptorCount = 1;
            //writeDescriptorSets[1].pBufferInfo = &descriptorBufferInfo;
            writeDescriptorSets[1].pImageInfo = &descriptorUVMSImageInfo;
            //writeDescriptorSets[1].pTexelBufferView = nullptr;

            writeDescriptorSets[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writeDescriptorSets[2].dstSet = descriptorSets[i];
            writeDescriptorSets[2].dstBinding = 2;
            writeDescriptorSets[2].dstArrayElement = 0;
            writeDescriptorSets[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            writeDescriptorSets[2].descriptorCount = 1;
            //writeDescriptorSets[2].pBufferInfo = &descriptorBufferInfo;
            writeDescriptorSets[2].pImageInfo = &descriptorUVLSImageInfo;
            //writeDescriptorSets[2].pTexelBufferView = nullptr;

            writeDescriptorSets[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writeDescriptorSets[3].dstSet = descriptorSets[i];
            writeDescriptorSets[3].dstBinding = 3;
            writeDescriptorSets[3].dstArrayElement = 0;
            writeDescriptorSets[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            writeDescriptorSets[3].descriptorCount = 1;
            //writeDescriptorSets[3].pBufferInfo = &descriptorBufferInfo;
            writeDescriptorSets[3].pImageInfo = &descriptorColorImageInfo;
            //writeDescriptorSets[3].pTexelBufferView = nullptr;

            vkUpdateDescriptorSets(logicalDevice, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);
        }
    }

    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
        VkBufferCreateInfo bufferCreateInfo = {};
        bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferCreateInfo.size = size;
        bufferCreateInfo.usage = usage;
        bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        std::cout << "...creating vertex buffer..." << size << std::endl;
        VkResult res = vkCreateBuffer(logicalDevice, &bufferCreateInfo, nullptr, &buffer);
        if (res != VK_SUCCESS) {
            throw std::runtime_error("failed to create vertex buffer!");
        }

        VkMemoryRequirements memoryRequirements;
        vkGetBufferMemoryRequirements(logicalDevice, buffer, &memoryRequirements);

        VkMemoryAllocateInfo memoryAllocateInfo = {};
        memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        memoryAllocateInfo.allocationSize = memoryRequirements.size;
        memoryAllocateInfo.memoryTypeIndex = findMemoryType(memoryRequirements.memoryTypeBits,
                                                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        
        if (vkAllocateMemory(logicalDevice, &memoryAllocateInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
            throw std::runtime_error("faild to allocate vertex buffer memory");
        }
        vkBindBufferMemory(logicalDevice, buffer, bufferMemory, 0);
    }

    VkCommandBuffer beginSingleTimeCommands() {
        VkCommandBufferAllocateInfo bufferAllocateInfo = {};
        bufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        bufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        bufferAllocateInfo.commandPool = commandPool;
        bufferAllocateInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer;
        vkAllocateCommandBuffers(logicalDevice, &bufferAllocateInfo, &commandBuffer);

        VkCommandBufferBeginInfo commandBufferBeginInfo = {};
        commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(commandBuffer ,&commandBufferBeginInfo);

        return commandBuffer;
    }

    void endSingleTimeCommands(VkCommandBuffer commandBuffer) {
        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(graphicsQueue); // this could be substituted with a fence (more advanced and flexible)

        vkFreeCommandBuffers(logicalDevice, commandPool, 1, &commandBuffer);
    }

    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
        VkCommandBuffer commandBuffer = beginSingleTimeCommands();
        
        VkBufferCopy copyRegion = {};
        //copyRegion.srcOffset = 0;
        //copyRegion.dstOffset = 0;
        copyRegion.size = size;
        vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
        
        endSingleTimeCommands(commandBuffer);
    }

    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
        VkPhysicalDeviceMemoryProperties memoryProperties;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

        for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++) {
            if ((typeFilter &(1 << i)) && (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }
    }

    void createCommandBuffers() {
        commandBuffers.resize(swapChainFramebuffers.size());

        VkCommandBufferAllocateInfo cbAllocateInfo = {};
        cbAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cbAllocateInfo.commandPool = commandPool;
        cbAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY; // this command buffer can be submitted but not be called from other command buffers
                                                                // secondary level command buffers CANNOT be submitted but can be called from other command buffers
        cbAllocateInfo.commandBufferCount = (uint32_t) commandBuffers.size();

        std::cout << "...creating a command buffer..." << std::endl;
        VkResult res = vkAllocateCommandBuffers(logicalDevice, &cbAllocateInfo, commandBuffers.data());
        if (res != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate command buffers!");
        }

        for (size_t i = 0; i < commandBuffers.size(); i++) {
            VkCommandBufferBeginInfo cbBeginInfo = {};
            cbBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            cbBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT; // command buffer can be resubmitted while it is already waiting for execution (other options: discarded right after execution, secondary command buffer within single render pass)
            //cbBeginInfo.pInheritanceInfo = nullptr; // only relevant for secondary command buffers 
            
            std::cout << "...beginning command buffer recording..." << std::endl;
            VkResult beginRes = vkBeginCommandBuffer(commandBuffers[i], &cbBeginInfo);
            if (beginRes != VK_SUCCESS) {
                throw std::runtime_error("failed to begin recording command buffer!");
            }

            VkRenderPassBeginInfo renderPassBeginInfo = {};
            renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassBeginInfo.renderPass = renderPass;
            renderPassBeginInfo.framebuffer = swapChainFramebuffers[i];
            renderPassBeginInfo.renderArea.offset = {0, 0};
            renderPassBeginInfo.renderArea.extent = swapChainExtent;
            VkClearValue clearColor = {0.0f, 0.0f, 0.0f, 1.0f};
            renderPassBeginInfo.clearValueCount = 1;
            renderPassBeginInfo.pClearValues = &clearColor; // clear values to be used for clearing framebuffer before new render pass (with colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR)
            
            vkCmdBeginRenderPass(commandBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
                vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

                VkBuffer vertexBuffers[] = {vertexBuffer};
                VkDeviceSize offsets[] = {0};
                vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vertexBuffers, offsets);
                vkCmdBindIndexBuffer(commandBuffers[i], indexBuffer, 0, VK_INDEX_TYPE_UINT16);

                vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[i], 0, nullptr);

                vkCmdDrawIndexed(commandBuffers[i], static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
            vkCmdEndRenderPass(commandBuffers[i]);

            std::cout << "...ending command buffer recording..." << std::endl;
            VkResult endRes = vkEndCommandBuffer(commandBuffers[i]);
            if (endRes != VK_SUCCESS) {
                throw std::runtime_error("failed to record command buffer!");
            }
        }
    }

    void createSyncObjs() {
        imgAvailSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

        VkSemaphoreCreateInfo semaphoreCreateInfo = {};
        semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceCreateInfo = {};
        fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        
        std::cout << "...creating semaphores and fences..." << std::endl;
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            VkResult semRes1 = vkCreateSemaphore(logicalDevice, &semaphoreCreateInfo, nullptr, &imgAvailSemaphores[i]);
            VkResult semRes2 = vkCreateSemaphore(logicalDevice, &semaphoreCreateInfo, nullptr, &renderFinishedSemaphores[i]);
            VkResult fenceRes = vkCreateFence(logicalDevice, &fenceCreateInfo, nullptr, &inFlightFences[i]);
            if (semRes1 != VK_SUCCESS || semRes2 != VK_SUCCESS || fenceRes != VK_SUCCESS) {
                throw std::runtime_error("failed to create sync objects for a frame!");
            }
        }
    }

    void updateUniformBuffer(uint32_t currentImage) {
        //static auto startTime = std::chrono::high_resolution_clock::now();

        //auto currentTime = std::chrono::high_resolution_clock::now();
        //float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
        time_t currentTime;
        time(&currentTime);

        frameNumber++;

        if (difftime(currentTime, globalStartTime) >= 1.0f) {
            std::cout << float(frameNumber) / 1.0f << " fps" << std::endl;
            globalStartTime = currentTime;
            //std::cout << frameNumber << std::endl;
            frameNumber = 0;
        }
        //std::cout << difftime(currentTime, globalStartTime) << std::endl;

        // necessary for pipeline but not used
        UniformBufferObject ubo = {};
        ubo.model = glm::mat4(1.0f);
        ubo.view = glm::mat4(1.0f);
        ubo.proj = glm::mat4(1.0f);

        void* data;
        vkMapMemory(logicalDevice, uniformBuffersMemory[currentImage], 0, sizeof(ubo), 0, &data);
            memcpy(data, &ubo, sizeof(ubo));
        vkUnmapMemory(logicalDevice, uniformBuffersMemory[currentImage]);
    }

    void updateScreenCapture() {
        // initialise structures for image
        int colorTexWidth, colorTexHeight, colorTexChannels;
        stbi_uc* colorPixels;
        #if __linux__
            // capture screen XGetImage
            screenCapture = XGetImage(display, root_window, 420, 0, HEIGHT, HEIGHT, AllPlanes, ZPixmap);
            colorTexWidth = screenCapture->width;
            colorTexHeight = screenCapture->height;
            colorTexChannels = 4;
            colorPixels = (stbi_uc*) screenCapture->data;
            colorTexFormat = VK_FORMAT_B8G8R8A8_UNORM;
            VkDeviceSize colorImageSize = colorTexWidth * colorTexHeight * 4;

            // pass to stagingBuffer captured data
            void* colorData;
            vkMapMemory(logicalDevice, colorStagingBufferMemory, 0, colorImageSize, 0, &colorData);
                memcpy(colorData, colorPixels, static_cast<size_t>(colorImageSize));
            vkUnmapMemory(logicalDevice, colorStagingBufferMemory);
            stbi_image_free(colorPixels);

            // update VkImage and, thus, its VkImageView
            transitionImageLayout(colorTextureImage, colorTexFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
                copyBufferToImage(colorStagingBuffer, colorTextureImage, static_cast<uint32_t>(colorTexWidth), static_cast<uint32_t>(colorTexHeight));
            transitionImageLayout(colorTextureImage, colorTexFormat, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        #endif
    }

    void drawFrame() {
        vkWaitForFences(logicalDevice, 1, &inFlightFences[currentFrame], VK_TRUE, std::numeric_limits<uint64_t>::max());

        uint32_t imgIndex;
        VkResult res = vkAcquireNextImageKHR(logicalDevice, swapChain, std::numeric_limits<uint64_t>::max(),
                                            imgAvailSemaphores[currentFrame], VK_NULL_HANDLE, &imgIndex);
        if (res == VK_ERROR_OUT_OF_DATE_KHR) {
            recreateSwapChain();
            return;
        } else if (res != VK_SUCCESS && res != VK_SUBOPTIMAL_KHR) {
            throw std::runtime_error("failed to acquire swap chain image!");
        }

        updateUniformBuffer(imgIndex);
        if (capture) {
            updateScreenCapture();
        }

        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore waitSemaphores[] = {imgAvailSemaphores[currentFrame]};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;

        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffers[imgIndex];

        VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        vkResetFences(logicalDevice, 1, &inFlightFences[currentFrame]);

        res = vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]);
        if (res == VK_SUBOPTIMAL_KHR || res == VK_ERROR_OUT_OF_DATE_KHR || framebufferResized) {
            framebufferResized = false;
            recreateSwapChain();
        } else if (res != VK_SUCCESS) {
            throw std::runtime_error("failed to submit draw command buffer!");
        }

        VkPresentInfoKHR presentInfo = {};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;
        
        VkSwapchainKHR swapChains[] = {swapChain};
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;
        presentInfo.pImageIndices = &imgIndex;
        //presentInfo.pResults = nullptr; // pointer to array of results to match (useful with multiple swap chains)

        vkQueuePresentKHR(presentQueue, &presentInfo);

        vkQueueWaitIdle(presentQueue);

        currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    VkShaderModule createShaderModule(const std::vector<char>& code) {
        VkShaderModuleCreateInfo smCreateInfo = {};
        smCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        smCreateInfo.codeSize = code.size();
        smCreateInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

        VkShaderModule shaderModule;
        VkResult res = vkCreateShaderModule(logicalDevice, &smCreateInfo, nullptr, &shaderModule);
        if (res != VK_SUCCESS){
            throw std::runtime_error("failed to create shader module!");
        }

        return shaderModule;
    }

    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
        if (availableFormats.size() == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED) {
            return {VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
        }
        
        for (const auto& availableFormat : availableFormats) {
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                return availableFormat;
            }
        }

        return availableFormats[0];
    }

    VkPresentModeKHR chooseSwapPresentMode (const std::vector<VkPresentModeKHR> availablePresentModes) {
        VkPresentModeKHR bestMode = VK_PRESENT_MODE_FIFO_KHR;
        
        for (const auto& availablePresentMode : availablePresentModes) {
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
                return availablePresentMode;
            } else if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
                bestMode = availablePresentMode;
            }
        }

        return bestMode;
    }

    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& surfaceCapabilities) {
        if (surfaceCapabilities.currentExtent.width != UINT32_MAX) {
            return surfaceCapabilities.currentExtent;
        } else {
            int width, height;
            glfwGetFramebufferSize(window, &width, &height);

            VkExtent2D actualExtent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};

            actualExtent.width = std::max(surfaceCapabilities.minImageExtent.width, 
                                    std::min(surfaceCapabilities.maxImageExtent.width, actualExtent.width));
            actualExtent.height = std::max(surfaceCapabilities.minImageExtent.height, 
                                    std::min(surfaceCapabilities.maxImageExtent.height, actualExtent.height));
            
            return actualExtent;
        }
    }

    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device) {
        SwapChainSupportDetails details;

        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.surfaceCapabilities);

        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
        if (formatCount != 0) {
            //std::cout << "more than one format modes" << std::endl;
            details.formats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
            //std::cout << "fomats: " << details.formats.size() << std::endl;
        }

        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
        if (presentModeCount != 0) {
            details.presentModes.resize(presentModeCount);
            //std::cout << "present modes: " << details.presentModes.size() << std::endl;
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
            //std::cout << "present modes: " << details.presentModes.size() << "\n" << std::endl;
        }

        return details;
    }

    bool isDeviceSuitable(VkPhysicalDevice device) {
        // here required properties and features can be listed in order to filter out GPU that are not suitable (could lead to runtime issues)
        QueueFamilyIndices indices = findQueueFamilies(device);
        /*if (indices.isComplete()) {
            std::cout << "all required queue families found" << std::endl;
        }*/
        
        bool extensionSupported = checkDeviceExtensionSupport(device);

        bool swapChainAdequate = false;
        if (extensionSupported) {
            //std::cout << "required extension supported" << std::endl;
            SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
            swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
            //std::cout << "swap chain is adequate\n" << std::endl;
        }

        VkPhysicalDeviceFeatures supportedFeatures = {};
        vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

        return indices.isComplete() && extensionSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;
    }

    bool checkDeviceExtensionSupport(VkPhysicalDevice device) {
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

        std::vector<VkExtensionProperties> availableExtension(extensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtension.data());

        std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

        for (const auto& extension : availableExtension) {
            requiredExtensions.erase(extension.extensionName);
        }

        return requiredExtensions.empty();
    }

    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) {
        QueueFamilyIndices indices;
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr); // get the number of queue families
        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data()); // get the data for each found queue family

        int i = 0;
        for (const auto& queueFamily : queueFamilies) {
            if (queueFamilyCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                indices.graphicsFamily = i;
                //std::cout << "graphics Family queue found" << std::endl;
            }

            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

            if (queueFamily.queueCount > 0 && presentSupport) {
                indices.presentFamily = i;
                //std::cout << "present Family queue found" << std::endl;
            }

            if (indices.isComplete()) {
                break;
            }

            i++;
        }

        return indices;
    }

    bool checkValidationLayerSupport() {
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        for (const char* layerName : validationLayers) {
            bool layerFound = false;

            for(const auto& layerProperties : availableLayers) {
                if (strcmp(layerName, layerProperties.layerName) == 0) {
                    layerFound = true;
                    break;
                }
            }

            if (!layerFound) {
                return false;
            }
        }
        return true;
    }

    std::vector<const char*> getRequiredExtensions() {
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

        if (enableValidationLayers) {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
            extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
        }
        return extensions;
    }

    static std::vector<char> readFile(const std::string& filename) {
        std::ifstream file(filename, std::ios::ate | std::ios::binary); // open as binary and At The End (of the file)

        if (!file.is_open()) {
            throw std::runtime_error("failed to open file!");
        }

        size_t fileSize = (size_t) file.tellg(); // size = position of the reading point (since it is ATE)
        std::vector<char> buffer(fileSize);

        file.seekg(0); // moving reading point at the beginning
        file.read(buffer.data(), fileSize);

        file.close();

        return buffer;
    }

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT msgSeverity, VkDebugUtilsMessageTypeFlagsEXT msgType,
                                                        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
        std::cerr << "validation layer" << pCallbackData->pMessage << std::endl;
    return VK_FALSE;
    }

public:
    void run(bool argCapture, const char* uvMSFilename, const char* uvLSFilename, bool fullscreen){
        capture = argCapture;
        initWindow();
        initVulkan(fullscreen, uvMSFilename, uvLSFilename);
        mainLoop();
        cleanup();
    }
};

int main(int argc, char const *argv[]){
    VkWarpApp vkBasicApp;

    try {
        char argCapture[] = "capture";
        char argFull[] = "full";
        //std::cout << argv[argc-1] << std::endl;
        if (strcmp(argCapture, argv[argc-1]) == 0) {
            std::cout << "captureID" << std::endl;
            vkBasicApp.run(true, "textures/identityUVMS.png", "textures/identityUVLS.png", false);
        } else if (argc > 4){
            std::cout << "captureWarp" << std::endl;
            vkBasicApp.run(true, argv[2], argv[3], true);
        } else if (argc > 2 && argc < 4) {
            if (strcmp(argFull, argv[argc-1])) {
                std::cout << "runSimpleWarp" << std::endl;
                vkBasicApp.run(false, argv[argc-2], argv[argc-1], true);
            } else {
                std::cout << "runSimpleWarp" << std::endl;
                vkBasicApp.run(false, argv[argc-2], argv[argc-1], false);
            }
        } else {
            std::cout << "Warp" << std::endl;
            vkBasicApp.run(false, "textures/identityUVMS.png", "textures/identityUVLS.png", false);
        }
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}