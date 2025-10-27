#ifndef UUGEAR_MEGA4_LIB_DEVICEPLUGIN_HPP
#define UUGEAR_MEGA4_LIB_DEVICEPLUGIN_HPP

#include <string>


namespace UUGear::Mega4
{
    struct PortConnectionInfo;

    class DevicePlugin;
}


class UUGear::Mega4::DevicePlugin
{
public:
    virtual ~DevicePlugin() = default;

    [[nodiscard]] virtual std::string name() const = 0;
    [[nodiscard]] virtual bool canHandle(const PortConnectionInfo& info) const = 0;
    virtual void onDeviceConnected(const PortConnectionInfo& info) = 0;
    virtual void onDeviceDisconnected(const PortConnectionInfo& info) = 0;
};


// Plugin factory entry points (C ABI)
extern "C" UUGear::Mega4::DevicePlugin* createPlugin();
extern "C" void destroyPlugin(UUGear::Mega4::DevicePlugin* plugin);


#endif //UUGEAR_MEGA4_LIB_DEVICEPLUGIN_HPP
