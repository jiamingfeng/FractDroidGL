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
    // opengl classes
	class QGLShaderProgram;
    class QGLFramebufferObject;

    //gesture classes
    class QGestureEvent;
    class QPanGesture;
    class QPinchGesture;
    class QTapAndHoldGesture;
    class QTapGesture;
QT_END_NAMESPACE

class BufferSwapWorker;

class MandelGLWidget : public QGLWidget, protected QGLFunctions
{
	Q_OBJECT

public:

    enum MultiTouchGestures
    {
        NONE     = 0,
        PIN_ZOOM = 1,
        PIN_ROTATE   = 2
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

#if !defined (Q_OS_ANDROID)
    // mouse events
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
#endif
    // devices with keyboard only
    void keyPressEvent(QKeyEvent *event);
    void keyReleaseEvent(QKeyEvent * event);

    // handel the gesture events
    bool event(QEvent *event);
    bool gestureEvent(QGestureEvent *event);
    void handelPanGesture(QPanGesture *gesture);
    void handelPinchGesture(QPinchGesture *gesture);
    void handelTapAndHoldGesture(QTapAndHoldGesture *gesture);


private:
    // update center position of the mandelbrot
    void UpdateMandelbrotCenter(QPointF& pixelOffset);

    //TODO:: currently, the rotation pivot is set as mandelbrot center point by default
    //       need to get the rotation pivot by reading the gesture center point
    void UpdateRotationPivot();
    void UpdateProjectedScales();
    void DrawHUD();
    void ComputeHUDRect();


private:

    // shader objects
	QGLShaderProgram* mandelProgram;

    QGLShaderProgram* postEffectProgram;

    bool passCompiled;

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

    // attribute/uniform locations for mandelbrot shader (fast data updating)
    GLint mvpFractLoc;               //MVP
    GLint posFractLoc;               //Position
    GLint uvFractLoc;                //InTexCoord
    GLint scaleFractLoc;             //scale
    GLint resFractLoc;               //resolution

    GLint rotFractLoc;               //rotRadian
    GLint rotPivotFractLoc;          //rotatePivot
    GLint iterFractLoc;              //maxIterations
    GLint centerFractLoc;            //center
    GLint lookupTextureLoc;          //lookUpTexture

    // uniform locations for post process  shader (fast data updating)
    GLint mvpPostLoc;               //MVP
    GLint posPostLoc;               //Position
    GLint uvPostLoc;                //InTexCoord
    GLint scalePostLoc;             //scale
    GLint resPostLoc;               //resolution

    GLint texCoodOffsetLoc;         //translation
    GLint rotationOffsetLoc;        //rotation
    GLint rotationPivotLoc;         //rotationPivot
    GLint fboTextureLoc;            //fbo


    
    // mouse movement and touch events
    QPointF pixelOffset;
    QPointF lastDragPos;

    // center point of mandelbrot
    QVector2D centerPos;

    // scale factor of current rendering
    float scaleFactor;
    float previousScale;
    float currentScaleFactor;
    QPointF projectedScaleFactor;

    // rotation of current view
    float rotation;             //rotation in radian
    QVector2D rotationPivot;    //rotationPivot for mandelbrot shader
    QVector2D rotationPivotSS;  //screen-space rotation pivot (post effect shader)
    QPointF screenPivot;        //rotation pivot in screen coordinate

    float maxInterations;

    // image manipulate parameters for final image
    QVector2D textCoordOffset;
    float rotationOffset;
    float imageScale;

    // whether to render a new frame of mandelbrot
    bool renderMandelbrot;

    // current gesture
    MultiTouchGestures currentGesture;


    // statics HUD
    QString hudMessage;
    QString strFps;
    int messageLength;
    QRect hudRect;
    int frames;
    QTime fpsTime;
    QPainter* textPainter;
    bool isHUDDirty;
    bool showHUD;

};


#endif // MandelGLWidget_h__
