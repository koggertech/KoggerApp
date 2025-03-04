# Universal AppImage build script
# Script downloads tools in out_x64, generates AppDir, builds .AppImage


set -e  # exit on any error

############################################
# 1. Project parameters
# 1.1. Source files
USER_NAME="username"                                            # !!! set your user name
BIN_PATH="build/Desktop_Qt_5_15_2_GCC_64bit-Release/KoggerApp"  # binary file
QML_DIR="QML"                                                   # QML path
ICON_SOURCE="assets/icons/kogger.png"                           # icon (256x256)
# 1.2. Result folders
OUT_DIR="out_x64"                                               # shared output folder
APPDIR="$OUT_DIR/AppDir"                                        # AppDir
# 1.3. Necessary Files for out_x64
DESKTOP_FILE="$OUT_DIR/KoggerApp.desktop"                       # automatically generated if not
ICON_DEST="$APPDIR/usr/share/icons/hicolor/256x256/apps/kogger.png"
# 1.4. Qt system path
export QTDIR="/home/$USER_NAME/Qt/5.15.2/gcc_64"
export PATH="$QTDIR/bin:$PATH"
export QML2_IMPORT_PATH="$QTDIR/qml"
export QT_PLUGIN_PATH="$QTDIR/plugins"
# 1.5. Set linuxdeploy, linuxdeploy-plugin-qt
LINUXDEPLOY="$OUT_DIR/linuxdeploy-x86_64.AppImage"
LINUXDEPLOY_QT="$OUT_DIR/linuxdeploy-plugin-qt-x86_64.AppImage"


############################################
# 2. Prepairing out_x64
echo "==> Creating folder $OUT_DIR (if not)"
mkdir -p "$OUT_DIR"
echo "==> Deleting old AppDir, if exist"
rm -rf "$APPDIR"
mkdir -p "$APPDIR"


############################################
# 3. Download linuxdeploy, linuxdeploy-plugin-qt if not exist
if [ ! -f "$LINUXDEPLOY" ]; then
  echo "==> Downloading linuxdeploy in $LINUXDEPLOY"
  wget -O "$LINUXDEPLOY" "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage"
  chmod +x "$LINUXDEPLOY"
else
  echo "==> linuxdeploy already exist: $LINUXDEPLOY"
fi

if [ ! -f "$LINUXDEPLOY_QT" ]; then
  echo "==> Downloading linuxdeploy-plugin-qt in $LINUXDEPLOY_QT"
  wget -O "$LINUXDEPLOY_QT" "https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage"
  chmod +x "$LINUXDEPLOY_QT"
else
  echo "==> linuxdeploy-plugin-qt already exist: $LINUXDEPLOY_QT"
fi


############################################
# 4. Desktop-file, icon
# 4.1. Creating .desktop-file, if not exist
if [ ! -f "$DESKTOP_FILE" ]; then
  echo "==> Creating KoggerApp.desktop in $DESKTOP_FILE"
  cat << EOF > "$DESKTOP_FILE"
[Desktop Entry]
Type=Application
Name=KoggerApp
Exec=KoggerApp
Icon=kogger
Categories=Utility;
EOF
else
  echo "==> Use existing .desktop: $DESKTOP_FILE"
fi
# 4.2. Copy .desktop -> AppDir
mkdir -p "$APPDIR/usr/share/applications"
cp "$DESKTOP_FILE" "$APPDIR/usr/share/applications/"
# 4.3. Icon: 
mkdir -p "$(dirname "$ICON_DEST")"  # usr/share/icons/hicolor/256x256/apps
echo "==> Copy icon $ICON_SOURCE -> $ICON_DEST"
cp "$ICON_SOURCE" "$ICON_DEST"


############################################
# 5. Copy binary KoggerApp file in AppDir
echo "==> Copy binary file $BIN_PATH -> $APPDIR/usr/bin"
mkdir -p "$APPDIR/usr/bin"
cp "$BIN_PATH" "$APPDIR/usr/bin/"
chmod +x "$APPDIR/usr/bin/KoggerApp"
# 5.1 Copy QML modules manually # !!! check QML paths
echo "==> Copy QML modules manually"
mkdir -p $OUT_DIR/AppDir/usr/qml/QtQuick
cp -r /home/$USER_NAME/Qt/5.15.2/gcc_64/qml/QtQuick/Controls.2     $OUT_DIR/AppDir/usr/qml/QtQuick/
cp -r /home/$USER_NAME/Qt/5.15.2/gcc_64/qml/QtQuick/Dialogs        $OUT_DIR/AppDir/usr/qml/QtQuick/
cp -r /home/$USER_NAME/Qt/5.15.2/gcc_64/qml/QtQuick/Layouts        $OUT_DIR/AppDir/usr/qml/QtQuick/
cp -r /home/$USER_NAME/Qt/5.15.2/gcc_64/qml/QtQuick/Window.2       $OUT_DIR/AppDir/usr/qml/QtQuick/
mkdir -p $OUT_DIR/AppDir/usr/qml/Qt/labs
cp -r /home/$USER_NAME/Qt/5.15.2/gcc_64/qml/Qt/labs/settings       $OUT_DIR/AppDir/usr/qml/Qt/labs/


##########################################
# 6. Building AppImage with linuxdeploy
echo "==> Run linuxdeploy with --qmldir $QML_DIR"
# Try set QML path through path for Qt plugin
LINUXDEPLOY_PLUGIN_QT_QMLDIR="$QML_DIR" \
"$LINUXDEPLOY" \
  --appdir "$APPDIR" \
  --desktop-file "$APPDIR/usr/share/applications/$(basename "$DESKTOP_FILE")" \
  --icon-file "$ICON_DEST" \
  -p qt \
  -o appimage
# 6.1 Copy AppImage to $OUT_DIR
mv KoggerApp-x86_64.AppImage "$OUT_DIR"/


##########################################
# 7. Check build results
echo "==> Complete! Checking results in $OUT_DIR"
ls -lh "$OUT_DIR"/*.AppImage || echo "AppImage not finded!"
echo
echo "For run KoggerApp AppImage try:"
echo "  chmod +x $OUT_DIR/KoggerApp*.AppImage"
echo "  ./$OUT_DIR/KoggerApp*.AppImage"
echo
echo "Done."
