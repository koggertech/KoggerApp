target_sources(KoggerApp PRIVATE
    boat_track_control_menu_controller.h      boat_track_control_menu_controller.cpp
    bottom_track_control_menu_controller.h    bottom_track_control_menu_controller.cpp
    geojson_controller.h                      geojson_controller.cpp
    geojson_tree_model.h                      geojson_tree_model.cpp
    image_view_control_menu_controller.h      image_view_control_menu_controller.cpp
    map_view_control_menu_controller.h        map_view_control_menu_controller.cpp
    mpc_filter_control_menu_controller.h      mpc_filter_control_menu_controller.cpp
    navigation_arrow_control_menu_controller.h navigation_arrow_control_menu_controller.cpp
    npd_filter_control_menu_controller.h      npd_filter_control_menu_controller.cpp
    point_group_control_menu_controller.h     point_group_control_menu_controller.cpp
    polygon_group_control_menu_controller.h   polygon_group_control_menu_controller.cpp
    qml_component_controller.h                qml_component_controller.cpp
    scene3d_control_menu_controller.h         scene3d_control_menu_controller.cpp
    scene3d_toolbar_controller.h              scene3d_toolbar_controller.cpp
    mosaic_view_control_menu_controller.h     mosaic_view_control_menu_controller.cpp
    isobaths_view_control_menu_controller.h   isobaths_view_control_menu_controller.cpp
    usbl_view_control_menu_controller.h       usbl_view_control_menu_controller.cpp)

target_include_directories(KoggerApp PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
