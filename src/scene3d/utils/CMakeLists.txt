target_sources(KoggerApp PRIVATE
    abstract_entity_data_filter.h
    draw_utils.h            draw_utils.cpp
    geojson_defs.h
    geojson_feature_model.h geojson_feature_model.cpp
    geojson_io.h            geojson_io.cpp
    geojson_style.h         geojson_style.cpp
    max_points_filter.h     max_points_filter.cpp
    nearest_point_filter.h  nearest_point_filter.cpp
    ray.h                   ray.cpp
    ray_caster.h            ray_caster.cpp
    text_renderer.h         text_renderer.cpp)

target_include_directories(KoggerApp PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
