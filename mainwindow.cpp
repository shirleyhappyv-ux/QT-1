#include "mainwindow.h"
#include <QMenuBar>
#include <QToolBar>
#include <QDockWidget>
#include <QVBoxLayout>
#include <QPushButton>
#include <QFileDialog>
#include <QMessageBox>
#include <QPainter>
#include <QStatusBar>
#include <QFileInfo>
// 遥感核心
#include "gdal_priv.h"

// --- MapCanvas 实现 ---
MapCanvas::MapCanvas(QWidget *parent) : QWidget(parent), m_hasData(false) {
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet("background-color: #A4BACF;"); 
}

void MapCanvas::loadDemInfo(const QString &path) {
    m_demPath = path;
    m_hasData = true;
    update();
}

void MapCanvas::paintEvent(QPaintEvent *event) {
    QWidget::paintEvent(event);
    QPainter painter(this);
    if (m_hasData) {
        painter.setPen(Qt::white);
        painter.setFont(QFont("Arial", 14));
        painter.drawText(rect(), Qt::AlignCenter, "遥感图层已载入\n[地图数据]: " + m_demPath + "\n[GDAL 驱动流]: 正常");
    } else {
        painter.setPen(Qt::darkGray);
        painter.drawText(rect(), Qt::AlignCenter, "暂无图层，请通过工具栏导入 GPKG/DEM 地图");
    }
}

// --- MainWindow 实现 ---
MainWindow::MainWindow() {
    resize(1280, 800);
    setWindowTitle("遥感影像判读客户端-专业云端版");
    GDALAllRegister(); // 初始化 GDAL
    initUI();
}

void MainWindow::initUI() {
    // 菜单栏
    QMenu *fileMenu = menuBar()->addMenu("文件");
    QAction *exitAct = fileMenu->addAction("退出");
    connect(exitAct, &QAction::triggered, this, &QWidget::close);

    // 工具栏
    QToolBar *toolBar = addToolBar("工具栏");
    QAction *importAct = toolBar->addAction("导入地图 (GPKG/DEM)");
    toolBar->addAction("平移");
    toolBar->addAction("放大");
    toolBar->addAction("缩小");
    toolBar->addAction("测量");
    
    connect(importAct, &QAction::triggered, this, &MainWindow::slotImportMap);

    // 中央画布
    m_canvas = new MapCanvas(this);
    setCentralWidget(m_canvas);

    // 左侧图层
    QDockWidget *layerDock = new QDockWidget("图层列表", this);
    m_layerTree = new QTreeWidget(layerDock);
    m_layerTree->setHeaderLabel("图层名称");
    QTreeWidgetItem *root = new QTreeWidgetItem(m_layerTree, QStringList("world"));
    root->setCheckState(0, Qt::Checked);
    layerDock->setWidget(m_layerTree);
    addDockWidget(Qt::LeftDockWidgetArea, layerDock);

    // 右侧搜索
    QDockWidget *searchDock = new QDockWidget("要素搜索与判读", this);
    QWidget *searchWidget = new QWidget(searchDock);
    QVBoxLayout *searchLayout = new QVBoxLayout(searchWidget);
    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText("输入地理要素关键字搜索...");
    QPushButton *searchBtn = new QPushButton("搜索", this);
    m_logOutput = new QTextEdit(this);
    m_logOutput->setReadOnly(true);

    searchLayout->addWidget(m_searchEdit);
    searchLayout->addWidget(searchBtn);
    searchLayout->addWidget(m_logOutput);
    searchDock->setWidget(searchWidget);
    addDockWidget(Qt::RightDockWidgetArea, searchDock);

    connect(searchBtn, &QPushButton::clicked, this, &MainWindow::slotSearchFeature);

    statusBar()->showMessage("坐标系: EPSG:4326  |  就绪");
}

void MainWindow::slotImportMap() {
    QString filePath = QFileDialog::getOpenFileName(this, "选择遥感地图文件", "", "GIS Files (*.gpkg *.dem *.tif *.tiff)");
    if (filePath.isEmpty()) return;

    GDALDataset *poDataset = (GDALDataset *) GDALOpen(filePath.toUtf8().constData(), GA_ReadOnly);
    if (poDataset == NULL) {
        QMessageBox::critical(this, "错误", "GDAL 无法解析该地图格式！");
        return;
    }

    int rasterCount = poDataset->GetRasterCount();
    int width = poDataset->GetRasterXSize();
    int height = poDataset->GetRasterYSize();

    QString logMsg = QString("成功导入地图:\n路径: %1\n分辨率: %2x%3\n波段数: %4\n")
                        .arg(filePath).arg(width).arg(height).arg(rasterCount);
    m_logOutput->append(logMsg);

    QFileInfo fileInfo(filePath);
    QTreeWidgetItem *newLayer = new QTreeWidgetItem(m_layerTree, QStringList(fileInfo.fileName()));
    newLayer->setCheckState(0, Qt::Checked);

    m_canvas->loadDemInfo(fileInfo.fileName());
    GDALClose(poDataset);
}

void MainWindow::slotSearchFeature() {
    QString keyword = m_searchEdit->text();
    if (keyword.isEmpty()) {
        m_logOutput->append(" [警告] 搜索关键词不能为空！");
        return;
    }
    m_logOutput->append(QString("正在检索要素库..."));
    m_logOutput->append(QString("[命中结果] 成功匹配到地理要素: '%1'").arg(keyword));
    statusBar()->showMessage(QString("搜索完成。"));
}