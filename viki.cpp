#include <assert.h>
#include <iostream>
#include <vector>
#include <string>

#include <iostream>

#include <glm/glm.hpp>

/* Set platform defines at build time for volk to pick up. */
#if defined(_WIN32)
#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_EXPOSE_NATIVE_WIN32
#define NOMINMAX
#elif defined(__linux__) || defined(__unix__)
#define VK_USE_PLATFORM_XLIB_KHR
#elif defined(__APPLE__)
#define VK_USE_PLATFORM_MACOS_MVK
#define GLFW_EXPOSE_NATIVE_COCOA
#else
#error "Platform not supported by this example."
#endif

#define VOLK_IMPLEMENTATION
#include "volk.h"

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>


#include <algorithm>

#define _DEBUG 1

#define VK_CHECK(call) \
    do {\
        VkResult result_ = (call); \
        assert(result_ == VK_SUCCESS); \
    } while (0)

#ifndef ARRAYSIZE
#define ARRAYSIZE(array) (sizeof(array) / sizeof((array)[0]))
#endif

VkInstance createInstance() {
    uint32_t instance_layer_count;
    VK_CHECK(vkEnumerateInstanceLayerProperties(&instance_layer_count, nullptr));
    std::cout << instance_layer_count << " layers found!\n";
    if (instance_layer_count > 0) {
        std::unique_ptr<VkLayerProperties[]> instance_layers(new VkLayerProperties[instance_layer_count]);
        VK_CHECK(vkEnumerateInstanceLayerProperties(&instance_layer_count, instance_layers.get()));
        for (int i = 0; i < instance_layer_count; ++i) {
            std::cout << instance_layers[i].layerName << "\n";
        }
    }

    std::vector<const char*> layers = { 
        "VK_LAYER_LUNARG_standard_validation",
    };
    std::vector<const char*> extensions = {
        VK_KHR_SURFACE_EXTENSION_NAME,
#ifdef VK_USE_PLATFORM_WIN32_KHR
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#endif
#ifdef VK_USE_PLATFORM_MACOS_MVK
        VK_MVK_MACOS_SURFACE_EXTENSION_NAME,
#endif
        VK_EXT_DEBUG_REPORT_EXTENSION_NAME
    };
    uint32_t instance_extension_count;
    vkEnumerateInstanceExtensionProperties(NULL, &instance_extension_count, nullptr);
    std::cout << instance_extension_count << " extensions found!\n";
    if (instance_extension_count > 0) {
        std::unique_ptr<VkExtensionProperties[]> instance_extensions(new VkExtensionProperties[instance_extension_count]);
        VK_CHECK(vkEnumerateInstanceExtensionProperties(NULL, &instance_extension_count, instance_extensions.get()));
        for (int i = 0; i < instance_extension_count; ++i) {
            std::cout << instance_extensions[i].extensionName << "\n";
        }
    }

    VkApplicationInfo applicationInfo = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
    applicationInfo.pApplicationName = "VIKI";
    applicationInfo.applicationVersion = 0;
    applicationInfo.pEngineName = "VIKI";
    applicationInfo.engineVersion = 0;
    applicationInfo.apiVersion = VK_API_VERSION_1_1;
    
    VkInstanceCreateInfo info = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
    info.pApplicationInfo = &applicationInfo;
    info.enabledLayerCount = layers.size();
    info.ppEnabledLayerNames = layers.data();
    info.enabledExtensionCount = extensions.size();
    info.ppEnabledExtensionNames = extensions.data();

    VkInstance instance = 0;
    VK_CHECK(vkCreateInstance(&info, NULL, &instance));

    return instance;
}

