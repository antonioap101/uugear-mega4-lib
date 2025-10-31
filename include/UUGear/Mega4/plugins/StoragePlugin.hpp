#ifndef UUGEAR_MEGA4_LIB_STORAGEPLUGIN_HPP
#define UUGEAR_MEGA4_LIB_STORAGEPLUGIN_HPP

#if !defined(UUGEAR_HAS_STORAGE_PLUGIN) || defined(UUGEAR_PLUGIN_LINK_MODE_MODULE)
    // Definición completa de la clase StoragePlugin (como tienes ahora)
    #warning "StoragePlugin is not available (either UUGEAR_HAS_STORAGE_PLUGIN=OFF or link mode = MODULE)"
#else


#include "UUGear/Mega4/DevicePlugin.hpp"
#include <string>
#include <filesystem>

namespace UUGear::Mega4
{
    struct PortConnectionInfo;
    class StoragePlugin;
}

/**
 * @brief Plugin that automatically mounts and unmounts USB storage devices
 *        connected to the UUGear MEGA4 hub.
 */
class UUGear::Mega4::StoragePlugin : public DevicePlugin
{
public:
    StoragePlugin() = default;

    ~StoragePlugin() override = default;

    [[nodiscard]] std::string name() const override { return "StoragePlugin"; }

    [[nodiscard]] virtual bool canHandle(const PortConnectionInfo& info) const override;

    virtual void onDeviceConnected(const PortConnectionInfo& info) override;

    virtual void onDeviceDisconnected(const PortConnectionInfo& info) override;

    [[nodiscard]] virtual std::string getMountPoint(const PortConnectionInfo& info);

    virtual  bool writeToFile(const PortConnectionInfo& info, const std::string& filename, const std::string& data);

    virtual std::string readFromFile(const PortConnectionInfo& info, const std::string& filename);

private:
    [[nodiscard]] static std::string findBlockDevice(const PortConnectionInfo& info);
    static bool mountDevice(const std::string& device, const std::string& mountPoint);
    static bool unmountDevice(const std::string& mountPoint);
};


// Factory entry points for dynamic loading
extern "C" UUGear::Mega4::DevicePlugin* createPlugin();
extern "C" void destroyPlugin(UUGear::Mega4::DevicePlugin* plugin);

#endif // UUGEAR_HAS_STORAGE_PLUGIN


#endif //UUGEAR_MEGA4_LIB_STORAGEPLUGIN_HPP
