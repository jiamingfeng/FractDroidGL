#include "fractalrenderer.h"
#include <QtOpenGL/QtOpenGL>
#include "MandelGLWidget.h"

FractalRenderer::FractalRenderer(MandelGLWidget *parent) :
    QObject()
{
    glWidget = parent;

    //create a shared context glwidget
    sharedWidget = new QGLWidget(0, parent);
    int width = glWidget->width();
    int height = glWidget->height();
    sharedWidget->resize(width, height);    
    
}

FractalRenderer::~FractalRenderer()
{
    delete sharedWidget;
    sharedWidget = 0;
}

bool FractalRenderer::StartRendering()
{
    sharedWidget->makeCurrent();

    glViewport(0, 0, glWidget->width(), glWidget->height());

    glWidget->RenderFractal();

    sharedWidget->doneCurrent();

    emit FinishedRendering();

    return true;
}
