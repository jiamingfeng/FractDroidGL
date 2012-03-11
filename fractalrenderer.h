#ifndef FRACTALRENDERER_H
#define FRACTALRENDERER_H

#include <QObject>

QT_BEGIN_NAMESPACE
    class MandelGLWidget;
    class QGLWidget;
    class QTimer;
QT_END_NAMESPACE

class FractalRenderer : public QObject
{
    Q_OBJECT
public:
    FractalRenderer(MandelGLWidget *parent = 0);
    ~FractalRenderer();
    
signals:
    void FinishedRendering();
    
public slots:
    bool StartRendering();

private:
    MandelGLWidget *glWidget;
    QGLWidget *sharedWidget;
    
};

#endif // FRACTALRENDERER_H
