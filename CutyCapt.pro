QT       +=  webkit svg network
SOURCES   =  CutyCapt.cpp
HEADERS   =  CutyCapt.hpp
CONFIG   +=  qt console
target.path = /usr/local/bin/
INSTALLS += target

greaterThan(QT_MAJOR_VERSION, 4): {
  QT       +=  webkitwidgets
}

contains(CONFIG, static): {
  QTPLUGIN += qjpeg qgif qsvg qmng qico qtiff
  DEFINES  += STATIC_PLUGINS
}

