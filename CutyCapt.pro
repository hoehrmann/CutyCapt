QT       +=  webkit svg network
SOURCES   =  CutyCapt.cpp
HEADERS   =  CutyCapt.hpp
CONFIG   +=  qt console

greaterThan(QT_MAJOR_VERSION, 4): {
  QT       +=  webkitwidgets
}

contains(CONFIG, static): {
  QTPLUGIN += qjpeg qgif qsvg qmng qico qtiff
  DEFINES  += STATIC_PLUGINS
}

mac {
  QMAKE_CXXFLAGS += -fvisibility=hidden
  QMAKE_LFLAGS += '-sectcreate __TEXT __info_plist Info.plist'
  CONFIG   -= app_bundle
}
