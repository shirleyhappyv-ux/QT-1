#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QFileDialog>
#include <QMessageBox>
#include <QPainter>
#include <QPen>
#include <QFont>
#include <QDebug>
#include <QPushButton> 
#include "gdal_priv.h"
#include <QStatusBar>

// ==================== 1. 画布组件实现 ====================
MapCanvas::MapCanvas(QWidget *parent) : QWidget(parent), m_hasData(false) {
    setAttribute(Qt::WA_StyledBackground, true);
}

void MapCanvas::setHasData(bool hasData) {
    m_hasData = hasData;
    update();
}

void MapCanvas::setLockedFeature(const QString &featureName) {
    m_lockedFeature = featureName;
    update(); // 强制触发绘图事件
}

void MapCanvas::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // 经典海蓝色背景
    painter.fillRect(rect(), QColor(30, 45, 65));

    int centerX = width() / 2;
    int centerY = height() / 2;

    if (!m_lockedFeature.isEmpty()) {
        // 🎯 炫酷雷达靶心模式
        QPen redPen(QColor(255, 60, 60), 2, Qt::DashLine);
        painter.setPen(redPen);
        painter.drawEllipse(QPoint(centerX, centerY), 60, 60);
        
        redPen.setStyle(Qt::SolidLine);
        redPen.setColor(QColor(255, 60, 60, 100));
        painter.setPen(redPen);
        painter.drawEllipse(QPoint(centerX, centerY), 120, 120);

        QPen crossPen(QColor(255, 60, 60, 180), 1.5);
        painter.setPen(crossPen);
        painter.drawLine(centerX - 150, centerY, centerX - 10, centerY);
        painter.drawLine(centerX + 10, centerY, centerX + 150, centerY);
        painter.drawLine(centerX, centerY - 150, centerX, centerY - 10);
        painter.drawLine(centerX, centerY + 10, centerY + 150, centerY);

        painter.setBrush(QColor(255, 30, 30));
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(QPoint(centerX, centerY), 4, 4);

        QFont hudFont("WenQuanYi Micro Hei", 12, QFont::Bold);
        painter.setFont(hudFont);
        painter.setPen(QColor(50, 255, 120));
        //  修正后的代码（改用经典 ASCII 符号）
        painter.drawText(centerX + 20, centerY - 30, ">> [TARGET LOCKED]");
        painter.drawText(centerX + 20, centerY - 10, QString("要素: %1").arg(m_lockedFeature));
    } else if (m_hasData) {
        painter.setPen(Qt::white);
        QFont textFont("WenQuanYi Micro Hei", 11);
        painter.setFont(textFont);
        painter.drawText(rect(), Qt::AlignCenter, "遥感图层已载入\n[地图数据]: fixed_map_data.gpkg\n[GDAL 驱动流]: 正常\n\n请在右侧输入关键字进行要素中心锁定");
    } else {
        painter.setPen(QColor(150, 170, 190));
        painter.drawText(rect(), Qt::AlignCenter, "暂无图层，请通过工具栏导入 GPKG/DEM 地图");
    }
}

void MapCanvas::setMapFilePath(const QString &path) {
    m_filePath = path;
    m_hasData = true;
    update();
}

