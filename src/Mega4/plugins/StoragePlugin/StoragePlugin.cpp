#include "UUGear/Mega4/plugins/StoragePlugin.hpp"

#include "UUGear/Mega4/Mega4Types.hpp"

#include <iostream>
#include <fstream>
#include <cstdlib>
#include <regex>
#include <sstream>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <libmount/libmount.h>


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


    // ================== NUEVAS IMPLEMENTACIONES USANDO LIBMOUNT ==================
    bool StoragePlugin::mountDevice(const std::string& device, const std::string& mountPoint)
    {
    #if defined(__linux__)
        struct libmnt_context* cxt = mnt_new_context();
        if (!cxt)
        {
            std::cerr << "[StoragePlugin] Failed to create libmount context\n";
            return false;
        }

        mnt_context_set_source(cxt, device.c_str());
        mnt_context_set_target(cxt, mountPoint.c_str());
        mnt_context_set_fstype(cxt, "auto"); // deja que el kernel detecte el tipo

        int status = mnt_context_mount(cxt);
        if (status != 0)
        {
            char buf[256];
            mnt_context_get_excode(cxt, status, buf, sizeof(buf));
            std::cerr << "[StoragePlugin] Mount error (" << status << "): " << buf << "\n";
            mnt_free_context(cxt);
            return false;
        }

        mnt_free_context(cxt);
        return true;
    #else
        std::cerr << "[StoragePlugin] Mount not supported on this platform\n";
        return false;
    #endif
    }

    bool StoragePlugin::unmountDevice(const std::string& mountPoint)
    {
    #if defined(__linux__)
        struct libmnt_context* cxt = mnt_new_context();
        if (!cxt)
        {
            std::cerr << "[StoragePlugin] Failed to create libmount context\n";
            return false;
        }

        mnt_context_set_target(cxt, mountPoint.c_str());
        int status = mnt_context_umount(cxt);
        if (status != 0)
        {
            char buf[256];
            mnt_context_get_excode(cxt, status, buf, sizeof(buf));
            std::cerr << "[StoragePlugin] Unmount error (" << status << "): " << buf << "\n";
            mnt_free_context(cxt);
            return false;
        }

        mnt_free_context(cxt);
        return true;
    #else
        std::cerr << "[StoragePlugin] Unmount not supported on this platform\n";
        return false;
    #endif
    }

    // ==============================================================================

    std::string StoragePlugin::getMountPoint(const PortConnectionInfo& info)
    {
        if (info.portNumber < 1 || info.portNumber > 4)
        {
            throw std::invalid_argument("Invalid port number: " + std::to_string(info.portNumber));
        }

        const std::string mountPoint = "/mnt/mega4/port" + std::to_string(info.portNumber);

        if (fs::exists(mountPoint) && fs::is_directory(mountPoint))
        {
            return mountPoint;
        }
        else
        {
            throw std::runtime_error("Mount point not found or is not a directory for port " + std::to_string(info.portNumber));
        }
    }


    bool StoragePlugin::writeToFile(const PortConnectionInfo& info, const std::string& filename, const std::string& data)
    {
        std::string mountPoint = getMountPoint(info);
        if (mountPoint.empty()) {
            std::cerr << "[StoragePlugin] Device is not mounted, cannot write to file." << std::endl;
            return false;
        }

        std::string filePath = mountPoint + "/" + filename;

        std::ofstream outFile(filePath);
        if (!outFile) {
            std::cerr << "[StoragePlugin] Failed to open file for writing: " << filePath << std::endl;
            return false;
        }

        outFile << data;
        outFile.close();
        return true;
    }

    std::string StoragePlugin::readFromFile(const PortConnectionInfo& info, const std::string& filename)
    {
        std::string mountPoint = getMountPoint(info);
        if (mountPoint.empty()) {
            std::cerr << "[StoragePlugin] Device is not mounted, cannot read from file." << std::endl;
            return "";
        }

        std::string filePath = mountPoint + "/" + filename;
        std::ifstream inFile(filePath);
        if (!inFile) {
            std::cerr << "[StoragePlugin] Failed to open file for reading: " << filePath << std::endl;
            return "";
        }

        std::stringstream buffer;
        buffer << inFile.rdbuf();
        return buffer.str();
    }


} // namespace UUGear::Mega4

// Dynamic plugin factory entry points
extern "C" UUGear::Mega4::DevicePlugin* createPlugin() { return new UUGear::Mega4::StoragePlugin(); }
extern "C" void destroyPlugin(UUGear::Mega4::DevicePlugin* plugin) { delete plugin; }
