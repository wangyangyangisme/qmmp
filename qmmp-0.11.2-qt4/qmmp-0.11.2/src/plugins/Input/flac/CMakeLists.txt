project(libflac)

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

# libqmmp
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../)
link_directories(${CMAKE_CURRENT_BINARY_DIR}/../../../qmmp)

# libflac
pkg_search_module(FLAC flac)

IF(NOT FLAC_LINK_FLAGS)
  SET(FLAC_LINK_FLAGS -lFLAC)
ENDIF(NOT FLAC_LINK_FLAGS)

include_directories(${FLAC_INCLUDE_DIRS} ${TAGLIB_INCLUDE_DIRS})
link_directories(${FLAC_LIBRARY_DIRS} ${TAGLIB_LIBRARY_DIRS})
ADD_DEFINITIONS(${FLAC_CFLAGS})
ADD_DEFINITIONS(${TAGLIB_CFLAGS})

SET(libflac_SRCS
  decoder_flac.cpp
  decoderflacfactory.cpp
  flacmetadatamodel.cpp
  cueparser.cpp
  replaygainreader.cpp
)

SET(libflac_HDRS
  decoder_flac.h
  cueparser.h
  replaygainreader.h
)

SET(libflac_RCCS translations/translations.qrc)

QT4_ADD_RESOURCES(libflac_RCC_SRCS ${libflac_RCCS})

# Don't forget to include output directory, otherwise
# the UI file won't be wrapped!
include_directories(${CMAKE_CURRENT_BINARY_DIR})

IF(FLAC_FOUND)
ADD_LIBRARY(flac MODULE ${libflac_SRCS} ${libflac_RCC_SRCS} ${libflac_HDRS})
add_dependencies(flac qmmp)
target_link_libraries(flac ${QT_LIBRARIES} libqmmp ${FLAC_LDFLAGS} ${TAGLIB_LDFLAGS})
install(TARGETS flac DESTINATION ${LIB_DIR}/qmmp/Input)
ENDIF(FLAC_FOUND)
