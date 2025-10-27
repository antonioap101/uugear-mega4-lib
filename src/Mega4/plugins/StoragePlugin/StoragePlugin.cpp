#include "StoragePlugin.hpp"

#include "UUGear/Mega4/Mega4Types.hpp"

#include <iostream>
#include <fstream>
#include <cstdlib>
#include <regex>
#include <sstream>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>


namespace fs = std::filesystem;

namespace UUGear::Mega4
{
    bool StoragePlugin::canHandle(const PortConnectionInfo& info) const
    {
        // USB mass storage devices typically have recognizable product names
        return info.hasDevice && (
            info.product.find("USB") != std::string::npos ||
            info.product.find("Mass Storage") != std::string::npos ||
            info.product.find("DISK") != std::string::npos
        );
    }

    void StoragePlugin::onDeviceConnected(const PortConnectionInfo& info)
    {
        std::cout << "[StoragePlugin] Detected storage device on port " << info.portNumber
            << " (" << info.manufacturer << " " << info.product << ")\n";

        const std::string devPath = findBlockDevice(info);
        if (devPath.empty())
        {
            std::cerr << "[StoragePlugin] Could not resolve block device for port "
                << info.portNumber << "\n";
            return;
        }

        std::string mountPoint = "/mnt/mega4/port" + std::to_string(info.portNumber);
        fs::create_directories(mountPoint);

        if (mountDevice(devPath, mountPoint))
            std::cout << "[StoragePlugin] Mounted " << devPath << " → " << mountPoint << "\n";
        else
            std::cerr << "[StoragePlugin] Failed to mount " << devPath << "\n";
    }

    void StoragePlugin::onDeviceDisconnected(const PortConnectionInfo& info)
    {
        std::string mountPoint = "/mnt/mega4/port" + std::to_string(info.portNumber);
        std::cout << "[StoragePlugin] Device removed from port " << info.portNumber << ", unmounting " << mountPoint <<
            "\n";

        if (unmountDevice(mountPoint))
            std::cout << "[StoragePlugin] Unmounted successfully.\n";
        else
            std::cerr << "[StoragePlugin] Failed to unmount.\n";
    }

    /**
     * Attempts to find the corresponding block device (e.g., /dev/sda1)
     * matching the given USB device using /sys and lsblk parsing.
     */
    std::string StoragePlugin::findBlockDevice(const PortConnectionInfo& info)
    {
        // Strategy 1: use lsblk output and filter by vendor/product string
        FILE* pipe = popen("lsblk -o NAME,MODEL,TRAN | grep usb", "r");
        if (!pipe) return "";

        char buffer[256];
        std::string output;
        while (fgets(buffer, sizeof(buffer), pipe))
            output += buffer;
        pclose(pipe);

        std::istringstream stream(output);
        std::string line;
        std::regex devRegex(R"(^(\S+)\s+(.+))");
        std::smatch match;

        while (std::getline(stream, line))
        {
            if (std::regex_search(line, match, devRegex))
            {
                const std::string name = match[1];
                const std::string model = match[2];

                if (model.find(info.product) != std::string::npos ||
                    model.find(info.manufacturer) != std::string::npos)
                {
                    return "/dev/" + name;
                }
            }
        }

        return "";
    }

    /**
     * Mounts the device at the specified mount point using a system call.
     */
    bool StoragePlugin::mountDevice(const std::string& device, const std::string& mountPoint)
    {
        std::string cmd = "mount " + device + " " + mountPoint + " 2>/dev/null";
        return system(cmd.c_str()) == 0;
    }

    /**
     * Unmounts the device using umount.
     */
    bool StoragePlugin::unmountDevice(const std::string& mountPoint)
    {
        std::string cmd = "umount " + mountPoint + " 2>/dev/null";
        return system(cmd.c_str()) == 0;
    }
} // namespace UUGear::Mega4

// Dynamic plugin factory entry points
extern "C" UUGear::Mega4::DevicePlugin* createPlugin() { return new UUGear::Mega4::StoragePlugin(); }
extern "C" void destroyPlugin(UUGear::Mega4::DevicePlugin* plugin) { delete plugin; }
