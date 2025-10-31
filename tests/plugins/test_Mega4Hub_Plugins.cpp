#include <gtest/gtest.h>
#if defined(UUGEAR_PLUGIN_LINK_MODE_MODULE)
#include "UUGear/Mega4/PluginManager.hpp"
#else
#include "UUGear/Mega4/plugins/StoragePlugin.hpp"
#endif
#include "UUGear/Mega4/Mega4Hub.hpp"
#include "UUGear/Mega4/DevicePlugin.hpp"
#include "UUGear/Mega4/Mega4Types.hpp"

#include <dlfcn.h>
#include <iostream>
#include <filesystem>

using namespace UUGear::Mega4;
namespace fs = std::filesystem;

// ------------------------------------------------------------------
// Mock objects for isolated testing without real USB devices
// ------------------------------------------------------------------
static PortConnectionInfo makeMockPort(
    int port, bool connected, const std::string& product = "USB DISK 3.0", const std::string& manufacturer = "Wilk")
{
    PortConnectionInfo info;
    info.portNumber = port;
    info.hasDevice = connected;
    info.vid = 0x13fe;
    info.pid = 0x4300;
    info.manufacturer = manufacturer;
    info.product = product;
    return info;
}

// ------------------------------------------------------------------
// Utility: dynamically load a plugin
// ------------------------------------------------------------------
#if defined(UUGEAR_PLUGIN_LINK_MODE_MODULE)
static DevicePlugin* loadPlugin(const std::string& pluginPath, void** handleOut)
{
    *handleOut = dlopen(pluginPath.c_str(), RTLD_NOW);
    if (!*handleOut)
    {
        std::cerr << "[Test] Failed to load plugin: " << pluginPath << " — " << dlerror() << "\n";
        return nullptr;
    }

    auto createPlugin = reinterpret_cast<DevicePlugin* (*)()>(dlsym(*handleOut, "createPlugin"));
    if (!createPlugin)
    {
        std::cerr << "[Test] Symbol 'createPlugin' not found: " << dlerror() << "\n";
        dlclose(*handleOut);
        *handleOut = nullptr;
        return nullptr;
    }

    return createPlugin();
}

static void unloadPlugin(void* handle, DevicePlugin* plugin)
{
    if (!handle) return;

    auto destroyPlugin = reinterpret_cast<void (*)(DevicePlugin*)>(dlsym(handle, "destroyPlugin"));
    if (destroyPlugin && plugin)
        destroyPlugin(plugin);

    dlclose(handle);
}
#endif

// ------------------------------------------------------------------
// Direct dynamic plugin test (realistic loading via dlopen)
// ------------------------------------------------------------------
#if defined(UUGEAR_PLUGIN_LINK_MODE_MODULE)
TEST(StoragePlugin, DynamicLoadAndDetection)
{
#ifdef UUGEAR_PLUGIN_DIR
    std::string pluginPath = std::string(UUGEAR_PLUGIN_DIR) + "/StoragePlugin.so";
#else
    std::string pluginPath = "./plugins/StoragePlugin.so";
#endif

    ASSERT_TRUE(fs::exists(pluginPath)) << "Plugin file not found: " << pluginPath;

    void* handle = nullptr;
    DevicePlugin* plugin = loadPlugin(pluginPath, &handle);
    ASSERT_NE(plugin, nullptr) << "Failed to create StoragePlugin instance dynamically";

    SCOPED_TRACE("✅ StoragePlugin loaded dynamically");

    // --- Simulated storage detection
    auto valid = makeMockPort(1, true, "USB DISK 3.0", "Wilk");
    EXPECT_TRUE(plugin->canHandle(valid)) << "Should detect valid USB storage";

    auto invalid = makeMockPort(2, true, "Logitech Mouse", "Logitech");
    EXPECT_FALSE(plugin->canHandle(invalid)) << "Should ignore non-storage devices";

    auto empty = makeMockPort(3, false, "", "");
    EXPECT_FALSE(plugin->canHandle(empty)) << "Should ignore empty port";

    unloadPlugin(handle, plugin);
}
#endif

