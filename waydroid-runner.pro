# NOTICE:
#
# Application name defined in TARGET has a corresponding QML filename.
# If name defined in TARGET is changed, the following needs to be done
# to match new name:
#   - corresponding QML filename must be changed
#   - desktop icon filename must be changed
#   - desktop filename must be changed
#   - icon definition filename in desktop file must be changed
#   - translation filenames have to be changed

# The name of your application
TARGET = waydroid-runner

# PREFIX
isEmpty(PREFIX) {
    PREFIX = /usr
}

target.path = $$PREFIX/bin

CONFIG += sailfishapp sailfishapp_i18n

DEFINES += QT_COMPOSITOR_QUICK
DEFINES += APP_VERSION=\\\"$$VERSION\\\"

QT += quick qml svg
QT += quick-private

QT += dbus

QT += compositor

# MDConfItem: the setting that Settings > Apps > Waydroid writes. No
# CONFIG += link_pkgconfig here: sailfishapp.prf adds it together with its
# own PKGCONFIG, which an earlier link_pkgconfig would miss (no
# -lsailfishapp)
PKGCONFIG += mlite5

SOURCES += \
    src/main.cpp \
    src/qmlcompositor.cpp \
    src/runner.cpp \
    src/service.cpp \
    src/setup.cpp

OTHER_FILES += \
    qml/main.qml \
    qml/MainPage.qml \
    qml/CoverPage.qml \
    qml/SetupView.qml \
    qml/WindowContainer.qml \
    settings/$${TARGET}.json \
    settings/WaydroidSettings.qml \

DISTFILES += \
    rpm/waydroid-runner.spec

HEADERS += \
    src/qmlcompositor.h \
    src/runner.h \
    src/service.h \
    src/setup.h

icons.files = icons
icons.path = $$PREFIX/share/$${TARGET}
INSTALLS += icons

# Background mode: D-Bus activation starts the user unit
systemd.files = systemd/$${TARGET}.service
systemd.path = $$PREFIX/lib/systemd/user
dbus_service.files = dbus/org.sailfishosopen.WaydroidRunner.service
dbus_service.path = $$PREFIX/share/dbus-1/services
INSTALLS += systemd dbus_service

# Settings > Apps > Waydroid
settings_entry.files = settings/$${TARGET}.json
settings_entry.path = $$PREFIX/share/jolla-settings/entries
settings_page.files = settings/WaydroidSettings.qml
settings_page.path = $$PREFIX/share/jolla-settings/pages/$${TARGET}
INSTALLS += settings_entry settings_page

SAILFISHAPP_ICONS = 86x86 108x108 128x128 172x172 256x256 512x512
DISTFILES += $${TARGET}.desktop

# translations
TRANSLATIONS += \
    translations/$${TARGET}-de_DE.ts \
