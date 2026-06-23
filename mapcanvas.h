#ifndef MAPCANVAS_H
#define MAPCANVAS_H

#include <QWidget>
#include <QString>
#include <QImage>
#include "gdal_priv.h"
#include "ogrsf_frmts.h"

class MapCanvas : public QWidget {
  
public:
    explicit MapCanvas(QWidget *parent = nullptr);
    
    // 💡 头文件里只声明，不要写 {} 实现，防止 redefinition 报错
    void setHasData(bool hasData); 
    void setMapFilePath(const QString &path);
    void setLockedFeature(const QString &featureName); // 🚀 搜索靶心接口

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    bool m_hasData;
    QString m_filePath;
    QString m_lockedFeature; // 🚀 恢复靶心锁定变量

    void drawVectorLayer(QPainter &painter, GDALDataset *poDS);
    void drawRasterLayer(QPainter &painter, GDALDataset *poDS);
};

#endif // MAPCANVAS_H
