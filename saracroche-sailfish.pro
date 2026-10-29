TARGET = saracroche-sailfish

CONFIG += sailfishapp link_pkgconfig c++11
QT += network sql
PKGCONFIG += sailfishapp mlite5

SOURCES += src/main.cpp
HEADERS += src/patterns.h

DISTFILES += \
    qml/saracroche-sailfish.qml \
    qml/pages/MainPage.qml \
    rpm/saracroche-sailfish.spec \
    saracroche-sailfish.desktop

SAILFISHAPP_ICONS = 86x86 108x108 128x128 172x172

systemd.files = systemd/saracroche-sync.service systemd/saracroche-sync.timer
systemd.path = /usr/lib/systemd/user
INSTALLS += systemd
