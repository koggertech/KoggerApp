<img src="resources/images/readme_md/main_logo.png" height="480" alt="KoggerApp Logo" style="pointer-events:none;">

[![GPLv3 License](https://img.shields.io/badge/License-GPLv3-green.svg)](https://choosealicense.com/licenses/gpl-3.0/)
[![Latest Release](https://img.shields.io/github/v/release/koggertech/KoggerApp?color=blue)](https://github.com/koggertech/KoggerApp/releases)

*KoggerApp* is a full-featured, easy-to-use, and free open-source software (FOSS) solution designed for hydrographic and survey applications. Whether you're a beginner or an experienced professional, KoggerApp delivers highly detailed visualization of sonar data collected from Kogger devices. The application is cross-platform and available for Windows, Android, and Linux.

---

### 🚁 *Key Features*
<!-- Add bullet points or descriptions of features here -->

---

### 🛠️ *Installation*
You can download the latest release from the [Releases page](https://github.com/koggertech/KoggerApp/releases).

🪟 Windows (x86_64):  
 — Portable: Download .zip. Extract and run "KoggerApp.exe".  
 — Installer: Download and launch the installer (Optionally associates .klf files with the app during setup).

🤖 Android (ARMv7, ARMv8):  
Download and install the .apk file. You may need to enable "Install from unknown sources" in your device settings.

🐧 Linux (Ubuntu x86_64):  
Download the .AppImage file and make it executable:
```bash
chmod +x KoggerApp_version_linux_x86_64.AppImage
./KoggerApp_version_linux_x86_64.AppImage
```

---

### 🧱 *Build Instructions*
*KoggerApp* is a cross-platform C++ Qt QML project, built using a .pro file. You can build it on Windows, Linux, and Android using the appropriate Qt kits and compilers.

🪟 Windows (x86_64):  
Compiler: MinGW 8.1.0  
Qt version: Qt 5.15.2 (MinGW)  
Steps:
- Open the .pro file in Qt Creator
- Select the MinGW 64-bit kit
- Click Build > Run qmake, then Build > Build Project
- Run the application from Qt Creator or find the built binary in build/

🤖 Android (ARMv7, ARMv8)  
Compiler: Clang from NDK 21.3.6528147  
Qt version: Qt 5.15.2 for Android  
Set up Android SDK/NDK in Qt Creator (via Tools > Options > Devices > Android)  
Steps:
- Open the .pro file in Qt Creator
- Select Android kit
- Click Build > Run qmake, then Build > Build Project
- Use the .apk file generated in android-build/ to install on a device

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

### 💻 *Get Involved!*
KoggerApp is open-source, meaning you have the power to shape it! Whether you're fixing bugs, adding features, or customizing for your specific needs, KoggerApp welcomes contributions from the community.

---

- 🌐 [Official Website](https://kogger.tech/)

---

Unlock next-level insights in hydrography and survey with KoggerApp
