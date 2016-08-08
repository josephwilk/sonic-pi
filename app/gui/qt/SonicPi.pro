#--
# This file is part of Sonic Pi: http://sonic-pi.net
# Full project source: https://github.com/samaaron/sonic-pi
# License: https://github.com/samaaron/sonic-pi/blob/master/LICENSE.md
#
# Copyright 2013, 2014, 2015, 2016 by Sam Aaron (http://sam.aaron.name).
# All rights reserved.
#
# Permission is granted for use, copying, modification, distribution,
# and distribution of modified versions of this work as long as this
# notice is included.
#++

#-------------------------------------------------
#
# Project created by QtCreator 2014-02-28T14:51:06
#
#-------------------------------------------------

# -- Change to match the location of QScintilla on your system
#
#--
# This file is part of Sonic Pi: http://sonic-pi.net
# Full project source: https://github.com/samaaron/sonic-pi
# License: https://github.com/samaaron/sonic-pi/blob/master/LICENSE.md
#
# Copyright 2013, 2014, 2015, 2016 by Sam Aaron (http://sam.aaron.name).
# All rights reserved.
#
# Permission is granted for use, copying, modification, distribution,
# and distribution of modified versions of this work as long as this
# notice is included.
#++

#-------------------------------------------------
#
# Project created by QtCreator 2014-02-28T14:51:06
#
#-------------------------------------------------

TARGET = 'sonic-pi'
CONFIG += qscintilla2 qwt c++11
QMAKE_MAC_SDK = macosx10.11
QT += core gui concurrent network 
greaterThan(QT_MAJOR_VERSION, 4) {
  QT += widgets
}


# Linux only
unix:!macx {
  LIBS += -lrt
  lessThan(QT_MAJOR_VERSION, 5) {
    LIBS += -lqscintilla2
  } else {
    LIBS += -lqt5scintilla2
  }
  QMAKE_CXXFLAGS += -std=gnu++11
  QMAKE_CXXFLAGS += -Wall -Werror -Wextra -Wno-unused-variable -Wno-unused-parameter
  debug {
    QMAKE_CXXFLAGS += -ggdb
  }
}

# Mac OS X only
macx {
  QMAKE_CXXFLAGS += -I/usr/local/include
  QMAKE_CXXFLAGS += -Wall -Werror -Wextra -Wno-unused-variable -Wno-unused-parameter
  CONFIG += warn_off
  TARGET = 'Sonic Pi'
  QT += opengl
}

# Windows only
win32 {
  include ( c:/qwt-6.1.2/features/qwt.prf )
  LIBS += -lqscintilla2
  QMAKE_CXXFLAGS += -Ic:/boost_1_61_0
#  QMAKE_CXXFLAGS += /WX
  QMAKE_LFLAGS += /LIBPATH:C:\boost_1_61_0\bin.v2\libs\date_time\build\msvc-12.0\release\link-static\threading-multi
  DEFINES += _CRT_SECURE_NO_WARNINGS _WINSOCK_DEPRECATED_NO_WARNINGS
}

CODECFORSRC = UTF-8
CODECFORTR = UTF-8

TEMPLATE = app

SOURCES += main.cpp \
           mainwindow.cpp \
           sonicpilexer.cpp \
           sonicpiapis.cpp \
           sonicpiscintilla.cpp \
           oschandler.cpp \
           sonicpilog.cpp \
           sonic_pi_osc_server.cpp \
           sonic_pi_udp_osc_server.cpp \
           sonic_pi_tcp_osc_server.cpp \
           sonicpitheme.cpp \
           scope.cpp
win32 {
# have to link these explicitly for some reason
  SOURCES += platform/win/moc_qsciscintilla.cpp \
             platform/win/moc_qsciscintillabase.cpp
}

HEADERS  += mainwindow.h \
            oscpkt.hh \
            udp.hh \
            sonicpilexer.h \
            sonicpilog.h \
            sonicpiapis.h \
            sonicpiscintilla.h \
            oschandler.h \
            sonic_pi_osc_server.h \
            sonic_pi_udp_osc_server.h \
            sonic_pi_tcp_osc_server.h \
            ruby_help.h \
            sonicpitheme.h \
            scope.h

TRANSLATIONS = lang/sonic-pi_de.ts \
               lang/sonic-pi_es.ts \
               lang/sonic-pi_fi.ts \
               lang/sonic-pi_fr.ts \
               lang/sonic-pi_hu.ts \
               lang/sonic-pi_is.ts \
               lang/sonic-pi_it.ts \
               lang/sonic-pi_ja.ts \
               lang/sonic-pi_nb.ts \
               lang/sonic-pi_nl.ts \
               lang/sonic-pi_pl.ts \
               lang/sonic-pi_ro.ts \
               lang/sonic-pi_ru.ts \
               lang/sonic-pi_zh-Hans.ts \

OTHER_FILES += \
    images/copy.png \
    images/cut.png \
    images/new.png \
    images/save.png \
    images/rec.png \
    images/recording_a.png \
    images/recording_b.png \

RESOURCES += \
    SonicPi.qrc \
    help_files.qrc \
    info_files.qrc

RC_FILE = SonicPi.rc

ICON = images/app.icns

win32 {
  install_qsci.files = $$[QT_INSTALL_LIBS]\qscintilla2.dll
  install_qsci.path = release

  install_bat.files = sonic-pi.bat
  install_bat.path = ..\..\..

  INSTALLS += install_qsci install_bat
  # allow to run on XP
  QMAKE_SUBSYSTEM_SUFFIX = ,5.01
}

# not unicode ready
win32 {
  DEFINES -= UNICODE
  DEFINES += _MBCS
  DEFINES += NOMINMAX
}