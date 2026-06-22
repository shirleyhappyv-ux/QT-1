#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QWidget>
#include <QLineEdit>
#include <QTextEdit>
#include <QListWidget>
#include <QLabel>
#include <QToolBar>
#include <QStatusBar>

// ==================== 1. 画布组件声明 ====================
class MapCanvas : public QWidget {
    Q_OBJECT
public:
    explicit MapCanvas(QWidget *parent = nullptr);
    void setHasData(bool hasData);
    void setLockedFeature(const QString &featureName);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    bool m_hasData;
    QString m_lockedFeature;
};

// ==================== 2. 主窗口声明 ====================
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    virtual ~MainWindow();

private slots:
    void slotImportMap();
    void slotSearchFeature();

private:
    MapCanvas *m_canvas;        // 物理绑定中央画布
    QLineEdit *m_searchEdit;    // 搜索输入框
    QTextEdit *m_logOutput;     // 日志输出框
    QListWidget *m_layerList;   // 左侧图层列表
};

#endif // MAINWINDOW_H
