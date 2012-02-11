/*
 * Copyright (c) 2012 Eric Feng
 *
 * This file is part of 'FractDroidGL' - an mandelbrot set rendering app for Android
 *
 * FractDroidGL is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * FractDroidGL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MandelGLWidget_h__
#define MandelGLWidget_h__

#include <QMatrix4x4>
#include <QVector2D>
#include <QTime>
#include <QGLWidget>
#include <QGLFunctions>
#include <QThread>

QT_BEGIN_NAMESPACE
	class QGLShaderProgram;
    class QGLFramebufferObject;
QT_END_NAMESPACE

class BufferSwapWorker;

class MandelGLWidget : public QGLWidget, protected QGLFunctions
{
	Q_OBJECT

public:

    enum MultiTouchGestures
    {
        PIN_ZOOM,
        ROTATE
    };

	MandelGLWidget(QWidget* parentWindow = 0);
	~MandelGLWidget();

signals:
    // emit the swap buffer signal after all the gl calls
    void NeedSwapBuffer();

public slots:
    void startRendering();

protected:

    // override gl functions
	void initializeGL();
	void resizeGL(int width, int height);
    void paintGL();

#if !defined (QT_OPENGL_ES_2)
    // mouse events
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
#endif
    // devices with keyboard only
    void keyPressEvent(QKeyEvent *event);

    // handel the touch events
    bool event(QEvent *event);


private:
    // update center position of the mandelbrot
    void UpdateMandelbrotPos();
    void UpdateProjectedScales();
    void DrawHUD();
    void ComputeHUDRect(bool forceUpdate = false);


private:

    // shader objects
	QGLShaderProgram* mandelProgram;

    QGLShaderProgram* postEffectProgram;

    // frame buffer object
    QGLFramebufferObject* fbo;

    // shader errors
    QString shaderErrors;

	// texture object
	GLuint lookupTextureId;

    // render loop timer
    QTimer *renderLoopTimer;

    // model-view-projection matrix for mandelbrot rendering
    QMatrix4x4 modelViewProjection;

    // projection matrix for post effect rendering
    QMatrix4x4 projectMat;

    // attribute/uniform locations for mandelbrot vertex shader (fast data updating)
    GLint projUniformLoc;
    GLint posAttrLoc;
    GLint uvAttrLoc;
    GLint scaleUniformLoc;

    // uniform locations for mandelbrot fragment shader (fast data updating)
    GLint iterUniformLoc;
    GLint centerUniformLoc;
    GLint resolutionLoc;
    GLint lookupTextureLoc;

    // uniform locations for post process fragment shader (fast data updating)
    GLint texCoodOffsetLoc;
    GLint imagescaleUniformLoc;
    GLint fboTextureLoc;


    
    // mouse movement and touch events
    QPointF pixelOffset;
    QPointF lastDragPos;
    //QLineF originalLine;
    QPointF originalP0P1;
    QVector2D centerPos;
    float scaleFactor;
    float previousScale;
    QPointF projectedScaleFactor;
    float maxInterations;
    qreal rotateAngle;

    // image manipulate parameters for final image
    QVector2D textCoordOffset;
    float imageScale;

    MultiTouchGestures currentGesture;

    bool renderMandelbrot;


    // statics
    QString hudMessage;
    QString strFps;
    int messageLength;
    QRect hudRect;
    int frames;
    QTime fpsTime;
    QPainter* textPainter;

    // threads
    //QThread swapBufferThread;
    //BufferSwapWorker* swapBufferWorker;


};


#endif // MandelGLWidget_h__
