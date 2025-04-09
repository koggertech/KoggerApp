<p align="center">
  <img src="resources/images/readme_md/kogger_app_logo.png" alt="KoggerApp Logo" width="670">
</p>

[![Latest Release](https://img.shields.io/github/v/release/koggertech/KoggerApp?color=blue)](https://github.com/koggertech/KoggerApp/releases)
[![GPLv3 License](https://img.shields.io/badge/License-GPLv3-green.svg)](https://choosealicense.com/licenses/gpl-3.0/)

*KoggerApp* is a full-featured, easy-to-use, and free open-source software (FOSS) solution designed for hydrographic and survey applications. Whether you're a beginner or an experienced professional, KoggerApp delivers highly detailed visualization of sonar data collected from Kogger devices. The application is cross-platform and available for Windows, Android, and Linux.

---

### 🚁 *Key Features*
- Multiple connections of Kogger sonars via serial port or UDP network.

- Autopilot data display (charge level, speed, communication quality)

- Real-time display of highly detailed echograms
<p align="center">
<img src="resources/images/readme_md/echogram.jpg" alt="Echogram" width="768">
</p>

- Depth calculation from received sonar data

- Manual editing of measurement data (depth, bottom surface, etc.)

- Calculation of bottom surface by bottom track
<p align="center">
<img src="resources/images/readme_md/surface.jpg" alt="3d Surface" width="768">
</p>

- Real-time calculation of side scan mosaics
<p align="center">
<img src="resources/webp/readme_md/echogram_and_mosaic.webp" alt="Echogram and Mosaic Preview" width="768">
</p>

- Loading and displaying a globe map for georeferencing measurements to the survey location

- Convert bottom track, bottom surface to .csv file for importing measurements into other applications

- Customization of sonar acquisition parameters (frequency, sound speed, resolution, etc.)
<p align="center">
<img src="resources/images/readme_md/sonar_settings.jpg" alt="Sonar settings" width="768">
</p>

- Switching modes of operation for new users and professional researchers.

---

### 🛠️ *Installation*
You can download the latest release from the [Releases page](https://github.com/koggertech/KoggerApp/releases).

🪟 Windows (x86_64):  
- Portable: Download .zip. Extract and run "KoggerApp.exe".  
- Installer: Download and launch the installer (Optionally associates .klf files with the app during setup).

🤖 Android (ARMv7, ARMv8):  
Download and install the .apk file. You may need to enable "Install from unknown sources" in your device settings.

🐧 Linux (Ubuntu x86_64):  
Download the .AppImage file and make it executable:
```bash
chmod +x KoggerApp_version_linux_x86_64.AppImage
```
and run:
```bash
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

### 🔗 *Useful links*
- 🌐 [Official Website](https://kogger.tech/)

---

Unlock next-level insights in hydrography and survey with KoggerApp.

<p align="center">
<img src="resources/images/readme_md/kogger_logo.png" alt="kogger logo" width="670">
</p>
