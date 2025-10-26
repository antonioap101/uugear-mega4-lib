#include "Mega4Hub.hpp"
#include <iostream>

int main() {
    try {
        Mega4Hub hub;
        auto devices = hub.listDevices();

        if (devices.empty()) {
            std::cout << "No MEGA4 hubs detected. Enabling simulation mode.\n";
            hub.setSimulationMode(true);
        } else {
            std::cout << "Detected " << devices.size() << " hub(s):\n";
            for (auto& d : devices)
                std::cout << " - " << d.description << " @ " << d.busPortPath << "\n";
        }

        std::cout << "Turning port 1 ON...\n";
        hub.powerOn(1);

        std::cout << "Turning port 1 OFF...\n";
        hub.powerOff(1);

    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << "\n";
        return 1;
    }
    return 0;
}
