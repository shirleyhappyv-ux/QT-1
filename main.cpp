#include <QApplication>
#include "mainwindow.h"
#include <QFont>
#include <QFontDatabase>
#include <QDebug>
#include <QDir>

int main(int argc, char *argv[]) {
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication app(argc, argv);

    // 🚀 物理绝杀：强制读取我们刚刚下载到项目根目录的字体文件
    QString fontPath = QCoreApplication::applicationDirPath() + "/local_font.ttc";
    int fontId = QFontDatabase::addApplicationFont(fontPath);
    
    if (fontId != -1) {
        QString fontFamily = QFontDatabase::applicationFontFamilies(fontId).at(0);
        QFont defaultFont(fontFamily, 10);
        app.setFont(defaultFont); 
        qDebug() << "🛰️ [物理注入成功] 全局默认字体已锁死为：" << fontFamily;
    } else {
        // 如果 local_font.ttc 读取失败，强制选用 Qt 内置可用的任何系统无衬线字体兜底
        QFont fallbackFont("Liberation Sans", 10);
        app.setFont(fallbackFont);
        qDebug() << "⚠️ 本地字体文件读取失败，已切换至内置 Sans 字体";
    }

    MainWindow w;
    w.resize(1280, 800);
    w.show();

    return app.exec();
}
