<img src="resources/images/readme_md/main_logo.png" width="1024" alt="KoggerApp Logo">

<p align="center">
  <a href="https://github.com/koggertech/KoggerApp/releases">
    <img src="https://img.shields.io/github/v/release/koggertech/KoggerApp.svg" alt="Latest Release">
  </a>
    <a href="https://choosealicense.com/licenses/gpl-3.0/">
    <img src="https://img.shields.io/badge/License-GPLv3-green.svg" alt="License: GPLv3">
  </a>  
</p>

*KoggerApp* is a full-featured, easy-to-use, and free open-source software (FOSS) solution designed for hydrographic and geodetic applications. Whether you're a beginner or an experienced professional, KoggerApp delivers highly detailed visualization of sonar data collected from Kogger devices. The application is cross-platform and available for Windows, Android, and Linux.

---

### 🚁 *Key Features*
<!-- Add bullet points or descriptions of features here -->

---

### 🛠️ Installation
You can download the latest release from the [Releases page](https://github.com/koggertech/KoggerApp/releases).

🪟 Windows (x86_64)
Option 1 — Portable:
Download .zip. Extract and run KoggerApp.exe (No installation required).
Option 2 — Installer:
Download and launch the installer (Optionally associates .klf files with the app during setup).

🤖 Android (ARMv7 and ARMv8)
Download and install the .apk file.
You may need to enable "Install from unknown sources" in your device settings.

🐧 Linux (x86_64)
Download the .AppImage file and make it executable:
```bash
chmod +x KoggerApp_version_linux_x86_64.AppImage
./KoggerApp_version_linux_x86_64.AppImage
```

---

### 🧱 Build Instructions
KoggerApp is a cross-platform C++ Qt QML project, built using a .pro file. You can build it on Windows, Linux, and Android using the appropriate Qt kits and compilers.

🪟 Windows (x86_64)
Compiler: MinGW 8.1.0
Qt version: Qt 5.15.2 (MinGW)
Qt Modules: core, gui, qml, quick, widgets, sql, network, etc.
Steps:
Open the .pro file in Qt Creator
Select the MinGW 64-bit kit
Click Build > Run qmake, then Build > Build Project
Run the application from Qt Creator or find the built binary in build/

🤖 Android
Compiler: Clang from NDK 21.3.6528147
Qt version: Qt 5.15.2 for Android
Set up Android SDK/NDK in Qt Creator (via Tools > Options > Devices > Android)
Steps:
Open the .pro file in Qt Creator
Select Android kit (e.g., armeabi-v7a, arm64-v8a, or universal)
Click Build > Run qmake, then Build > Build Project
Use the .apk file generated in android-build/ to install on a device

🐧 Linux (Ubuntu x86_64)
Compiler: GCC 13
Qt version: Qt 5.15.2
Required packages:
```bash
sudo apt install qtbase5-dev qtdeclarative5-dev qtquickcontrols2-5-dev qt5-qmake build-essential
```
Clone and build:
```bash
git clone https://github.com/koggertech/KoggerApp.git
cd KoggerApp
qmake
make -j$(nproc)
./KoggerApp
```

---

### 💻 Get Involved!
KoggerApp is open-source, meaning you have the power to shape it! Whether you're fixing bugs, adding features, or customizing for your specific needs, KoggerApp welcomes contributions from the community.

---

- 🌐 [Official Website](https://kogger.tech/)

---

Unlock next-level insights in hydrography and geodesy with KoggerApp.
