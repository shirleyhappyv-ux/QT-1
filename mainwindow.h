#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QToolBar>
#include <QLabel>
#include <QListWidget>
#include <QLineEdit>
#include <QTextEdit>
#include <QPushButton>
#include <QAction>
#include "mapcanvas.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // 🚀 对齐你原本的槽函数命名
    void slotImportMap();
    void slotSearchFeature();

private:
    MapCanvas   *m_canvas;     // 中央画布指针
    QListWidget *m_layerList;   // 左侧图层列表
    QLineEdit   *m_searchEdit;  // 右侧搜索输入框
    QTextEdit   *m_logOutput;   // 右侧 HUD 状态输出（就是报错里的 m_logOutput）
};

#endif // MAINWINDOW_H
