cmake_minimum_required(VERSION 3.19)

set(PROGRAM_NAME qtvtk)

project(${PROGRAM_NAME} VERSION 0.1.0 LANGUAGES C CXX)
set(CMAKE_AUTOMOC ON)
set(QT_GENERATE_QMLLS_INI ON)

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
        RenderingFreeType
    HINTS
        "D:/lib/vtk"
)

find_package(copick3d_api REQUIRED
    HINTS "D:/repos/copick3d_api/build/install"
)

qt_add_executable(${PROGRAM_NAME}
    MANUAL_FINALIZATION
    src/main.cpp
    src/graphics/PointCloudActor.cpp
    src/graphics/PointCloudView.cpp
    src/graphics/PointCloudLoader.cpp

    src/graphics/qvtk/QQuickVTKItem.cxx
    src/graphics/qvtk/QQuickVTKInteractor.cxx
    src/graphics/qvtk/QQuickVTKInteractorAdapter.cxx

    src/graphics/vtk/vtkCameraOrientationRepresentationEx.cxx
    src/graphics/vtk/vtkCameraOrientationWidgetEx.cxx
)

qt6_add_resources(${PROGRAM_NAME} ${PROGRAM_NAME}
    PREFIX  "/"
    BASE    "qml/"
    FILES
        "qml/main.qml"
)

# if (VTK_QT_VERSION EQUAL 6)
#     qt_import_qml_plugins(${PROGRAM_NAME})
#     qt_finalize_executable(${PROGRAM_NAME})
# endif()

target_link_libraries(${PROGRAM_NAME} PRIVATE
    copick3d_api::copick3d_api
    Qt6::Core 
    Qt6::Quick 
    ${VTK_LIBRARIES}
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

target_compile_options(${PROGRAM_NAME} PRIVATE "/Zi") # Enable debug information for Release builds
target_link_options(${PROGRAM_NAME} PRIVATE "/DEBUG") # Link with debug information for Release builds