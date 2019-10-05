#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <set>
#include <array>
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <functional>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <vulkan/vk_sdk_platform.h>
#include <vulkan/vulkan.hpp>

const int WIDTH = 800;
const int HEIGHT = 600;

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
    //glm::vec3 color;

    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription bindingDescr = {};
        bindingDescr.binding = 0;
        bindingDescr.stride = sizeof(Vertex);
        bindingDescr.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescr;
    }

    static std::array<VkVertexInputAttributeDescription, 1> getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 1> attrDesrc = {};
        
        attrDesrc[0].binding = 0;
        attrDesrc[0].location = 0;
        attrDesrc[0].format = VK_FORMAT_R32G32_SFLOAT;
        attrDesrc[0].offset = offsetof(Vertex, pos);

        //attrDescr for colours etc. here
    } 
};

const std::vector<Vertex> vertices = {
    {{-0.5f, -0.5f} /*, {color_here}*/},
    {{ 0.5f, -0.5f} /*, {color_here}*/},
    {{ 0.5f,  0.5f} /*, {color_here}*/}
};

class VkWarpApp {
private:
    bool framebufferResized = false;
    size_t currentFrame = 0;
    GLFWwindow* window;
    VkInstance instance = 0;
    VkDebugUtilsMessengerEXT debugMessenger;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice logicalDevice;
    VkQueue graphicsQueue;
    VkQueue presentQueue;
    VkSurfaceKHR surface;
    VkSwapchainKHR swapChain;
    std::vector<VkImage> swapChainImages;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
    std::vector<VkImageView> swapChainImageViews;
    std::vector<VkFramebuffer> swapChainFramebuffers;
    VkRenderPass renderPass;
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;
    VkCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;
    std::vector<VkSemaphore> imgAvailSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;

    // Initialising GLFW instance, attributes and creating window
    void initWindow() {
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

    void initVulkan() {
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
        createGraphicsPipeline();
        std::cout << "Graphics Pipeline Created" << std::endl;
        createFramebuffers();
        std::cout << "Framebuffers Created" << std::endl;
        createCommandPool();
        std::cout << "Command Pool Created" << std::endl;
        createCommandBuffers();
        std::cout << "Command Buffers Created" << std::endl;
        createSyncObjs();
        std::cout << "Semaphores Created" << std::endl;
    }

    void mainLoop() {
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
    }

    // Destroying Vulkan and GLFW instances before exit
    void cleanup() {
        cleanupSwapChain();

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
            VkImageViewCreateInfo ivCreateInfo = {};
            ivCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            ivCreateInfo.image = swapChainImages[i];
            ivCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D; // 1D-/2D-/3D-textures/cube maps
            ivCreateInfo.format = swapChainImageFormat;
            // swizzling (rearranging vector components) colour channels
            ivCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            ivCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            ivCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            ivCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            // defining purpose of image
            ivCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            ivCreateInfo.subresourceRange.baseMipLevel = 0;
            ivCreateInfo.subresourceRange.levelCount = 1;
            ivCreateInfo.subresourceRange.baseArrayLayer = 0;
            ivCreateInfo.subresourceRange.layerCount = 1;

            std::cout << "...creating an image view..." << std::endl;
            VkResult res = vkCreateImageView(logicalDevice, &ivCreateInfo, nullptr, &swapChainImageViews[i]);
            if (res != VK_SUCCESS){
                throw std::runtime_error("failed to create image view!");
            }
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
        pipelineLayoutCreateInfo.setLayoutCount = 0;
        pipelineLayoutCreateInfo.pushConstantRangeCount = 0;

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
        std::cout << "Vertex Shader Destroyed" << std::endl;
        vkDestroyShaderModule(logicalDevice, vertShaderModule, nullptr);
        std::cout << "Fragment Shader Destroyed" << std::endl;
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
                vkCmdDraw(commandBuffers[i], 4, 1, 0, 0);
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
        //here required properties and features can be listed in order to filter out GPU that are not suitable (could lead to runtime issues)
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

        return indices.isComplete() && extensionSupported && swapChainAdequate;
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
    void run(){
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }
};

int main(int argc, char const *argv[]){
    VkWarpApp vkBasicApp;

    try {
        vkBasicApp.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}