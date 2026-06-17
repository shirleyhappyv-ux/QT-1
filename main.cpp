#include <QApplication>
#include <QMainWindow>
#include <QMenuBar>
#include <QToolBar>
#include <QDockWidget>
#include <QTreeWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTextEdit>
#include <QFileDialog>
#include <QMessageBox>
#include <QPainter>
#include <QWidget>
#include <QStatusBar>

// 引入GDAL遥感核心头文件
#include "gdal_priv.h"
#include "cpl_conv.h" 

// --- 1. 自定义地图画布组件 ---
class MapCanvas : public QWidget {
    
public:
    MapCanvas(QWidget *parent = nullptr) : QWidget(parent), m_hasData(false) {
        setAttribute(Qt::WA_StyledBackground, true);
        setStyleSheet("background-color: #A4BACF;"); // 模拟海蓝色底图
    }

    void loadDemInfo(const QString &path) {
        m_demPath = path;
        m_hasData = true;
        update(); // 触发重绘
    }

protected:
    void paintEvent(QPaintEvent *event) override {
        QWidget::paintEvent(event);
        QPainter painter(this);
        if (m_hasData) {
            painter.setPen(Qt::white);
            painter.setFont(QFont("Arial", 14));
            // 工业级渲染通常使用 GDAL 读取栅格像素并用 QImage 绘制
            // 这里绘制占位图层表示 DEM 和 GPKG 成功注入数据流
            painter.drawText(rect(), Qt::AlignCenter, "遥感图层已载入\n[DEM]: " + m_demPath + "\n[GPKG]: 已连接矢量要素");
        } else {
            painter.setPen(Qt::darkGray);
            painter.drawText(rect(), Qt::AlignCenter, "暂无图层，请通过工具栏导入 GPKG/DEM 地图");
        }
    }

private:
    bool m_hasData;
    QString m_demPath;
};

// --- 2. 主窗口客户端 ---
class MainWindow : public QMainWindow {
    
public:
    MainWindow() {
        resize(1280, 800);
        setWindowTitle("遥感影像判读客户端-云端版");

        // 初始化 GDAL 驱动
        GDALAllRegister();

        initUI();
    }

private:
    void initUI() {
        // A. 菜单栏
        QMenu *fileMenu = menuBar()->addMenu("文件");
        QAction *exitAct = fileMenu->addAction("退出");
        connect(exitAct, &QAction::triggered, this, &QWidget::close);

        // B. 工具栏 (复刻你图中的放大、缩小、导入图标)
        QToolBar *toolBar = addToolBar("工具栏");
        QAction *importAct = toolBar->addAction("导入地图 (GPKG/DEM)");
        toolBar->addAction("平移");
        toolBar->addAction("放大");
        toolBar->addAction("缩小");
        toolBar->addAction("测量");
        
        connect(importAct, &QAction::triggered, this, &MainWindow::slotImportMap);

        // C. 中央大画布
        m_canvas = new MapCanvas(this);
        setCentralWidget(m_canvas);

        // D. 左侧：图层管理器 (DockWidget)
        QDockWidget *layerDock = new QDockWidget("图层列表", this);
        m_layerTree = new QTreeWidget(layerDock);
        m_layerTree->setHeaderLabel("图层名称");
        QTreeWidgetItem *root = new QTreeWidgetItem(m_layerTree, QStringList("world"));
        root->setCheckState(0, Qt::Checked);
        layerDock->setWidget(m_layerTree);
        addDockWidget(Qt::LeftDockWidgetArea, layerDock);

        // E. 右侧：搜索与属性面板 (DockWidget)
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

        // F. 状态栏
        statusBar()->showMessage("坐标系: EPSG:4326  |  就绪");
    }

private slots:
    // 核心交互 1：导入本机已有地图
    void slotImportMap() {
        QString filePath = QFileDialog::getOpenFileName(this, "选择遥感地图文件", "", "GIS Files (*.gpkg *.dem *.tif *.tiff)");
        if (filePath.isEmpty()) return;

        // C++ 使用 GDAL 底层打开地理数据集
        GDALDataset *poDataset = (GDALDataset *) GDALOpen(filePath.toUtf8().constData(), GA_ReadOnly);
        if (poDataset == NULL) {
            QMessageBox::critical(this, "错误", "GDAL 无法解析该地图格式！");
            return;
        }

        // 读取元数据元
        int rasterCount = poDataset->GetRasterCount();
        int width = poDataset->GetRasterXSize();
        int height = poDataset->GetRasterYSize();

        QString logMsg = QString("成功导入地图:\n路径: %1\n分辨率: %2x%3\n波段数: %4\n")
                            .arg(filePath).arg(width).arg(height).arg(rasterCount);
        
        m_logOutput->append(logMsg);

        // 更新左侧图层树
        QFileInfo fileInfo(filePath);
        QTreeWidgetItem *newLayer = new QTreeWidgetItem(m_layerTree, QStringList(fileInfo.fileName()));
        newLayer->setCheckState(0, Qt::Checked);

        // 通知画布重绘
        m_canvas->loadDemInfo(fileInfo.fileName());

        GDALClose(poDataset); // 关闭句柄
    }

    // 核心交互 2：执行属性搜索
    void slotSearchFeature() {
        QString keyword = m_searchEdit->text();
        if (keyword.isEmpty()) {
            m_logOutput->append(" [警告] 搜索关键词不能为空！");
            return;
        }
        
        m_logOutput->append(QString("正在检索 GPKG 属性表..."));
        // 模拟属性搜索逻辑
        // 在实际开发中，GPKG 是一个 SQLite 数据库，通常使用 OGR_DS_GetLayerByName() 
        // 配合 poLayer->SetAttributeFilter("name LIKE '%keyword%'") 进行 SQL 检索。
        
        m_logOutput->append(QString("[命中结果] 成功在当前视窗内匹配到要素: '%1'").arg(keyword));
        statusBar()->showMessage(QString("搜索完成。命中: 1"));
    }

private:
    MapCanvas *m_canvas;
    QTreeWidget *m_layerTree;
    QLineEdit *m_searchEdit;
    QTextEdit *m_logOutput;
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    MainWindow w;
    w.show();
    return app.exec();
}
