cmake_minimum_required(VERSION 3.19)

set(PROGRAM_NAME qtvtk)

project(${PROGRAM_NAME} VERSION 0.1.0 LANGUAGES C CXX)
set(CMAKE_AUTOMOC ON)
set(QT_GENERATE_QMLLS_INI ON)
set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/install)

find_package(Qt6 REQUIRED 
    COMPONENTS
        Core 
        Quick
        Qml
)

find_package(VTK REQUIRED
    COMPONENTS
        CommonCore
        CommonColor
        FiltersCore
        FiltersPoints
        GUISupportQtQuick
        InteractionStyle
        RenderingAnnotation
    HINTS
        "D:/lib/vtk/"
)

find_package(copick3d_api REQUIRED
    HINTS "D:/repos/copick3d_api/build/install"
)

set_target_properties(copick3d_api::copick3d_api PROPERTIES
    MAP_IMPORTED_CONFIG_MINSIZEREL Release
    MAP_IMPORTED_CONFIG_RELWITHDEBINFO Release
)

qt_standard_project_setup()

qt_add_executable(${PROGRAM_NAME}
    MANUAL_FINALIZATION
    src/main.cpp

    src/graphics/ColormapPreset.cpp
    src/graphics/PointCloudActor.cpp
    src/graphics/PointCloudView.cpp
    src/graphics/PointCloudLoader.cpp

    src/graphics/vtk/CoPickInteractorStyle.cpp
    src/graphics/vtk/DepthFilter.cpp
    src/graphics/vtk/vtkCameraOrientationWidgetEx.cpp
    src/graphics/vtk/QQuickVTKItemEx.cpp
)

qt_add_qml_module(${PROGRAM_NAME}
    URI Application
    VERSION 1.0
    QML_FILES qml/main.qml
)

qt6_add_resources(${PROGRAM_NAME} ${PROGRAM_NAME}
    PREFIX  "/"
    BASE    "qml/"
    FILES
        "qml/main.qml"
)

qt_finalize_executable(${PROGRAM_NAME})

target_link_libraries(${PROGRAM_NAME} PRIVATE
    copick3d_api::copick3d_api
    Qt6::Core 
    Qt6::Quick 
    ${VTK_LIBRARIES}
    VTK::FiltersPoints
)

set_target_properties(${PROGRAM_NAME} PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

add_custom_command(TARGET ${PROGRAM_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:copick3d_api::copick3d_api>
        $<TARGET_FILE_DIR:${PROGRAM_NAME}>
)

install(TARGETS ${PROGRAM_NAME}
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

qt_generate_deploy_qml_app_script(
    TARGET ${PROGRAM_NAME}
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)

install(SCRIPT ${deploy_script})

if (WIN32)
    install(FILES
        $<TARGET_RUNTIME_DLLS:${PROGRAM_NAME}>
        DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endif()
