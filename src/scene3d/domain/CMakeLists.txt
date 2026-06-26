target_sources(KoggerApp PRIVATE
    barycentric_interpolator.h
    boat_track.h            boat_track.cpp
    bottom_track.h          bottom_track.cpp
    boundary_detector.h
    circle.h
    contour.h               contour.cpp
    coordinate_axes.h       coordinate_axes.cpp
    cube.h                  cube.cpp
    delaunay_triangulation.h
    edge.h
    equals.h
    geojson_layer.h         geojson_layer.cpp
    grid_generator.h
    image_view.h            image_view.cpp
    map_view.h              map_view.cpp
    navigation_arrow.h      navigation_arrow.cpp
    plane.h                 plane.cpp
    plane_grid.h            plane_grid.cpp
    point_2d.h
    point_3d.h
    point_group.h           point_group.cpp
    point_object.h          point_object.cpp
    polygon_group.h         polygon_group.cpp
    polygon_object.h        polygon_object.cpp
    quad.h
    ruler_tool.h            ruler_tool.cpp
    surface_view.h          surface_view.cpp
    isobaths_view.h         isobaths_view.cpp
    triangle.h
    usbl_view.h             usbl_view.cpp
    contacts.h              contacts.cpp)

target_include_directories(KoggerApp PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
