#include "UUGear/Mega4/PluginManager.hpp"
#include <filesystem>
#include <dlfcn.h>
#include <iostream>

namespace fs = std::filesystem;

namespace UUGear::Mega4
{
    PluginManager::PluginManager(std::string directory)
        : directory_(std::move(directory))
    {
    }

    PluginManager::~PluginManager()
    {
        for (auto& plugin : plugins_)
        {
            if (plugin.instance && plugin.destroy)
                plugin.destroy(plugin.instance);
            if (plugin.handle)
                dlclose(plugin.handle);
        }
    }

    void PluginManager::loadAll()
    {
        if (!fs::exists(directory_))
        {
            std::cerr << "Plugin directory not found: " << directory_ << "\n";
            return;
        }

        for (const auto& entry : fs::directory_iterator(directory_))
        {
            if (entry.path().extension() != ".so")
                continue;

            const std::string path = entry.path().string();
            void* handle = dlopen(path.c_str(), RTLD_NOW);
            if (!handle)
            {
                std::cerr << "Failed to load plugin: " << path << " — " << dlerror() << "\n";
                continue;
            }

            const auto create = reinterpret_cast<DevicePlugin* (*)()>(dlsym(handle, "createPlugin"));
            const auto destroy = reinterpret_cast<void (*)(DevicePlugin*)>(dlsym(handle, "destroyPlugin"));

            if (!create || !destroy)
            {
                std::cerr << "Invalid plugin: " << path << "\n";
                dlclose(handle);
                continue;
            }

            DevicePlugin* instance = create();
            std::cout << "[PluginManager] Loaded plugin: " << instance->name() << "\n";

            plugins_.push_back({handle, instance, destroy});
        }
    }

    void PluginManager::handlePortChange(const PortConnectionInfo& info, bool connected)
    {
        for (auto& plugin : plugins_)
        {
            if (!plugin.instance->canHandle(info))
            {
                continue;
            }
            if (connected) plugin.instance->onDeviceConnected(info);
            else plugin.instance->onDeviceDisconnected(info);
        }
    }

    std::vector<DevicePlugin*> PluginManager::plugins() const
    {
        std::vector<DevicePlugin*> out;
        out.reserve(plugins_.size());
        for (const auto& p : plugins_)
            out.push_back(p.instance);
        return out;
    }

    size_t PluginManager::pluginCount() const noexcept
    {
        return plugins_.size();
    }
} // namespace UUGear::Mega4