VkBool32 debugReportCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, uint64_t object, size_t location, int32_t messageCode, const char* pLayerPrefix, const char* pMessage, void* pUserData)
{
    const char* type =
        (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
        ? "ERROR"
        : (flags & (VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT))
            ? "WARNING"
            : "INFO";

    char message[4096];
    snprintf(message, ARRAYSIZE(message), "%s: %s\n", type, pMessage);

    printf("%s", message);

#ifdef _WIN32
    OutputDebugStringA(message);
#endif

  //  if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
//        assert(!"Validation error encountered!");

    return VK_FALSE;
}

VkDebugReportCallbackEXT registerDebugCallback(VkInstance instance)
{
    VkDebugReportCallbackCreateInfoEXT createInfo = { VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT };
    createInfo.flags = VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT | VK_DEBUG_REPORT_ERROR_BIT_EXT;
    createInfo.pfnCallback = debugReportCallback;

    VkDebugReportCallbackEXT callback = 0;
    VK_CHECK(vkCreateDebugReportCallbackEXT(instance, &createInfo, 0, &callback));

    return callback;
}

uint32_t getGraphicsFamilyIndex(VkPhysicalDevice physicalDevice)
{
    uint32_t queueCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueCount, 0);

    std::vector<VkQueueFamilyProperties> queues(queueCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueCount, queues.data());

    for (uint32_t i = 0; i < queueCount; ++i)
        if (queues[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
            return i;

    return VK_QUEUE_FAMILY_IGNORED;
}

bool supportsPresentation(VkPhysicalDevice physicalDevice, uint32_t familyIndex)
{
#if defined(VK_USE_PLATFORM_WIN32_KHR)
    return vkGetPhysicalDeviceWin32PresentationSupportKHR(physicalDevice, familyIndex);
#elif defined(VK_USE_PLATFORM_MACOS_MVK)
    return true;
    //return vkGetPhysicalMacDevicePresentationSupport(physicalDevice, familyIndex);
#else
    return true;
#endif
}

VkPhysicalDevice pickPhysicalDevice(VkPhysicalDevice* physicalDevices, uint32_t physicalDeviceCount)
{
    VkPhysicalDevice discrete = 0;
    VkPhysicalDevice fallback = 0;

    for (uint32_t i = 0; i < physicalDeviceCount; ++i)
    {
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(physicalDevices[i], &props);

        printf("GPU%d: %s\n", i, props.deviceName);

        uint32_t familyIndex = getGraphicsFamilyIndex(physicalDevices[i]);
        if (familyIndex == VK_QUEUE_FAMILY_IGNORED)
            continue;

        if (!supportsPresentation(physicalDevices[i], familyIndex))
            continue;

        if (!discrete && props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        {
            discrete = physicalDevices[i];
        }

        if (!fallback)
        {
            fallback = physicalDevices[i];
        }
    }

    VkPhysicalDevice result = discrete ? discrete : fallback;

    if (result)
    {
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(result, &props);

        printf("Selected GPU %s\n", props.deviceName);
    }
    else
    {
        printf("ERROR: No GPUs found\n");
    }

    return result;
}


VkDevice createDevice(VkInstance instance, VkPhysicalDevice physicalDevice, uint32_t* familyIndex)
{
    *familyIndex = 0; // SHORTCUT: this needs to be computed from queue properties // TODO: this produces a validation error
    
    float queuePriorities[] = { 1.0f };
    
    VkDeviceQueueCreateInfo queueInfo = { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
    queueInfo.queueFamilyIndex = *familyIndex;
    queueInfo.queueCount = 1;
    queueInfo.pQueuePriorities = queuePriorities;
    
    const char* extensions[] =
    {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
    
    VkPhysicalDeviceFeatures features = {};
    features.vertexPipelineStoresAndAtomics = true; // TODO: we aren't using this yet? is this a bug in glslang or in validation layers or do I just not understand the spec?

    VkDeviceCreateInfo createInfo = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
    createInfo.queueCreateInfoCount = 1;
    createInfo.pQueueCreateInfos = &queueInfo;
    
    createInfo.ppEnabledExtensionNames = extensions;
    createInfo.enabledExtensionCount = sizeof(extensions) / sizeof(extensions[0]);
    createInfo.pEnabledFeatures = &features;

    VkDevice device = 0;
    VK_CHECK(vkCreateDevice(physicalDevice, &createInfo, 0, &device));
        printf("vkCreateDevice \n");
    
    return device;
}

VkSurfaceKHR createSurface(VkInstance instance, GLFWwindow* window)
{
#if defined(VK_USE_PLATFORM_WIN32_KHR)
    VkWin32SurfaceCreateInfoKHR createInfo = { VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR };
    createInfo.hinstance = GetModuleHandle(0);
    createInfo.hwnd = glfwGetWin32Window(window);
    
    VkSurfaceKHR surface = 0;
    VK_CHECK(vkCreateWin32SurfaceKHR(instance, &createInfo, 0, &surface));
    return surface;
#elif defined (VK_USE_PLATFORM_MACOS_MVK)
    VkSurfaceKHR surface = 0;
    VK_CHECK(glfwCreateWindowSurface(instance, window, nullptr, &surface));
    return surface;

/*    VkMacOSSurfaceCreateInfoMVK createInfo = { VK_STRUCTURE_TYPE_MACOS_SURFACE_CREATE_INFO_MVK };
    auto nswnd = glfwGetCocoaWindow(window);
    createInfo.pView = nswnd;
    
    VkSurfaceKHR surface = 0;
    VK_CHECK(vkCreateMacOSSurfaceMVK(instance, &createInfo, 0, &surface));
    return surface;*/
#elif defined (VK_USE_PLATFORM_METAL_EXT)

#else
//#error Unsupported platform
#endif

}

VkFormat getSwapchainFormat(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
{
    uint32_t formatCount = 0;
    VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, 0));
    assert(formatCount > 0);

    std::vector<VkSurfaceFormatKHR> formats(formatCount);
    VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, formats.data()));

    if (formatCount == 1 && formats[0].format == VK_FORMAT_UNDEFINED)
        return VK_FORMAT_R8G8B8A8_UNORM;

    for (uint32_t i = 0; i < formatCount; ++i)
        if (formats[i].format == VK_FORMAT_R8G8B8A8_UNORM || formats[i].format == VK_FORMAT_B8G8R8A8_UNORM)
            return formats[i].format;

    return formats[0].format;
}

VkSwapchainKHR createSwapchain(VkDevice device, VkSurfaceKHR surface, VkSurfaceCapabilitiesKHR surfaceCaps, uint32_t familyIndex, VkFormat format, uint32_t width, uint32_t height, VkSwapchainKHR oldSwapchain)
{
    VkCompositeAlphaFlagBitsKHR surfaceComposite =
        (surfaceCaps.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR)
        ? VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR
        : (surfaceCaps.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR)
        ? VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR
        : (surfaceCaps.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR)
        ? VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR
        : VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;

    VkSwapchainCreateInfoKHR createInfo = { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
    createInfo.surface = surface;
    createInfo.minImageCount = std::max(2u, surfaceCaps.minImageCount);
    createInfo.imageFormat = format;
    createInfo.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    createInfo.imageExtent.width = std::max(width, surfaceCaps.minImageExtent.width);
    createInfo.imageExtent.height = std::max(height, surfaceCaps.minImageExtent.height);
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    createInfo.queueFamilyIndexCount = 1;
    createInfo.pQueueFamilyIndices = &familyIndex;
    createInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    createInfo.compositeAlpha = surfaceComposite;
    createInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    createInfo.oldSwapchain = oldSwapchain;

    VkSwapchainKHR swapchain = 0;
    VK_CHECK(vkCreateSwapchainKHR(device, &createInfo, 0, &swapchain));

    return swapchain;
}

VkSemaphore createSemaphore(VkDevice device)
{
    VkSemaphoreCreateInfo createInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };

    VkSemaphore semaphore = 0;
    VK_CHECK(vkCreateSemaphore(device, &createInfo, 0, &semaphore));

    return semaphore;
}

VkCommandPool createCommandPool(VkDevice device, uint32_t familyIndex)
{
    VkCommandPoolCreateInfo createInfo = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
    createInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    createInfo.queueFamilyIndex = familyIndex;

    VkCommandPool commandPool = 0;
    VK_CHECK(vkCreateCommandPool(device, &createInfo, 0, &commandPool));

    return commandPool;
}

VkRenderPass createRenderPass(VkDevice device, VkFormat format)
{
    VkAttachmentDescription attachments[1] = {};
    attachments[0].format = format;
    attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[0].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    attachments[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorAttachments = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachments;

    VkRenderPassCreateInfo createInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
    createInfo.attachmentCount = sizeof(attachments) / sizeof(attachments[0]);
    createInfo.pAttachments = attachments;
    createInfo.subpassCount = 1;
    createInfo.pSubpasses = &subpass;

    VkRenderPass renderPass = 0;
    VK_CHECK(vkCreateRenderPass(device, &createInfo, 0, &renderPass));

    return renderPass;
}

VkFramebuffer createFramebuffer(VkDevice device, VkRenderPass renderPass, VkImageView imageView, uint32_t width, uint32_t height)
{
    VkFramebufferCreateInfo createInfo = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
    createInfo.renderPass = renderPass;
    createInfo.attachmentCount = 1;
    createInfo.pAttachments = &imageView;
    createInfo.width = width;
    createInfo.height = height;
    createInfo.layers = 1;

    VkFramebuffer framebuffer = 0;
    VK_CHECK(vkCreateFramebuffer(device, &createInfo, 0, &framebuffer));

    return framebuffer;
}

VkImageView createImageView(VkDevice device, VkImage image, VkFormat format)
{
    VkImageViewCreateInfo createInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
    createInfo.image = image;
    createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    createInfo.format = format;
    createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    createInfo.subresourceRange.levelCount = 1;
    createInfo.subresourceRange.layerCount = 1;

    VkImageView view = 0;
    VK_CHECK(vkCreateImageView(device, &createInfo, 0, &view));

    return view;
}

VkShaderModule loadShader(VkDevice device, const char* path)
{
    FILE* file = fopen(path, "rb");
    assert(file);

    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    assert(length >= 0);
    fseek(file, 0, SEEK_SET);

    char* buffer = new char[length];
    assert(buffer);

    size_t rc = fread(buffer, 1, length, file);
    assert(rc == size_t(length));
    fclose(file);

    VkShaderModuleCreateInfo createInfo = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
    createInfo.codeSize = length;
    createInfo.pCode = reinterpret_cast<const uint32_t*>(buffer);

    VkShaderModule shaderModule = 0;
    VK_CHECK(vkCreateShaderModule(device, &createInfo, 0, &shaderModule));

    delete[] buffer;

    return shaderModule;
}

struct Camera {
    glm::vec3 _eye;     float _focal;
    glm::vec3 _right;   float _sensorHeight;
    glm::vec3 _up;      float _aspectRatio;
    glm::vec3 _back;   float _spare;

};


VkPipelineLayout createPipelineLayout(VkDevice device, VkDescriptorSetLayout& descriptorSetLayout)
{
    VkDescriptorSetLayoutBinding setBindings[1] = {};
//	setBindings[0].binding = 0;
//	setBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
//	setBindings[0].descriptorCount = 1;
//	setBindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
//	setBindings[0].pImmutableSamplers = nullptr; 
    
    setBindings[0].binding = 0;
    setBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    setBindings[0].descriptorCount = 1;
    setBindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    setBindings[0].pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo setCreateInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
    setCreateInfo.flags = 0;
 //   setCreateInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR;
 //   setCreateInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT_EXT;
    setCreateInfo.bindingCount = ARRAYSIZE(setBindings);
    setCreateInfo.pBindings = setBindings;

    VK_CHECK(vkCreateDescriptorSetLayout(device, &setCreateInfo, 0, &descriptorSetLayout));

    VkPipelineLayoutCreateInfo createInfo = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
    createInfo.setLayoutCount = 1;
    createInfo.pSetLayouts = &descriptorSetLayout;


    VkPipelineLayout layout = 0;
    VK_CHECK(vkCreatePipelineLayout(device, &createInfo, 0, &layout));

    // TODO: is this safe?
  //  vkDestroyDescriptorSetLayout(device, setLayout, 0);

    return layout;
}

VkPipeline createGraphicsPipeline(VkDevice device, VkPipelineCache pipelineCache, VkRenderPass renderPass, VkShaderModule vs, VkShaderModule fs, VkPipelineLayout layout)
{
    VkGraphicsPipelineCreateInfo createInfo = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
 
    VkPipelineShaderStageCreateInfo stages[2] = {};
    stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = vs;
    stages[0].pName = "main";
    stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = fs;
    stages[1].pName = "main";

    createInfo.stageCount = sizeof(stages) / sizeof(stages[0]);
    createInfo.pStages = stages;

    VkPipelineVertexInputStateCreateInfo vertexInput = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
    createInfo.pVertexInputState = &vertexInput;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
    createInfo.pInputAssemblyState = &inputAssembly;

    VkPipelineViewportStateCreateInfo viewportState = { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;
    createInfo.pViewportState = &viewportState;

    VkPipelineRasterizationStateCreateInfo rasterizationState = { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
    rasterizationState.lineWidth = 1.f;
    createInfo.pRasterizationState = &rasterizationState;

    VkPipelineMultisampleStateCreateInfo multisampleState = { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
    multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    createInfo.pMultisampleState = &multisampleState;

    VkPipelineDepthStencilStateCreateInfo depthStencilState = { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
    createInfo.pDepthStencilState = &depthStencilState;

    VkPipelineColorBlendAttachmentState colorAttachmentState = {};
    colorAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo colorBlendState = { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
    colorBlendState.attachmentCount = 1;
    colorBlendState.pAttachments = &colorAttachmentState;
    createInfo.pColorBlendState = &colorBlendState;

    VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

    VkPipelineDynamicStateCreateInfo dynamicState = { VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
    dynamicState.dynamicStateCount = sizeof(dynamicStates) / sizeof(dynamicStates[0]);
    dynamicState.pDynamicStates = dynamicStates;
    createInfo.pDynamicState = &dynamicState;

    createInfo.layout = layout;
    createInfo.renderPass = renderPass;

    VkPipeline pipeline = 0;
    VK_CHECK(vkCreateGraphicsPipelines(device, pipelineCache, 1, &createInfo, 0, &pipeline));

    return pipeline;
}
struct Swapchain
{
    VkSwapchainKHR swapchain;

    std::vector<VkImage> images;
    std::vector<VkImageView> imageViews;
    std::vector<VkFramebuffer> framebuffers;

    uint32_t width, height;
    uint32_t imageCount;
};

void createSwapchain(Swapchain& result, VkPhysicalDevice physicalDevice, VkDevice device, VkSurfaceKHR surface,  uint32_t familyIndex, VkFormat format, VkRenderPass renderPass, VkSwapchainKHR oldSwapchain = 0)
{
    VkSurfaceCapabilitiesKHR surfaceCaps;
    VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCaps));

    uint32_t width = surfaceCaps.currentExtent.width;
    uint32_t height = surfaceCaps.currentExtent.height;

    VkSwapchainKHR swapchain = createSwapchain(device, surface, surfaceCaps, familyIndex, format, width, height, oldSwapchain);
    assert(swapchain);

    uint32_t imageCount = 0;
    VK_CHECK(vkGetSwapchainImagesKHR(device, swapchain, &imageCount, 0));

    std::vector<VkImage> images(imageCount);
    VK_CHECK(vkGetSwapchainImagesKHR(device, swapchain, &imageCount, images.data()));

    std::vector<VkImageView> imageViews(imageCount);
    for (uint32_t i = 0; i < imageCount; ++i)
    {
        imageViews[i] = createImageView(device, images[i], format);
        assert(imageViews[i]);
    }

    std::vector<VkFramebuffer> framebuffers(imageCount);
    for (uint32_t i = 0; i < imageCount; ++i)
    {
        framebuffers[i] = createFramebuffer(device, renderPass, imageViews[i], width, height);
        assert(framebuffers[i]);
    }

    result.swapchain = swapchain;

    result.images = images;
    result.imageViews = imageViews;
    result.framebuffers = framebuffers;

    result.width = width;
    result.height = height;
    result.imageCount = imageCount;
}

void destroySwapchain(VkDevice device, const Swapchain& swapchain)
{
    for (uint32_t i = 0; i < swapchain.imageCount; ++i)
        vkDestroyFramebuffer(device, swapchain.framebuffers[i], 0);

    for (uint32_t i = 0; i < swapchain.imageCount; ++i)
        vkDestroyImageView(device, swapchain.imageViews[i], 0);

    vkDestroySwapchainKHR(device, swapchain.swapchain, 0);
}

void resizeSwapchainIfNecessary(Swapchain& result, VkPhysicalDevice physicalDevice, VkDevice device, VkSurfaceKHR surface, uint32_t familyIndex, VkFormat format, VkRenderPass renderPass)
{
    VkSurfaceCapabilitiesKHR surfaceCaps;
    VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCaps));

    uint32_t newWidth = surfaceCaps.currentExtent.width;
    uint32_t newHeight = surfaceCaps.currentExtent.height;

    if (result.width == newWidth && result.height == newHeight)
        return;

    Swapchain old = result;

    createSwapchain(result, physicalDevice, device, surface, familyIndex, format, renderPass, old.swapchain);

    VK_CHECK(vkDeviceWaitIdle(device));

    destroySwapchain(device, old);
}

struct Vertex
{
    float vx, vy, vz;
    float nx, ny, nz;
};

struct Mesh
{
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
};

bool loadMesh(Mesh& result)
{
    /*!
    * The vertex data for a colored cube. Position data is interleaved with
    * color data.
    */
    static const float vertexData[] =
    {
        // left-bottom-front
        -1.f, -1.f, -1.f,
        0.f,  0.f,  1.f,

        // right-bottom-front
        1.f, -1.f, -1.f,
        0.f,  0.f,  0.f,

        // right-top-front
        1.f,  1.f, -1.f,
        0.f,  1.f,  1.f,

        // left-top-front
        -1.f,  1.f, -1.f,
        0.f,  1.f,  0.f,

        // left-bottom-back
        -1.f, -1.f,  1.f,
        1.f,  0.f,  1.f,

        // right-bottom-back
        1.f, -1.f,  1.f,
        1.f,  0.f,  0.f,

        // right-top-back
        1.f,  1.f,  1.f,
        1.f,  1.f,  1.f,

        // left-top-back
        -1.f,  1.f,  1.f,
        1.f,  1.f,  0.f,
    };

    /*!
    * The index data for a cube drawn using triangles.
    */
    static const uint32_t triIndices[] =
    {
        0,1,2,
        2,0,3,

        1,5,6,
        6,1,2,

        3,2,6,
        6,3,7,

        4, 0, 3,
        3, 4, 7,

        4, 5, 1,
        1, 4, 0,

        5, 4, 7,
        7, 5, 6,
    };


    std::vector<Vertex> vertices(ARRAYSIZE(vertexData) / 6);
    memcpy(vertices.data(), vertexData, ARRAYSIZE(vertexData) * sizeof(float));

    std::vector<uint32_t> indices(ARRAYSIZE(triIndices));
    memcpy(indices.data(), triIndices, ARRAYSIZE(triIndices) * sizeof(uint32_t));
     
    result.vertices = vertices;
    result.indices = indices;

    return true;
}

struct Buffer
{
    VkBuffer buffer;
    VkDeviceMemory memory;
    void* data;
    size_t size;
    VkMemoryRequirements mem_reqs;
};

uint32_t selectMemoryType(const VkPhysicalDeviceMemoryProperties& memoryProperties, uint32_t memoryTypeBits, VkMemoryPropertyFlags flags)
{
    for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i)
        if ((memoryTypeBits & (1 << i)) != 0 && (memoryProperties.memoryTypes[i].propertyFlags & flags) == flags)
            return i;

    assert(!"No compatible memory type found");
    return ~0u;
}

void createBuffer(Buffer& result, VkDevice device, const VkPhysicalDeviceMemoryProperties& memoryProperties, size_t size, VkBufferUsageFlags usage)
{
    VkBufferCreateInfo createInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    createInfo.size = size;
    createInfo.usage = usage;
    createInfo.flags = 0;
    createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    VkBuffer buffer = 0;
    VK_CHECK(vkCreateBuffer(device, &createInfo, 0, &buffer));

    VkMemoryRequirements memoryRequirements;
    vkGetBufferMemoryRequirements(device, buffer, &memoryRequirements);

    uint32_t memoryTypeIndex = selectMemoryType(memoryProperties, memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    assert(memoryTypeIndex != ~0u);

    VkMemoryAllocateInfo allocateInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    allocateInfo.allocationSize = memoryRequirements.size;
    allocateInfo.memoryTypeIndex = memoryTypeIndex;

    VkDeviceMemory memory = 0;
    VK_CHECK(vkAllocateMemory(device, &allocateInfo, 0, &memory));

    VK_CHECK(vkBindBufferMemory(device, buffer, memory, 0));

    void* data = 0;
    VK_CHECK(vkMapMemory(device, memory, 0, size, 0, &data));

    result.buffer = buffer;
    result.memory = memory;
    result.data = data;
    result.size = size;
    result.mem_reqs = memoryRequirements;
}

void destroyBuffer(const Buffer& buffer, VkDevice device)
{
    vkFreeMemory(device, buffer.memory, 0);
    vkDestroyBuffer(device, buffer.buffer, 0);
}

struct UniformBuffer
{
    std::vector<Buffer> buffers;

    uint32_t bufferCount;

    VkDescriptorPool descriptorPool;
    std::vector<VkDescriptorSet> descriptorSets;
};

VkDescriptorPool createUniformDescriptorPool(uint32_t count, VkDevice device) {
    VkDescriptorPoolSize poolSize = {};
    poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize.descriptorCount = count;

    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = count;

    VkDescriptorPool descriptorPool;
    VK_CHECK(vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool));

    return descriptorPool;
}


void createUniformBuffer(UniformBuffer& ubo, uint32_t numBuffers, VkDevice device, const VkPhysicalDeviceMemoryProperties& memoryProperties, size_t bufferSize, VkDescriptorSetLayout& descriptorSetLayout) {
    ubo.buffers.resize(numBuffers);

    for (size_t i = 0; i < numBuffers; i++) {
        createBuffer(ubo.buffers[i], device, memoryProperties, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
    }

    ubo.descriptorPool = createUniformDescriptorPool(numBuffers, device);

    std::vector<VkDescriptorSetLayout> layouts(numBuffers, descriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = ubo.descriptorPool;
    allocInfo.descriptorSetCount = numBuffers;
    allocInfo.pSetLayouts = layouts.data();

    std::vector<VkDescriptorSet> descriptorSets(numBuffers);
    VK_CHECK(vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()));
    ubo.descriptorSets = descriptorSets;
    
    for (int i = 0; i < numBuffers; i++) {
        VkDescriptorBufferInfo bufferInfo = {};
        bufferInfo.buffer = ubo.buffers[i].buffer;
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(bufferSize);
        
        VkWriteDescriptorSet descriptorWrite = {};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = descriptorSets[i];
        descriptorWrite.dstBinding = 0;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrite.descriptorCount = 1;
        
        descriptorWrite.pBufferInfo = &bufferInfo;
        descriptorWrite.pImageInfo = nullptr; // Optional
        descriptorWrite.pTexelBufferView = nullptr; // Optional
        
        vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);
    }
}

template <typename T> void updateUniformBuffer(VkDevice device, uint32_t index, UniformBuffer& ubo, T& data) {
//    void* dstdata = 0;
  //  vkMapMemory(device, ubo.buffers[index].memory, 0, ubo.buffers[index].mem_reqs.size, 0, &dstdata);
      //  memcpy(dstdata, &data, sizeof(T));
   // vkUnmapMemory(device, ubo.buffers[index].memory);

    memcpy(ubo.buffers[index].data, &data, sizeof(T));
    
}

VkImageMemoryBarrier imageBarrier(VkImage image, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, VkImageLayout oldLayout, VkImageLayout newLayout)
{
    VkImageMemoryBarrier result = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };

    result.srcAccessMask = srcAccessMask;
    result.dstAccessMask = dstAccessMask;
    result.oldLayout = oldLayout;
    result.newLayout = newLayout;
    result.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    result.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    result.image = image;
    result.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    result.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
    result.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

    return result;
}

int main(int argc, const char * argv[]) {

    int rc = glfwInit();
    assert(rc);

    std::cout << "Create Viki... \n";
    VK_CHECK(volkInitialize());

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    VkInstance instance = createInstance();
    assert(instance);

    volkLoadInstance(instance);

    VkDebugReportCallbackEXT debugCallback = registerDebugCallback(instance);

    VkPhysicalDevice physicalDevices[16];
    uint32_t physicalDeviceCount = sizeof(physicalDevices) / sizeof(physicalDevices[0]);
    VK_CHECK(vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices));
    
    VkPhysicalDevice physicalDevice = pickPhysicalDevice(physicalDevices, physicalDeviceCount);
    assert(physicalDevice);
    
    uint32_t familyIndex = getGraphicsFamilyIndex(physicalDevice);
    assert(familyIndex != VK_QUEUE_FAMILY_IGNORED);

    VkDevice device = createDevice(instance, physicalDevice, &familyIndex);
    assert(device);

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    auto window = glfwCreateWindow(800, 400, "viki", nullptr, nullptr);
    assert(window);

    VkSurfaceKHR surface = createSurface(instance, window);
    assert(surface);

    VkBool32 presentSupported = 0;
    VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, familyIndex, surface, &presentSupported));
    assert(presentSupported);

    VkFormat swapchainFormat = getSwapchainFormat(physicalDevice, surface);

    VkSemaphore acquireSemaphore = createSemaphore(device);
    assert(acquireSemaphore);

    VkSemaphore releaseSemaphore = createSemaphore(device);
    assert(releaseSemaphore);

    VkQueue queue = 0;
    vkGetDeviceQueue(device, familyIndex, 0, &queue);

    VkRenderPass renderPass = createRenderPass(device, swapchainFormat);
    assert(renderPass);

    Swapchain swapchain;
    createSwapchain(swapchain, physicalDevice, device, surface, familyIndex, swapchainFormat, renderPass);

    VkShaderModule triangleVS = loadShader(device, "../shaders/triangle.vert.glsl.spv");
    assert(triangleVS);

    VkShaderModule triangleFS = loadShader(device, "../shaders/triangle.frag.glsl.spv");
    assert(triangleFS);

    // TODO: this is critical for performance!
    VkPipelineCache pipelineCache = 0;

    VkDescriptorSetLayout triangleDescriptorSetLayout;
    VkPipelineLayout triangleLayout = createPipelineLayout(device, triangleDescriptorSetLayout);
    assert(triangleLayout);

    VkPipeline trianglePipeline = createGraphicsPipeline(device, pipelineCache, renderPass, triangleVS, triangleFS, triangleLayout);
    assert(trianglePipeline);

    VkCommandPool commandPool = createCommandPool(device, familyIndex);
    assert(commandPool);

    VkCommandBufferAllocateInfo allocateInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
    allocateInfo.commandPool = commandPool;
    allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocateInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer = 0;
    VK_CHECK(vkAllocateCommandBuffers(device, &allocateInfo, &commandBuffer));

    VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

    Mesh mesh;
    bool rcm = loadMesh(mesh);

    Buffer vb = {};
    createBuffer(vb, device, memoryProperties, 128 * 1024 * 1024, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    Buffer ib = {};
    createBuffer(ib, device, memoryProperties, 128 * 1024 * 1024, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);

    assert(vb.size >= mesh.vertices.size() * sizeof(Vertex));
    memcpy(vb.data, mesh.vertices.data(), mesh.vertices.size() * sizeof(Vertex));

    assert(ib.size >= mesh.indices.size() * sizeof(uint32_t));
    memcpy(ib.data, mesh.indices.data(), mesh.indices.size() * sizeof(uint32_t));

    
    glm::vec3 forward = glm::normalize(glm::vec3(0.05, -0.05, -1.0));
    glm::vec3 worldUp = glm::vec3(0.0, 1.0, 0.0);
    glm::vec3 right = glm::normalize(glm::cross(worldUp, -forward));
    glm::vec3 up = glm::normalize(glm::cross(-forward, right));
    
    Camera cam;
    cam._eye = glm::vec3( 0.1f, 0.5f, -0.2f);
    cam._right = right;
    cam._up = up;
    cam._back = -forward;
    cam._focal = 0.036f;
    cam._sensorHeight = 0.056f;
    cam._aspectRatio = 16.0f/9.0f;

    
    UniformBuffer cam_ub;
    createUniformBuffer(cam_ub, swapchain.imageCount, device, memoryProperties, sizeof(Camera), triangleDescriptorSetLayout);

    while (!glfwWindowShouldClose(window)) {
        // Poll for and process events 
        glfwPollEvents();
 
        resizeSwapchainIfNecessary(swapchain, physicalDevice, device, surface, familyIndex, swapchainFormat, renderPass);

        uint32_t imageIndex = 0;
        VK_CHECK(vkAcquireNextImageKHR(device, swapchain.swapchain, ~0ull, acquireSemaphore, VK_NULL_HANDLE, &imageIndex));

        VK_CHECK(vkResetCommandPool(device, commandPool, 0));

        VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        VK_CHECK(vkBeginCommandBuffer(commandBuffer, &beginInfo));

        VkImageMemoryBarrier renderBeginBarrier = imageBarrier(swapchain.images[imageIndex], 0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, 0, 0, 0, 1, &renderBeginBarrier);

        VkClearColorValue color = { 48.f / 255.f, 10.f / 255.f, 36.f / 255.f, 1 };
        VkClearValue clearColor = { color };

        VkRenderPassBeginInfo passBeginInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
        passBeginInfo.renderPass = renderPass;
        passBeginInfo.framebuffer = swapchain.framebuffers[imageIndex];
        passBeginInfo.renderArea.extent.width = swapchain.width;
        passBeginInfo.renderArea.extent.height = swapchain.height;
        passBeginInfo.clearValueCount = 1;
        passBeginInfo.pClearValues = &clearColor;

        vkCmdBeginRenderPass(commandBuffer, &passBeginInfo, VK_SUBPASS_CONTENTS_INLINE);



        updateUniformBuffer(device, imageIndex, cam_ub, cam);

        VkViewport viewport = { 0, 0, float(swapchain.width), float(swapchain.height), 0, 1 };
        VkRect2D scissor = { {0, 0}, {uint32_t(swapchain.width), uint32_t(swapchain.height)} };

        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, trianglePipeline);
        
        VkDescriptorBufferInfo bufferInfo = {};
        bufferInfo.buffer = vb.buffer;
        bufferInfo.offset = 0;
        bufferInfo.range = vb.size;

        /*VkWriteDescriptorSet descriptors[1] = {};
        descriptors[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptors[0].dstBinding = 0;
        descriptors[0].descriptorCount = 1;
        descriptors[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        descriptors[0].pBufferInfo = &bufferInfo;

        vkCmdPushDescriptorSetKHR(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, triangleLayout, 0, ARRAYSIZE(descriptors), descriptors);

        vkCmdBindIndexBuffer(commandBuffer, ib.buffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(commandBuffer, uint32_t(mesh.indices.size()), 1, 0, 0, 0);
       */
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, triangleLayout, 0, 1, &cam_ub.descriptorSets[imageIndex], 0, nullptr);
        vkCmdDraw(commandBuffer, 4, 1, 0, 0);

    
        vkCmdEndRenderPass(commandBuffer);

        VkImageMemoryBarrier renderEndBarrier = imageBarrier(swapchain.images[imageIndex], VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, 0, 0, 0, 1, &renderEndBarrier);

        VK_CHECK(vkEndCommandBuffer(commandBuffer));

        VkPipelineStageFlags submitStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

        VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = &acquireSemaphore;
        submitInfo.pWaitDstStageMask = &submitStageMask;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &releaseSemaphore;

        VK_CHECK(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));

        VkPresentInfoKHR presentInfo = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &releaseSemaphore;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &swapchain.swapchain;;
        presentInfo.pImageIndices = &imageIndex;

        VK_CHECK(vkQueuePresentKHR(queue, &presentInfo));

        VK_CHECK(vkDeviceWaitIdle(device));
    }

    VK_CHECK(vkDeviceWaitIdle(device));

    
    vkDestroyCommandPool(device, commandPool, 0);

    destroyBuffer(vb, device);
    destroyBuffer(ib, device);
    destroySwapchain(device, swapchain);

    vkDestroyPipeline(device, trianglePipeline, 0);
    vkDestroyPipelineLayout(device, triangleLayout, 0);

    vkDestroyShaderModule(device, triangleFS, 0);
    vkDestroyShaderModule(device, triangleVS, 0);

    vkDestroyRenderPass(device, renderPass, 0);

    vkDestroySemaphore(device, releaseSemaphore, 0);
    vkDestroySemaphore(device, acquireSemaphore, 0);

    
    vkDestroySurfaceKHR(instance, surface, 0);

    glfwDestroyWindow(window);
    glfwTerminate();

    vkDestroyDevice(device, 0);

    vkDestroyDebugReportCallbackEXT(instance, debugCallback, 0);

    vkDestroyInstance(instance, nullptr);
    return 0;
}
