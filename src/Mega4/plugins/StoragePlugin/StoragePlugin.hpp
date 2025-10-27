#ifndef UUGEAR_MEGA4_LIB_STORAGEPLUGIN_HPP
#define UUGEAR_MEGA4_LIB_STORAGEPLUGIN_HPP


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
class UUGear::Mega4::StoragePlugin final : public DevicePlugin
{
public:
    StoragePlugin() = default;
    ~StoragePlugin() override = default;

    [[nodiscard]] std::string name() const override { return "StoragePlugin"; }

    [[nodiscard]] bool canHandle(const PortConnectionInfo& info) const override;

    void onDeviceConnected(const PortConnectionInfo& info) override;

    void onDeviceDisconnected(const PortConnectionInfo& info) override;

private:
    [[nodiscard]] static std::string findBlockDevice(const PortConnectionInfo& info);
    static bool mountDevice(const std::string& device, const std::string& mountPoint);
    static bool unmountDevice(const std::string& mountPoint);
};


// Factory entry points for dynamic loading
extern "C" UUGear::Mega4::DevicePlugin* createPlugin();
extern "C" void destroyPlugin(UUGear::Mega4::DevicePlugin* plugin);

#endif //UUGEAR_MEGA4_LIB_STORAGEPLUGIN_HPP
