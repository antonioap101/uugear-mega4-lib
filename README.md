# 🧩 UUGear MEGA4 C++ Library

<img src="https://www.uugear.com/wordpress/wp-content/uploads/2021/08/01.jpg" width="500" alt="UUGear MEGA4 USB Hub">

> “MEGA4 is a smart 4-port USB 3.1 PPPS hub for Raspberry Pi 4B that supports individual port power control, USB-C input, and high-current output.”
> — [UUGear Official Website](https://www.uugear.com/product/mega4-4-port-usb-3-1-ppps-hub-for-raspberry-pi-4b/)

A C++17 library for controlling the [**UUGear MEGA4**](https://www.uugear.com/product/mega4-4-port-usb-3-1-ppps-hub-for-raspberry-pi-4b/) —  
a 4-port USB 3.1 PPPS hub with per-port power switching, designed for **Raspberry Pi 4B** and compatible embedded systems.

This library provides a minimal interface on top of [**libusb**](https://libusb.info)  
to query, power-toggle and inspect devices connected to MEGA4 hubs programmatically.

---

## 🚀 Features

- Detect connected **MEGA4 hubs** automatically  
- **Turn ON/OFF power** on each individual USB port  
- Query **real-time port power states**  
- Enumerate and identify **devices connected to each port**  
- Cross-platform (Linux, ARM boards such as Rock Pi S / Raspberry Pi)

---

## 🧠 Examples

Ready-to-run examples are available in the [`examples/`](examples/) directory. They demonstrate how to:

- Detect connected **MEGA4 hubs**
- Enumerate devices connected to each port
- Query **port power states**
- Toggle power **ON/OFF** on individual ports

You can build and run the example with:

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -t toggle_port
./build/toggle_port
```

---

## 🛠️ Installation

### Using **FetchContent** (recommended)

Add this to your project’s `CMakeLists.txt`:

```cmake
include(FetchContent)

FetchContent_Declare(
    uugear_mega4_lib
    GIT_REPOSITORY https://github.com/antonioap101/uugear-mega4-lib.git
    GIT_TAG master
)
FetchContent_MakeAvailable(uugear_mega4_lib)

target_link_libraries(your_project PRIVATE uugear_mega4_lib)
```

The library will be automatically downloaded and built with your project.

### Building standalone

```bash
git clone https://github.com/antonioaparicio/uugear-mega4-lib.git
cd uugear-mega4-lib
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build
```

### Dependencies

* [libusb 1.0.26+](https://libusb.info)
* CMake ≥ 3.16
* A C++17 compiler (GCC 9+, Clang 10+, MSVC 2019+)

---

## 🧩 API Overview

| Function                           | Description                                                            |
| ---------------------------------- | ---------------------------------------------------------------------- |
| `listDevices()`                    | Detects all MEGA4 hubs connected to the system                         |
| `powerOn(port)` / `powerOff(port)` | Turns a port ON or OFF                                                 |
| `getPortStates()`                  | Returns current ON/OFF states of all 4 ports                           |
| `getPortConnections()`             | Lists devices connected to each port (VID, PID, manufacturer, product) |

---


## 🧑‍💻 Author & License

Created by **Antonio Aparicio**
MIT License — see [`LICENSE`](./LICENSE) for details.

> This project is an independent, community-developed library and is **not affiliated with or endorsed by UUGear**. If you use this project, please consider visiting [**UUGear**](https://www.uugear.com) and supporting the creators of the MEGA4 hardware.

---

### ⭐ Star this repo

If this library helps your project, please consider giving it a ⭐!

