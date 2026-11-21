# QML plugin of Settings > Apps > Waydroid (settings/WaydroidSettings.qml):
# whether Android is set up, and its removal. The spec builds and installs it
# after the app.

TEMPLATE = lib
TARGET = waydroidrunnerplugin
CONFIG += qt plugin hide_symbols
QT = core qml dbus

INCLUDEPATH += ../src

SOURCES += \
    plugin.cpp \
    android.cpp

HEADERS += \
    android.h \
    ../src/androidstate.h

target.path = $$[QT_INSTALL_QML]/org/sailfishosopen/waydroidrunner
qmldir.files = qmldir
qmldir.path = $$target.path
INSTALLS += target qmldir
