#include "vk/inst.h"
#include "log.h"
#include <cstdlib>

namespace vk {

static VKAPI_ATTR VkBool32 VKAPI_CALL debug_cb(
    VkDebugUtilsMessageSeverityFlagBitsEXT sev,
    VkDebugUtilsMessageTypeFlagsEXT,
    const VkDebugUtilsMessengerCallbackDataEXT *data, void *)
{
    (void)sev;
    LOG("[vk] %s", data ? data->pMessage : "?");
    return VK_FALSE;
}

bool inst_t::create() {
    LOGS("loading vulkan-1.dll");
    if (!LoadLibraryA("vulkan-1.dll")) {
        LOGS("vulkan-1.dll not found");
        MessageBox(nullptr,
            L"Vulkan runtime not found.\n\n"
            L"this required vk to run :-).\n"
            L"please install the Vulkan Runtime:\n"
            L"https://vulkan.lunarg.com/",
            L"Vulkan Error", MB_ICONERROR);
        return false;
    }

    LOGS("creating vulkan instance");

    uint32_t n = 0;
    vkEnumerateInstanceLayerProperties(&n, nullptr);
    auto al = std::vector<VkLayerProperties>(n);
    vkEnumerateInstanceLayerProperties(&n, al.data());
    for (auto &l : al) LOG("  layer: %s (%s)", l.layerName, l.description);

    vkEnumerateInstanceExtensionProperties(nullptr, &n, nullptr);
    auto ae = std::vector<VkExtensionProperties>(n);
    vkEnumerateInstanceExtensionProperties(nullptr, &n, ae.data());
    for (auto &e : ae) LOG("  ext: %s (%u)", e.extensionName, e.specVersion);

    auto exts = std::vector<const char *>();
    auto layers = std::vector<const char *>();
    if (ov::g_debug) {
        layers.push_back("VK_LAYER_KHRONOS_validation");
        exts.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        LOGS("validation + debug utils enabled");
    }

    auto ai = VkApplicationInfo{};
    ai.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    ai.pApplicationName = "overlay";
    ai.apiVersion = VK_API_VERSION_1_3;

    auto ci = VkInstanceCreateInfo{};
    ci.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    ci.pApplicationInfo = &ai;
    ci.enabledLayerCount = (uint32_t)layers.size();
    ci.ppEnabledLayerNames = layers.data();
    ci.enabledExtensionCount = (uint32_t)exts.size();
    ci.ppEnabledExtensionNames = exts.data();

    if (vkCreateInstance(&ci, nullptr, &v) != VK_SUCCESS)
        { LOGS("instance creation failed"); return false; }
    LOG("instance created (vk %u.%u.%u)",
        VK_API_VERSION_MAJOR(ai.apiVersion),
        VK_API_VERSION_MINOR(ai.apiVersion),
        VK_API_VERSION_PATCH(ai.apiVersion));

    d = nullptr;
    if (!ov::g_debug) { LOGS("debug mode off, skipping messenger"); return true; }
    auto dci = VkDebugUtilsMessengerCreateInfoEXT{};
    dci.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    dci.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT
        | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
    dci.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
        | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
    dci.pfnUserCallback = debug_cb;
    auto fn = (PFN_vkCreateDebugUtilsMessengerEXT)
        vkGetInstanceProcAddr(v, "vkCreateDebugUtilsMessengerEXT");
    if (fn) fn(v, &dci, nullptr, &d);
    LOG("debug messenger %s", d ? "created" : "not created");
    return true;
}

void inst_t::destroy() {
    if (d) {
        auto fn = (PFN_vkDestroyDebugUtilsMessengerEXT)
            vkGetInstanceProcAddr(v, "vkDestroyDebugUtilsMessengerEXT");
        if (fn) fn(v, d, nullptr);
    }
    vkDestroyInstance(v, nullptr);
    LOGS("vulkan instance destroyed");
}

}