TEMPLATE = app
TARGET = tst_patterns
QT += testlib
QT -= gui
CONFIG += console c++11
CONFIG -= app_bundle
SOURCES += tst_patterns.cpp
HEADERS += ../src/patterns.h
