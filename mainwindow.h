#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTreeWidget>
#include <QLineEdit>
#include <QTextEdit>
#include <QWidget>

// 自定义地图画布
class MapCanvas : public QWidget {
    Q_OBJECT
public:
    MapCanvas(QWidget *parent = nullptr);
    void loadDemInfo(const QString &path);
protected:
    void paintEvent(QPaintEvent *event) override;
private:
    bool m_hasData;
    QString m_demPath;
};

// 主窗口
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow();
private slots:
    void slotImportMap();
    void slotSearchFeature();
private:
    void initUI();
    MapCanvas *m_canvas;
    QTreeWidget *m_layerTree;
    QLineEdit *m_searchEdit;
    QTextEdit *m_logOutput;
};

#endif // MAINWINDOW_H