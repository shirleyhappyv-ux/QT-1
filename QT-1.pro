QT += core gui widgets

TEMPLATE = app
TARGET = QT-1
INCLUDEPATH += .

# 源文件指向你的 main.cpp
SOURCES += main.cpp

# 核心：引入 Alpine 的 GDAL 编译依赖链接（分两行写，最稳妥）
CONFIG += link_pkgconfig
PKGCONFIG += gdal