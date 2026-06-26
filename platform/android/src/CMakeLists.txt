target_sources(KoggerApp PRIVATE
    android_init.cpp
    android_interface.h        android_interface.cpp
    android_serial.h           android_serial.cpp
    kogger_logging_category.h  kogger_logging_category.cpp
    qml_object_list_model.h    qml_object_list_model.cpp

    qtandroidserialport/qserialport.h
    qtandroidserialport/qserialport_p.h
    qtandroidserialport/qserialport.cpp
    qtandroidserialport/qserialport_android.cpp
    qtandroidserialport/qserialportglobal.h
    qtandroidserialport/qserialportinfo.h
    qtandroidserialport/qserialportinfo_p.h
    qtandroidserialport/qserialportinfo.cpp
    qtandroidserialport/qserialportinfo_android.cpp
    qtandroidserialport/qtserialportexports.h
    qtandroidserialport/qtserialportversion.h)

target_include_directories(KoggerApp PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/qtandroidserialport)

# QtCore private headers (serialport shim)
target_include_directories(KoggerApp PRIVATE
    "${QT6_INSTALL_PREFIX}/include/QtCore/${Qt6Core_VERSION}"
    "${QT6_INSTALL_PREFIX}/include/QtCore/${Qt6Core_VERSION}/QtCore")
