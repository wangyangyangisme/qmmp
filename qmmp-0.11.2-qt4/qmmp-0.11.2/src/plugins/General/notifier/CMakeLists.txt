project(libnotifier)

# qt plugin
ADD_DEFINITIONS( -Wall )
ADD_DEFINITIONS(${QT_DEFINITIONS})
ADD_DEFINITIONS(-DQT_PLUGIN)
ADD_DEFINITIONS(-DQT_NO_DEBUG)
ADD_DEFINITIONS(-DQT_SHARED)
ADD_DEFINITIONS(-DQT_THREAD)

include_directories(${CMAKE_CURRENT_BINARY_DIR})

SET(QT_INCLUDES
  ${QT_INCLUDES}
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../
)

# libqmmpui & libqmmp
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../)
link_directories(${CMAKE_CURRENT_BINARY_DIR}/../../../qmmpui)
link_directories(${CMAKE_CURRENT_BINARY_DIR}/../../../qmmp)

SET(libnotifier_SRCS
    settingsdialog.cpp
    notifier.cpp
    notifierfactory.cpp
    popupwidget.cpp
)

SET(libnotifier_RCCS 
    translations/translations.qrc
    notifier_images.qrc)

QT4_ADD_RESOURCES(libnotifier_RCC_SRCS ${libnotifier_RCCS})

# user interface


SET(libnotifier_UIS
    settingsdialog.ui
)

QT4_WRAP_UI(libnotifier_UIS_H ${libnotifier_UIS})
# Don't forget to include output directory, otherwise
# the UI file won't be wrapped!
include_directories(${CMAKE_CURRENT_BINARY_DIR})

ADD_LIBRARY(notifier MODULE ${libnotifier_SRCS} ${libnotifier_UIS_H} ${libnotifier_RCC_SRCS})
add_dependencies(notifier qmmpui)
IF(X11_FOUND)
  include_directories(${X11_INCLUDE_DIRS})
  link_directories(${X11_LIBRARY_DIRS})
  ADD_DEFINITIONS(${X11_CFLAGS})
  ADD_DEFINITIONS(-DX11_FOUND)
  target_link_libraries(notifier ${QT_LIBRARIES} qmmpui libqmmp ${X11_LDFLAGS})
ELSE(X11_FOUND)
  target_link_libraries(notifier ${QT_LIBRARIES} qmmpui libqmmp)
ENDIF(X11_FOUND)
install(TARGETS notifier DESTINATION ${LIB_DIR}/qmmp/General)
