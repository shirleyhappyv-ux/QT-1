#include "mapcanvas.h"
#include <QPainter>
#include <QPainterPath>
#include <QDebug>
#include <algorithm>

// 如果你的构造函数里没有注册，加上这两行
MapCanvas::MapCanvas(QWidget *parent) : QWidget(parent), m_hasData(false), m_filePath("") {
    GDALAllRegister();
    OGRRegisterAll();
}

void MapCanvas::setHasData(bool hasData) {
    m_hasData = hasData;
    update();
}

void MapCanvas::setMapFilePath(const QString &path) {
    m_filePath = path;
    m_hasData = true;
    update();
}

void MapCanvas::setLockedFeature(const QString &featureName) {
    m_lockedFeature = featureName;
    update();
}

void MapCanvas::paintEvent(QPaintEvent *event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // 1. 无数据时兜底
    if (!m_hasData || m_filePath.isEmpty()) {
        painter.fillRect(rect(), QColor(20, 24, 35));
        painter.setPen(Qt::gray);
        painter.drawText(rect(), Qt::AlignCenter, "等待导入地图数据...");
        return;
    }

    // 2. 物理打开 GPKG
    GDALDataset *poDS = (GDALDataset *)GDALOpen(m_filePath.toLocal8Bit().constData(), GA_ReadOnly);
    if (!poDS) {
        painter.fillRect(rect(), QColor(40, 20, 20));
        painter.setPen(Qt::white);
        painter.drawText(rect(), Qt::AlignCenter, "GDAL 物理读取文件失败，请检查路径。");
        return;
    }

    int layerCount = poDS->GetLayerCount();
    int rasterCount = poDS->GetRasterCount();

    // 🌟 核心分流：优先看有没有矢量图层，如果没有再看有没有栅格图层
    if (layerCount > 0) {
        // ---------------- [ 矢量图层绘制逻辑 ] ----------------
        painter.fillRect(rect(), QColor(15, 18, 26)); // 深色底

        OGRLayer *poLayer = poDS->GetLayer(0);
        poLayer->ResetReading();

        OGREnvelope envelope;
        if (poLayer->GetExtent(&envelope) == OGRERR_NONE) {
            double mapWidth = envelope.MaxX - envelope.MinX;
            double mapHeight = envelope.MaxY - envelope.MinY;
            if (mapWidth <= 0) mapWidth = 1.0;
            if (mapHeight <= 0) mapHeight = 1.0;

            int margin = 40;
            double scaleX = (width() - 2 * margin) / mapWidth;
            double scaleY = (height() - 2 * margin) / mapHeight;
            double scale = std::min(scaleX, scaleY);

            painter.setPen(QPen(QColor(0, 255, 127), 1.5)); // 荧光绿线

            OGRFeature *poFeature;
            while ((poFeature = poLayer->GetNextFeature()) != nullptr) {
                OGRGeometry *poGeometry = poFeature->GetGeometryRef();
                if (poGeometry != nullptr) {
                    int gType = wkbFlatten(poGeometry->getGeometryType());
                    // 兼容线段 (LineString) 和多边形边界 (Polygon)
                    if (gType == wkbLineString || gType == wkbPolygon) {
                        // 这里为了简化演示，强制转成 LineString 读取点
                        OGRLineString *poLine = (OGRLineString *)poGeometry;
                        QPainterPath path;
                        bool first = true;

                        for (int i = 0; i < poLine->getNumPoints(); i++) {
                            int screenX = margin + static_cast<int>((poLine->getX(i) - envelope.MinX) * scale);
                            int screenY = height() - margin - static_cast<int>((poLine->getY(i) - envelope.MinY) * scale);

                            if (first) { path.moveTo(screenX, screenY); first = false; }
                            else { path.lineTo(screenX, screenY); }
                        }
                        painter.drawPath(path);
                    }
                }
                OGRFeature::DestroyFeature(poFeature);
            }
        }
    } 
    else if (rasterCount > 0) {
        // ---------------- [ 栅格图层绘制逻辑（DEM / 遥感图） ] ----------------
        GDALRasterBand *poBand = poDS->GetRasterBand(1);
        int nXSize = poBand->GetXSize();
        int nYSize = poBand->GetYSize();

        std::vector<float> buf(nXSize * nYSize);
        poBand->RasterIO(GF_Read, 0, 0, nXSize, nYSize, &buf[0], nXSize, nYSize, GDT_Float32, 0, 0);

        float minV = *std::min_element(buf.begin(), buf.end());
        float maxV = *std::max_element(buf.begin(), buf.end());
        if (maxV == minV) maxV += 1.0f;

        QImage img(nXSize, nYSize, QImage::Format_Grayscale8);
        for (int y = 0; y < nYSize; ++y) {
            uchar *line = img.scanLine(y);
            for (int x = 0; x < nXSize; ++x) {
                float v = buf[y * nXSize + x];
                int gray = static_cast<int>((v - minV) / (maxV - minV) * 255.0f);
                line[x] = static_cast<uchar>(std::clamp(gray, 0, 255));
            }
        }
        // 铺满画布
        painter.drawImage(rect(), img);
    } 
    else {
        // ---------------- [ 诊断模式 ] ----------------
        // 如果文件打开了，但既没有矢量也没有栅格，把文件真实的元数据打印在屏幕上
        painter.fillRect(rect(), QColor(30, 35, 45));
        painter.setPen(Qt::white);
        QString info = QString("诊断报告：\n文件已成功打开\n矢量图层数: %1\n栅格波段数: %2\n此文件可能为空白图集！")
                       .arg(layerCount).arg(rasterCount);
        painter.drawText(rect(), Qt::AlignCenter, info);
    }

    GDALClose(poDS);
}

