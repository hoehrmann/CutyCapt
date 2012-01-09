QT       +=  webkit svg network
SOURCES   =  CutyCapt.cpp
HEADERS   =  CutyCapt.hpp
CONFIG   +=  qt console

contains(CONFIG, static): {
  QTPLUGIN += qjpeg qgif qsvg qmng qico qtiff
  DEFINES  += STATIC_PLUGINS
}

