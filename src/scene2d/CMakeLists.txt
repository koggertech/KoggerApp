target_sources(KoggerApp PRIVATE
    plot2D.h                     plot2D.cpp
    plot2D_aim.h                 plot2D_aim.cpp
    plot2D_attitude.h            plot2D_attitude.cpp
    plot2D_bottom_processing.h   plot2D_bottom_processing.cpp
    plot2D_contact.h             plot2D_contact.cpp
    plot2D_defs.h
    plot2D_dvl_beam_velocity.h   plot2D_dvl_beam_velocity.cpp
    plot2D_dvl_solution.h        plot2D_dvl_solution.cpp
    plot2D_echogram.h            plot2D_echogram.cpp
    plot2D_encoder.h             plot2D_encoder.cpp
    plot2D_gnss.h                plot2D_gnss.cpp
    plot2D_grid.h                plot2D_grid.cpp
    plot2D_temperature.h         plot2D_temperature.cpp
    plot2D_line.h                plot2D_line.cpp
    plot2D_plot_layer.h
    plot2D_quadrature.h          plot2D_quadrature.cpp
    plot2D_rangefinder.h         plot2D_rangefinder.cpp
    plot2D_usbl_solution.h       plot2D_usbl_solution.cpp
    plot2D_depth.h               plot2D_depth.cpp
    qPlot2D.h                    qPlot2D.cpp)

target_include_directories(KoggerApp PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
