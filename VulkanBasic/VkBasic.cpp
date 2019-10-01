#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <functional>
#include <cstdlib>
#include <cstring>
#include <vulkan/vk_sdk_platform.h>
#include <vulkan/vulkan.hpp>

const int WIDTH = 800;
const int HEIGHT = 600;

const std::vector<const char*> validationLayers = {"VK_LAYER_KHRONOS_validation"};

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
    int graphicsFamily = -1;

    bool isComplete() {
        return graphicsFamily >= 0;
    } 
};

class VkBasic {
private:
    bool validate;
    GLFWwindow* window;
    VkInstance instance = 0;
    VkDebugUtilsMessengerEXT debugMessenger;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    uint32_t enabled_extension_count{0};
    uint32_t enabled_layer_count{0};
    char const *extension_names[64];
    char const *enabled_layers[64];

    // Initialising GLFW instance, attributes and creating window
    void initWindow() {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        window = glfwCreateWindow(WIDTH, HEIGHT, "Basic Vulkan", nullptr, nullptr);
    }

    VkBool32 check_layers(uint32_t check_count, char const *const *const check_names, uint32_t layer_count,
                              VkLayerProperties *layers) {
        for (uint32_t i = 0; i < check_count; i++) {
            VkBool32 found = VK_FALSE;
            for (uint32_t j = 0; j < layer_count; j++) {
                if (!strcmp(check_names[i], layers[j].layerName)) {
                    found = VK_TRUE;
                    break;
                }
            }
            if (!found) {
                fprintf(stderr, "Cannot find layer: %s\n", check_names[i]);
                return 0;
            }
        }
        return VK_TRUE;
    }

    // Initialising Application Metadata, checking supported extensions and initialising Vulkan instance
    void createInstance(){
        if (enableValidationLayers && !checkValidationLayerSupport()) {
            throw std::runtime_error("validation layers requested, but not available!");
        }

        uint32_t instance_extension_count = 0;
        uint32_t instance_layer_count = 0;
        char const *const instance_validation_layers[] = {"VK_LAYER_KHRONOS_validation"};
        VkBool32 validation_found = VK_FALSE;

        if (validate) {
            auto result = vkEnumerateInstanceLayerProperties(&instance_layer_count, static_cast<VkLayerProperties *>(nullptr));
            assert(result == VK_SUCCESS);

            if (instance_layer_count > 0) {
                std::unique_ptr<VkLayerProperties[]> instance_layers(new VkLayerProperties[instance_layer_count]);
                result = vkEnumerateInstanceLayerProperties(&instance_layer_count, instance_layers.get());
                assert(result == VK_SUCCESS);

                validation_found = check_layers(ARRAY_SIZE(instance_validation_layers), instance_validation_layers,
                                                instance_layer_count, instance_layers.get());
                if (validation_found) {
                    enabled_layer_count = ARRAY_SIZE(instance_validation_layers);
                    enabled_layers[0] = "VK_LAYER_KHRONOS_validation";
                }
            }

            if (!validation_found) {
                ERR_EXIT(
                    "vkEnumerateInstanceLayerProperties failed to find required validation layer.\n\n"
                    "Please look at the Getting Started guide for additional information.\n",
                    "vkCreateInstance Failure");
            }
        }

        /*auto const app = VkApplicationInfo {
                         .pApplicationName(APP_SHORT_NAME)
                         .setApplicationVersion(0)
                         .setPEngineName(APP_SHORT_NAME)
                         .setEngineVersion(0)
                         .setApiVersion(VK_API_VERSION_1_0)};
        auto const inst_info = VkInstanceCreateInfo {
                               .pApplicationInfo = &app
                               .enabledLayerCount = enabled_layer_count
                               .setPpEnabledLayerNames(instance_validation_layers)
                               .setEnabledExtensionCount(enabled_extension_count)
                               .setPpEnabledExtensionNames(extension_names)};*/

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
        createInfo.ppEnabledLayerNames = instance_validation_layers;

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
    void initVulkan() {
        createInstance();
        std::cout << "Instance Created" << std::endl;
        setupDebugCallback();
        std::cout << "Debug Callback Setup successful" << std::endl;
        pickPhysicalDevice();
        std::cout << "Physical Device Picked" << std::endl;
    }

    void mainLoop() {
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
        }
        
    }

    // Destroying Vulkan and GLFW instances before exit
    void cleanup() {
        if(enableValidationLayers) {
            DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
            std::cout << "Debug Messenger Destroyed" << std::endl;
        }

        vkDestroyInstance(instance, nullptr);
        std::cout << "Instance Destroyed" << std::endl;
        glfwDestroyWindow(window);
        std::cout << "Window Destroyed" << std::endl;
        glfwTerminate();
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

    void pickPhysicalDevice() {
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

        if (deviceCount == 0) {
            throw std::runtime_error("failed to find GPUs with Vulkan support!");
        }

        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

        for (const auto& device : devices) {
            if (isDeviceSuitable(device)) {
                physicalDevice = device;
                break;
            }
        }

        if (physicalDevice == VK_NULL_HANDLE) {
            throw std::runtime_error("failed to find a suitable GPU");
        }
    }

    bool isDeviceSuitable(VkPhysicalDevice device) {
        //here required properties and features can be listed in order to filter out GPU that are not suitable (could lead to runtime issues)
        QueueFamilyIndices indeces = findQueueFamilies(device);
        return indeces.isComplete();
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
            }

            if (indices.isComplete()) {
                break;
            }

            i++; // this is going to be used later (different version of indeces.isComplete())
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
    VkBasic vkBasicApp;

    try {
        vkBasicApp.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}