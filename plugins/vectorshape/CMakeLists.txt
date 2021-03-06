project( vectorshape)

add_definitions(-DTRANSLATION_DOMAIN=\"calligra_shape_vector\")

include_directories(${FLAKE_INCLUDES} ${VECTORIMAGE_INCLUDES})

set ( VectorShape_SRCS
    VectorDebug.cpp
    VectorShapePlugin.cpp
    VectorShape.cpp
    VectorShapeConfigWidget.cpp
    VectorShapeFactory.cpp
    #VectorCollection.cpp
    #VectorData.cpp
    VectorTool.cpp
    VectorToolFactory.cpp
    ChangeVectorDataCommand.cpp
)

#ki18n_wrap_ui(VectorShape_SRCS
#)

add_library(calligra_shape_vector MODULE ${VectorShape_SRCS})
kcoreaddons_desktop_to_json(calligra_shape_vector calligra_shape_vector.desktop)

target_link_libraries(calligra_shape_vector
    flake
    kovectorimage
    KF5::I18n
    KF5::KIOFileWidgets
    Qt5::Svg
)

install(TARGETS calligra_shape_vector DESTINATION ${PLUGIN_INSTALL_DIR}/calligra/shapes)

########### install files ###############

if (SHARED_MIME_INFO_FOUND)
    install(FILES calligra_svm.xml DESTINATION ${XDG_MIME_INSTALL_DIR})
    update_xdg_mimetypes(${XDG_MIME_INSTALL_DIR})
endif ()

########### subdirectories ###############

#add_subdirectory( libemf )
