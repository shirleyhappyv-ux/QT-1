QT += core gui widgets

TEMPLATE = app
TARGET = QT-1

# 头文件
HEADERS += mainwindow.h

# 源文件
SOURCES += main.cpp \
           mainwindow.cpp

# 引入 GDAL 依赖
CONFIG += link_pkgconfig
PKGCONFIG += gdal