include_directories(
    ${CMAKE_SOURCE_DIR}/sheets
    ${KOMAIN_INCLUDES}
)


if(SHOULD_BUILD_FILTER_SHEETS_TO_GNUMERIC)

set(sheets2gnumeric_PART_SRCS gnumericexport.cc )
add_library(calligra_filter_sheets2gnumeric MODULE ${sheets2gnumeric_PART_SRCS})
kcoreaddons_desktop_to_json(calligra_filter_sheets2gnumeric calligra_filter_sheets2gnumeric.desktop)
target_link_libraries(calligra_filter_sheets2gnumeric calligrasheetscommon kowidgets KF5::Archive)

install(TARGETS calligra_filter_sheets2gnumeric  DESTINATION ${PLUGIN_INSTALL_DIR}/calligra/formatfilters)


endif()


if(SHOULD_BUILD_FILTER_GNUMERIC_TO_SHEETS)

set(gnumeric2sheets_PART_SRCS gnumericimport.cc )
add_library(calligra_filter_gnumeric2sheets MODULE ${gnumeric2sheets_PART_SRCS})
kcoreaddons_desktop_to_json(calligra_filter_gnumeric2sheets calligra_filter_gnumeric2sheets.desktop)
target_link_libraries(calligra_filter_gnumeric2sheets calligrasheetscommon kowidgets KF5::Archive)
install(TARGETS calligra_filter_gnumeric2sheets  DESTINATION ${PLUGIN_INSTALL_DIR}/calligra/formatfilters)


endif()