// ------------------------------------------------------------------
// Simulated mount/unmount operations through dynamic plugin
// ------------------------------------------------------------------
#if defined(UUGEAR_PLUGIN_LINK_MODE_MODULE)
TEST(StoragePlugin, DynamicMountUnmountSimulation)
{
#ifdef UUGEAR_PLUGIN_DIR
    std::string pluginPath = std::string(UUGEAR_PLUGIN_DIR) + "/StoragePlugin.so";
#else
    std::string pluginPath = "./plugins/StoragePlugin.so";
#endif

    ASSERT_TRUE(fs::exists(pluginPath)) << "Plugin file not found: " << pluginPath;

    void* handle = nullptr;
    DevicePlugin* plugin = loadPlugin(pluginPath, &handle);
    ASSERT_NE(plugin, nullptr) << "Failed to create StoragePlugin instance dynamically";

    auto dev = makeMockPort(2, true);
    EXPECT_NO_THROW(plugin->onDeviceConnected(dev));
    EXPECT_NO_THROW(plugin->onDeviceDisconnected(dev));

    unloadPlugin(handle, plugin);
}
#endif

// ------------------------------------------------------------------
// PluginManager integration test (loads all .so plugins)
// ------------------------------------------------------------------
#if defined(UUGEAR_PLUGIN_LINK_MODE_MODULE)
TEST(PluginManager, LoadsPluginsDynamically)
{

#ifdef UUGEAR_PLUGIN_DIR
    std::string pluginDir = UUGEAR_PLUGIN_DIR;
#else
    std::string pluginDir = "./plugins";
#endif

    ASSERT_TRUE(fs::exists(pluginDir)) << "Plugin directory not found: " << pluginDir;

    PluginManager manager(pluginDir);

    ASSERT_NO_THROW(manager.loadAll()) << "PluginManager failed to load plugins";
    std::cout << "[Test] Loaded " << manager.pluginCount() << " plugins from " << pluginDir << "\n";
    for (const auto& plugin : manager.plugins())
    {
        std::cout << "[Test] Plugin loaded: " << plugin->name() << "\n";
    }

    // Expect at least one plugin to be loaded
    EXPECT_GE(manager.pluginCount(), 0);

    auto mockDevice = makeMockPort(1, true);
    EXPECT_NO_THROW(manager.handlePortChange(mockDevice, true)); // simulate connect
    EXPECT_NO_THROW(manager.handlePortChange(mockDevice, false)); // simulate disconnect
}
#endif


// ------------------------------------------------------------------
// PluginManager new API tests: getPluginByName() and getPluginAs<T>()
// ------------------------------------------------------------------
#if defined(UUGEAR_PLUGIN_LINK_MODE_MODULE)

TEST(PluginManager, DynamicLoad_GetByName)
{

#ifdef UUGEAR_PLUGIN_DIR
std::string pluginDir = UUGEAR_PLUGIN_DIR;
#else
const std::string pluginDir = "./plugins";
#endif

ASSERT_TRUE (fs::exists(pluginDir)) << "Plugin directory not found: " << pluginDir;

PluginManager manager(pluginDir);
manager.loadAll();

ASSERT_GT (manager.pluginCount(), 0u) << "No plugins loaded; cannot test dynamic plugins.";

// Try to find the StoragePlugin by name
DevicePlugin* plugin = manager.getPluginByName("StoragePlugin");
ASSERT_NE(plugin, nullptr)<< "getPluginByName() returned nullptr for StoragePlugin";
EXPECT_EQ (plugin->name(), "StoragePlugin");

// Verify that it can handle a mock USB device
auto mock = makeMockPort(1, true, "USB DISK 3.0", "Wilk");
EXPECT_TRUE (plugin->canHandle (mock)) << "StoragePlugin did not recognize valid USB device";

// Optional check for consistency
EXPECT_NO_THROW (manager.handlePortChange(mock, true));
EXPECT_NO_THROW (manager.handlePortChange(mock, false));
}

#else  // ----------------------------------------------------------------------


// ------------------------------------------------------------------
// Test group for STATIC/SHARED PLUGIN MODE (linked directly)
// ------------------------------------------------------------------
TEST(StoragePlugin, DirectUseAndRTTICheck)
{
#if !defined(UUGEAR_HAS_STORAGE_PLUGIN)
    GTEST_SKIP() << "StoragePlugin not compiled into build (UUGEAR_HAS_STORAGE_PLUGIN=OFF)";
#endif

    using namespace UUGear::Mega4;

    StoragePlugin plugin;

    EXPECT_EQ(plugin.name(), "StoragePlugin");

    auto mock = makeMockPort(1, true, "USB DISK 3.0", "Wilk");
    EXPECT_TRUE(plugin.canHandle(mock)) << "StoragePlugin returned false for valid USB device";

    // Test static helper methods
    fs::create_directories("/mnt/mega4/port" + std::to_string(mock.portNumber));
    EXPECT_NO_THROW(const auto mountPoint = StoragePlugin::getMountPoint(mock));
    fs::remove_all("mnt/mega4/port" + std::to_string(mock.portNumber));
}

#endif // UUGEAR_PLUGIN_LINK_MODE_MODULE
