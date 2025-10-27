#ifndef UUGEAR_MEGA4_LIB_PLUGINMANAGER_HPP
#define UUGEAR_MEGA4_LIB_PLUGINMANAGER_HPP

#include "UUGear/Mega4/DevicePlugin.hpp"
#include <string>
#include <vector>
#include <memory>

namespace UUGear::Mega4
{
    class PluginManager;
    struct PortConnectionInfo;
}

/**
 * @brief Loads and manages dynamically linked plugins (.so/.dll) at runtime.
 */
class UUGear::Mega4::PluginManager
{
public:
    explicit PluginManager(std::string directory);
    ~PluginManager();

    /**
    * @brief Loads all plugins from the specified directory.
    */
    void loadAll();

    /**
     * @brief Notifies all loaded plugins about a port connection change.
     * @param info Information about the port connection change.
     * @param connected True if a device was connected, false if disconnected.
    */
    void handlePortChange(const PortConnectionInfo& info, bool connected);

    /**
    * @brief Returns all currently loaded plugin instances.
    *        The returned pointers are owned by PluginManager — do not delete them.
    */
    [[nodiscard]] std::vector<DevicePlugin*> plugins() const;

    /**
     * @brief Returns the number of loaded plugins.
     */
    [[nodiscard]] size_t pluginCount() const noexcept;

private:
    struct LoadedPlugin
    {
        void* handle;
        DevicePlugin* instance;
        void (*destroy)(DevicePlugin*);
    };

    std::vector<LoadedPlugin> plugins_;
    std::string directory_;
};

#endif //UUGEAR_MEGA4_LIB_PLUGINMANAGER_HPP