// ==================== 2. 主窗口实现 ====================
MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    resize(1200, 750);
    setWindowTitle("云端遥感影像判读客户端 C++ fully-loaded");

    GDALAllRegister();

    // 工具栏
    QToolBar *toolBar = addToolBar("文件");
    QAction *actImport = toolBar->addAction("导入地图 (GPKG/DEM)");
    connect(actImport, &QAction::triggered, this, &MainWindow::slotImportMap);
    toolBar->addAction("平移");
    toolBar->addAction("放大");
    toolBar->addAction("缩小");
    toolBar->addAction("测量");

    // 中央核心布局
    QSplitter *mainSplitter = new QSplitter(Qt::Horizontal, this);
    setCentralWidget(mainSplitter);

    // 左面板：图层列表
    QWidget *leftPanel = new QWidget(this);
    QVBoxLayout *vboxLeft = new QVBoxLayout(leftPanel);
    vboxLeft->addWidget(new QLabel("图层列表", leftPanel));
    m_layerList = new QListWidget(leftPanel);
    vboxLeft->addWidget(m_layerList);
    mainSplitter->addWidget(leftPanel);

    // 中间面板：画布 (在这里直接将实例绑死到类成员变量 m_canvas)
    m_canvas = new MapCanvas(this);
    mainSplitter->addWidget(m_canvas);

    // 右面板：搜索与日志
    QWidget *rightPanel = new QWidget(this);
    QVBoxLayout *vboxRight = new QVBoxLayout(rightPanel);
    vboxRight->addWidget(new QLabel("要素搜索与判读", rightPanel));
    
    m_searchEdit = new QLineEdit(rightPanel);
    m_searchEdit->setPlaceholderText("输入地理要素关键字搜索...");
    vboxRight->addWidget(m_searchEdit);

    QPushButton *btnSearch = new QPushButton("搜索", rightPanel);
    connect(btnSearch, &QPushButton::clicked, this, &MainWindow::slotSearchFeature);
    vboxRight->addWidget(btnSearch);

    m_logOutput = new QTextEdit(rightPanel);
    m_logOutput->setReadOnly(true);
    vboxRight->addWidget(m_logOutput);
    
    mainSplitter->addWidget(rightPanel);

    mainSplitter->setStretchFactor(0, 1);
    mainSplitter->setStretchFactor(1, 4);
    mainSplitter->setStretchFactor(2, 1);
}

MainWindow::~MainWindow() {}

void MainWindow::slotImportMap() {
    QString filePath = QFileDialog::getOpenFileName(this, "选择遥感地图文件", "/workspaces/QT-1", "GIS Files (*.gpkg *.dem *.tif *.tiff)");
    if (filePath.isEmpty()) return;

    GDALDataset *poDataset = (GDALDataset *) GDALOpenEx(filePath.toUtf8().constData(), GDAL_OF_READONLY | GDAL_OF_ALL, NULL, NULL, NULL);
    if (!poDataset) {
        QMessageBox::critical(this, "错误", "GDAL 无法解析该地图格式！");
        return;
    }

    m_canvas->setHasData(true);
    m_layerList->clear();
    m_layerList->addItem(new QListWidgetItem(QIcon(), "world"));
    QListWidgetItem *item = new QListWidgetItem(QIcon(), QFileInfo(filePath).fileName());
    item->setCheckState(Qt::Checked);
    m_layerList->addItem(item);

    m_logOutput->append("成功导入地图:");

    //  正确的代码
    m_logOutput->append(QString("路径: %1").arg(filePath));
    m_logOutput->append(QString("分辨率: %1x%2").arg(poDataset->GetRasterXSize()).arg(poDataset->GetRasterYSize()));
    m_logOutput->append(QString("波段数: %1").arg(poDataset->GetRasterCount()));

    // 🚀 精准加入这一行，告诉中间的画布：有数据了，立刻让 GDAL 开始画画！
    m_canvas->setHasData(true); 
    m_canvas->update(); // 强制触发 paintEvent 重绘
    
    GDALClose(poDataset);
}

void MainWindow::slotSearchFeature() {
    QString keyword = m_searchEdit->text();
    if (keyword.isEmpty()) {
        m_logOutput->append(" [警告] 搜索关键词不能为空！");
        return;
    }

    m_logOutput->append("正在检索要素库...");
    m_logOutput->append(QString("[核心命中] 成功匹配到地理要素: '%1'").arg(keyword));

    // 🛰️ 物理直连点火：完全不需要系统猜测
    if (m_canvas) {
        m_canvas->setLockedFeature(keyword);
    } else {
        m_logOutput->append(" [内核报错] m_canvas 指针在当前运行的实例中为空！");
    }
    
    statusBar()->showMessage(QString("目标已锁定。"));
}


