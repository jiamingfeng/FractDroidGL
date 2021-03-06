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


#include "MandelGLWidget.h"
#include <QtOpenGL/QtOpenGL>
#include <QtCore/qmath.h>
#include <QFutureWatcher>
#include <math.h>

#include "fractalrenderer.h"

// updates with highest framerate
//#define PERFORMANCE_TEST

//turn on and off HUD
#define SHOW_HUD

//turn on and off debug hud
//#define SHOW_DEBUG_HUD

// updates only when user interact with the mandelbrot set
//#define INTERACTIVE_UPDATES_ONLY


// pos, uv and indices array for post effect plane
const GLfloat quad_vertices[] = { -1.0f,-1.0f, 1.0f,-1.0f, 
                                   1.0f,1.0f, -1.0f,1.0f };
const GLfloat quad_uvs[]      = { 0.0f, 1.0f, 1.0f, 1.0f, 
                                  1.0f, 0.0f,  0.0f, 0.0f };
const GLushort quad_indices[] = {0,1,2, 2,3,0};



const float ZOOM_STEP = 1.2f;           // zoom step for pin zoom
const float INTERATION_STEP = 1.06f;    // interaction change step
const float INIT_ITERATION = 64.0f;     // init iteration count
const float LOG_BASE = 8.0f;
const float LN_LOG_BASE = float(qLn(LOG_BASE));
const float AUTO_INTERATION_FACTOR = INTERATION_STEP / ZOOM_STEP;
const float MAX_ONE_SHOT_ZOOM = 50.0f;

const float PIN_ROTATE_THRESHOLD = 0.05f;   // pin rotate threhold in degree

const float RADIAN_TO_DEGREE = 57.295779513082320876798154814105f; // 180 / PI
const float DEGREE_TO_RADIAN = 0.01745329251994329576923690768489f; // PI / 180

MandelGLWidget::MandelGLWidget(QWidget* parentWindow /* = 0 */)
    : QGLWidget(parentWindow)
{
    renderer = 0;

#ifdef USE_QT_CONCURRENT
    watcher = new QFutureWatcher<bool>(this);
#endif

    mandelProgram = 0;
    currentIndex    = 0;
    nextIndex       = 1;
    for(int i=0; i < MandelGLWidget::PING_PONG_COUNT; i++)
    {
        fbo[i] = 0;
    }
    lookupTextureId = 0;
    fboId = 0;
    modelViewProjection.setToIdentity();
    projectMat.setToIdentity();

    shaderErrors = "";
    passCompiled = false;

    mvpFractLoc     = 0;
    posFractLoc     = 0;
    uvFractLoc      = 0;
    scaleFractLoc   = 0;
    resFractLoc     = 0;

    rotFractLoc     = 0;
    rotPivotFractLoc= 0;
    iterFractLoc    = 0;
    centerFractLoc  = 0;
    lookupTextureLoc= 0;

    mvpPostLoc      = 0;
    posPostLoc      = 0;
    uvPostLoc       = 0;
    scalePostLoc    = 0;
    resPostLoc      = 0;

    texCoodOffsetLoc= 0;
    rotationOffsetLoc=0;
    fboTextureLoc   = 0;


    // default values for the shader
    centerPos = QVector2D(-0.5, 0.0);//QVector2D(-0.74635183346747f, 0.09853820615559f);
    scaleFactor = 0.8f;//1333333333.333333333f;
    previousScale = scaleFactor;
    maxInterations = INIT_ITERATION;

    // statistics
    frames = 0;
    textPainter = 0;
    messageLength = 0;
    isHUDDirty = true;
    showHUD = true;

    UpdateProjectedScales();

    // disable background filling
    setAutoFillBackground(false);
    setAttribute(Qt::WA_NoSystemBackground, true);
    setAttribute(Qt::WA_PaintOutsidePaintEvent, true);
    setAutoBufferSwap(false);

    rotation = 0.0f;
    rotationOffset = 0.0f;
    imageScale = 1.0f;
    renderMandelbrot = true;

#ifdef Q_OS_ANDROID
    // turn of touch events
    setAttribute(Qt::WA_AcceptTouchEvents);

    // enable gesture events
    //grabGesture(Qt::PanGesture);
    grabGesture(Qt::PinchGesture);
    //grabGesture(Qt::TapGesture);
    //grabGesture(Qt::TapAndHoldGesture);

    // change the smaller font in mobile devices
    QFont currentFont = font();
    currentFont.setPointSizeF(7.0);
    setFont(currentFont);
#endif

    currentGesture = MandelGLWidget::NONE;
}

MandelGLWidget::~MandelGLWidget()
{
    // timer
    renderLoopTimer->stop();
    delete renderLoopTimer;
    renderLoopTimer = 0;

#ifdef USE_QT_MULTI_THREAD
    if(renderThread.isRunning())
    {
        renderThread.quit();
    }
#endif

#ifdef USE_QT_CONCURRENT
    delete watcher;
    watcher = 0;
#endif

    delete renderer;
    renderer = 0;

    // shaders
    delete mandelProgram;
    mandelProgram = 0;

    delete postEffectProgram;
    postEffectProgram = 0;

    // textures
    glDeleteTextures(1, &lookupTextureId);


    // fbo
    for(int i=0; i < MandelGLWidget::PING_PONG_COUNT; i++)
    {
        delete fbo[i];
        fbo[i] = 0;
    }

    glDisable(GL_TEXTURE_2D);

    // painter
    delete textPainter;
    textPainter = 0;

}

void MandelGLWidget::initializeGL()
{
    renderer = new FractalRenderer(this);

    initializeGLFunctions();


    glEnable(GL_TEXTURE_2D);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    // load the texture from file
    QImage lookupImage(QString(":/FractDroidGL/Resources/lookup.png"));

    // init gl texture object
    glGenTextures(1, &lookupTextureId);
    glBindTexture(GL_TEXTURE_2D, lookupTextureId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, lookupImage.width(),
        lookupImage.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, lookupImage.bits());

    // setup all texture parameters
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    // bind back the default texture id
    glBindTexture(GL_TEXTURE_2D, 0);

    mandelProgram = new QGLShaderProgram(context());

    // add vertex and fragment shaders for mandelbrot program
    QGLShader mandelbrotVertexShader(QGLShader::Vertex, context());
    passCompiled = mandelbrotVertexShader.compileSourceFile(":/FractDroidGL/Resources/mandelbrot_vert.glsl");

    if( passCompiled == false )
    {
        shaderErrors = mandelbrotVertexShader.log();
    }

    QGLShader mandelbrotFragShader(QGLShader::Fragment, context());

    // try to load the original shader
    // if the compilation failed, try to fall back plan
    QFile fragShaderFile(":/FractDroidGL/Resources/mandelbrot_frag.glsl");

    if (!fragShaderFile.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    QTextStream in(&fragShaderFile);

    // read the whole shader in one shot, it is a small text file anyway
    QString fragShaderContent = in.readAll();
    fragShaderFile.close();
    passCompiled = mandelbrotFragShader.compileSourceCode(fragShaderContent);

    if( passCompiled == false)
    {
        shaderErrors += mandelbrotFragShader.log();
    }

    // use the fallback shader instead
    if( shaderErrors.isEmpty() == false)
    {
        QString removeString("//#define FALL_BACK");
        // locate the comment, uncomment it by remove the "//"
        fragShaderContent.remove(fragShaderContent.indexOf(removeString), 2);
        passCompiled = mandelbrotFragShader.compileSourceCode(fragShaderContent);
        shaderErrors = "";
    }

    mandelProgram->addShader(&mandelbrotVertexShader);
    mandelProgram->addShader(&mandelbrotFragShader);

    mandelProgram->link();
    mandelProgram->bind();

    // Get the attribute/uniform locations from shaders
    mvpFractLoc = mandelProgram->uniformLocation("MVP");
    posFractLoc  = mandelProgram->attributeLocation("Position");
    uvFractLoc   = mandelProgram->attributeLocation("InTexCoord");
    scaleFractLoc = mandelProgram->uniformLocation("scale");
    resFractLoc = mandelProgram->uniformLocation("whScale");
    rotFractLoc = mandelProgram->uniformLocation("rotRadian");
    rotPivotFractLoc = mandelProgram->uniformLocation("rotatePivot");
    iterFractLoc = mandelProgram->uniformLocation("maxIterations");
    centerFractLoc = mandelProgram->uniformLocation("center");
    lookupTextureLoc = mandelProgram->uniformLocation("lookUpTexture");

    //set up the post effect shader program to do the final rendering
    postEffectProgram = new QGLShaderProgram(context());

    // add vertex and fragment shaders for post effect program
    QGLShader vertexShader(QGLShader::Vertex, context());
    passCompiled = vertexShader.compileSourceFile(":/FractDroidGL/Resources/vert.glsl");

    if( passCompiled == false)
    {
        shaderErrors = vertexShader.log();
    }

    QGLShader fragShader(QGLShader::Fragment, context());
    passCompiled = fragShader.compileSourceFile(":/FractDroidGL/Resources/frag.glsl");

    if( passCompiled == false)
    {
        shaderErrors += fragShader.log();
    }

    if( shaderErrors.isEmpty() == false)
    {
        QString title = "Please report the following errors to developer, thank you";
        QMessageBox msgBox(this);
        msgBox.setWindowTitle(title);
        msgBox.setText(shaderErrors);
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.exec();
        this->close();
    }

    postEffectProgram->addShader(&vertexShader);
    postEffectProgram->addShader(&fragShader);

    postEffectProgram->link();
    postEffectProgram->bind();

    mvpPostLoc = postEffectProgram->uniformLocation("MVP");
    posPostLoc  = postEffectProgram->attributeLocation("Position");
    uvPostLoc   = postEffectProgram->attributeLocation("InTexCoord");
    scalePostLoc = postEffectProgram->uniformLocation("scale");
    resPostLoc = postEffectProgram->uniformLocation("whScale");

    texCoodOffsetLoc = postEffectProgram->uniformLocation("translation");
    rotationOffsetLoc = postEffectProgram->uniformLocation("rotation");
    rotationPivotLoc = postEffectProgram->uniformLocation("rotationPivot");
    fboTextureLoc = postEffectProgram->uniformLocation("fboTexture");

    //postEffectProgram->enableAttributeArray(posAttrLoc);
    //postEffectProgram->enableAttributeArray(uvAttrLoc);

    //init fbo
    for(int i=0; i < MandelGLWidget::PING_PONG_COUNT; i++)
    {
        fbo[i] = new QGLFramebufferObject(width(), height());
    }

    // Clear the background with black color
    glClearColor(0, 0, 0, 1.0f);

    textPainter = new QPainter;

#if !defined ( INTERACTIVE_UPDATES_ONLY )
    // rendering loop timer set as 30 fps
    renderLoopTimer = new QTimer(this);
    QObject::connect(renderLoopTimer, SIGNAL(timeout()), this, SLOT(updateGL()), Qt::DirectConnection);
    //QObject::connect(timer, SIGNAL(timeout()), this, SLOT(update()), Qt::DirectConnection);
#if !defined ( PERFORMANCE_TEST )
    renderLoopTimer->setInterval(16);
#endif
    renderLoopTimer->start();
#endif

#if defined ( USE_QT_CONCURRENT )
    connect(watcher, SIGNAL(finished()), this, SLOT(updateRenderFBO()));
#else
    connect(renderer, SIGNAL(FinishedRendering()),
        this, SLOT(updateRenderFBO()));
    connect(this, SIGNAL(StartFractalRendering()),
        renderer, SLOT(StartRendering()));
#if defined (USE_QT_MULTI_THREAD)
    renderer->moveToThread(&renderThread);
#endif  //USE_QT_MULTI_THREAD
#endif  //USE_QT_CONCURRENT
    
    //StopInteraction();

}

void MandelGLWidget::resizeGL(int width, int height)
{

    // setup viewport
    glViewport(0, 0, (GLint)width, (GLint)height);

    // update model-view-projection matrix
    //
    // we only need to calculate the projection matrix
    // because the model and view matrix are identity
    // e.g.
    // QMatrix4x4 view;
    // view.translate(0.0f, 0.0f, 0.0f);
    // QMatrix4x4 model;
    // model.setToIdentity();
    // modelViewProjection = modelViewProjection * view * model;

    whScale = float(width) / float(height);


    // reset the projection matrix
    modelViewProjection.ortho(QRectF(0.0f, float(width), float(height), 0.0f));

    projectMat.ortho(QRectF(0.0f, float(width), float(height), 0.0f));

    // update projection scale
    UpdateProjectedScales();

    // nuke the framebuffer if size changes
    for(int i=0; i < MandelGLWidget::PING_PONG_COUNT; i++)
    {
        if (fbo[i]->width() != width || fbo[i]->height() != height)
        {
            delete fbo[i];
            fbo[i] = new QGLFramebufferObject(width, height);
        }
    }

    // re-compute the rect for HUD after resizing
    isHUDDirty = true;

    StopInteraction();

}

void MandelGLWidget::paintGL()
{
    makeCurrent();

    //first time init
    if(fboId == 0)
        StopInteraction();

    glDisable(GL_CULL_FACE);

    glClear(GL_COLOR_BUFFER_BIT);

    //render the post effect
    postEffectProgram->bind();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, fboId);

    // Set vertex array to the shader
    postEffectProgram->enableAttributeArray(posPostLoc);
    postEffectProgram->setAttributeArray(posPostLoc, quad_vertices, 2 );

    postEffectProgram->enableAttributeArray(uvPostLoc);
    postEffectProgram->setAttributeArray(uvPostLoc, quad_uvs, 2 );

    //vertex shader uniforms
    postEffectProgram->setUniformValue( mvpPostLoc, projectMat );
    postEffectProgram->setUniformValue( scalePostLoc, imageScale );
    postEffectProgram->setUniformValue( resPostLoc,  whScale);

    postEffectProgram->setUniformValue( texCoodOffsetLoc, textCoordOffset);
    postEffectProgram->setUniformValue( rotationOffsetLoc, rotationOffset);
    postEffectProgram->setUniformValue( rotationPivotLoc, rotationPivotSS);
    postEffectProgram->setUniformValue( fboTextureLoc, 0);

    // draw the quad
    glDrawElements(GL_TRIANGLES, 2*3, GL_UNSIGNED_SHORT, quad_indices );

    glBindTexture(GL_TEXTURE_2D, 0);

    postEffectProgram->disableAttributeArray(uvPostLoc);
    postEffectProgram->disableAttributeArray(posPostLoc);

    //render the mandelbrot image ends
    postEffectProgram->release();

    if ( showHUD )
    {

        // build the HUG message

        hudMessage = "Fps: ";

        // update the fps message every 20 frames
        if (!(frames % 20))
        {
            // calculate fps
            strFps.setNum(frames * 1000.0 / fpsTime.elapsed(), 'f', 2);
        }
        hudMessage += strFps;

        QString tempStr;

        //shader paremeters

        hudMessage += "\nZoom: ";
        tempStr.setNum(scaleFactor, 'f', 2);
        hudMessage += tempStr;

        hudMessage += "\nRotation: ";
        tempStr.setNum(rotation * RADIAN_TO_DEGREE, 'f', 1);
        hudMessage += tempStr;

        hudMessage += "\nIterations: ";
        tempStr.setNum(int(maxInterations));
        hudMessage += tempStr;

#ifdef SHOW_DEBUG_HUD
        hudMessage += "\nCenter position: ";
        tempStr.setNum(centerPos.x(), 'f', 8);
        hudMessage += "\n";
        hudMessage += tempStr;
        tempStr.setNum(centerPos.y(), 'f', 8);
        hudMessage += "\n";
        hudMessage += tempStr;

#endif

        // reset the time every 100 frames
        if (!(frames % 100))
        {
            fpsTime.start();
            frames = 0;
        }
        frames ++;

        //draw fps

        textPainter->begin(this);
        DrawHUD();
        textPainter->end();
    }


    swapBuffers();
    //emit NeedSwapBuffer();

    doneCurrent();
}

void MandelGLWidget::DrawHUD()
{
    ComputeHUDRect();
    textPainter->setRenderHint(QPainter::TextAntialiasing);
    textPainter->setPen(Qt::white);
    textPainter->fillRect(hudRect, QColor(0, 0, 0, 127));
    textPainter->drawText(hudRect, Qt::AlignLeft, hudMessage);
}

void MandelGLWidget::ComputeHUDRect()
{
    if( messageLength == 0 || messageLength != hudMessage.length() || isHUDDirty)
    {

        QFontMetrics metrics(font());
        int border = qMax(4, metrics.leading());

        QRect rect = metrics.boundingRect(0, 0, width() - border, height(),
                                          Qt::AlignLeft, hudMessage);
        hudRect = QRect((width() - rect.width() - border), border * 2,
                     rect.width(), rect.height());

        messageLength = hudMessage.length();

        isHUDDirty = false;
    }
}

// events
#if !defined (Q_OS_ANDROID)
void MandelGLWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        lastDragPos = event->pos();

        //stop the mandelbrot rendering
        StartInteraction();

    }
}

void MandelGLWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() == Qt::LeftButton) 
    {
        pixelOffset = event->pos() - lastDragPos;
        lastDragPos = event->pos();

        //update shader position
        UpdateMandelbrotCenter(pixelOffset);
    }
}

void MandelGLWidget::mouseReleaseEvent(QMouseEvent *event)
{
    Q_UNUSED(event);

    // resume the rendering after mouse released 
    StopInteraction();
}

#endif

void MandelGLWidget::keyPressEvent(QKeyEvent *event)
{

    switch (event->key())
    {

    // zoom in
    case Qt::Key_Z:
        scaleFactor *= ZOOM_STEP;
        // interation = log10(scaleFactor) * INIT_ITERATION when scaleFactor > 10
        maxInterations = (scaleFactor < LOG_BASE ? 1.0f : (float(qLn(scaleFactor)) / LN_LOG_BASE)) * INIT_ITERATION;  // *= INTERATION_STEP;

        StartInteraction();
        break;
    // zoom out
    case Qt::Key_X:
        scaleFactor /= ZOOM_STEP;
        maxInterations  = (scaleFactor < LOG_BASE ? 1.0f : (float(qLn(scaleFactor)) / LN_LOG_BASE)) * INIT_ITERATION; //  /= INTERATION_STEP;

        StartInteraction();
        break;

    // rotate 5 degree counter clockwise
    case Qt::Key_Up:
        rotation += 5.0f * DEGREE_TO_RADIAN;
        rotationOffset += 5.0f * DEGREE_TO_RADIAN;
        UpdateRotationPivot();
        
        StartInteraction();
        break;

    // rotate 5 degree clockwise
    case Qt::Key_Down:
        rotation -= 5.0f * DEGREE_TO_RADIAN;
        rotationOffset -= 5.0f * DEGREE_TO_RADIAN;
        UpdateRotationPivot();
        
        StartInteraction();
        break;

    case Qt::Key_Escape:
        this->close();
        break;
    default:
        QGLWidget::keyPressEvent(event);
    }
}

void MandelGLWidget::keyReleaseEvent(QKeyEvent *event)
{

    switch (event->key())
    {
    // zoom in
    case Qt::Key_Z:
    // zoom out
    case Qt::Key_X:
    // rotate 5 degree counter clockwise
    case Qt::Key_Up:
    // rotate 5 degree clockwise
    case Qt::Key_Down:
        
        StopInteraction();
        break;
    default:
        QGLWidget::keyReleaseEvent(event);
    }
}

bool MandelGLWidget::event(QEvent *event)
{
    if (event->type() == QEvent::Gesture)
        return gestureEvent(static_cast<QGestureEvent*>(event));

    if (event->type() == QEvent::TouchBegin  ||
        event->type() == QEvent::TouchUpdate ||
        event->type() == QEvent::TouchEnd    )
    {
        QTouchEvent *touchEvent = static_cast<QTouchEvent *>(event);
        QList<QTouchEvent::TouchPoint> touchPoints = touchEvent->touchPoints();
        const QTouchEvent::TouchPoint &touchPoint0 = touchPoints.first();

        // one touch points are for moving around the center point
        if (touchPoints.count() == 1)
        {
            if ( touchEvent->touchPointStates() & Qt::TouchPointPressed )
            {
                lastDragPos = touchPoint0.startPos();

                // stop the mandelbrot rendering
                StartInteraction();

            }
            else if ( touchEvent->touchPointStates() & Qt::TouchPointMoved )
            {

                pixelOffset = touchPoint0.pos() - lastDragPos;
                lastDragPos = touchPoint0.pos();

                UpdateMandelbrotCenter(pixelOffset);
            }
            else if ( touchEvent->touchPointStates() & Qt::TouchPointReleased )
            {
                // resume the rendering after mouse released
                StopInteraction();
            }

            return true;
        }


    }

    return QGLWidget::event(event);
}

bool MandelGLWidget::gestureEvent(QGestureEvent *event)
{
    //disable pan gesture for now since it only support two touch points pan

//    if (QGesture *pan = event->gesture(Qt::PanGesture))
//        handelPanGesture(static_cast<QPanGesture *>(pan));
    if (QGesture *hold = event->gesture(Qt::TapAndHoldGesture))
        handelTapAndHoldGesture(static_cast<QTapAndHoldGesture *>(hold));

    if (QGesture *pinch = event->gesture(Qt::PinchGesture))
        handelPinchGesture(static_cast<QPinchGesture *>(pinch));

    return true;
}

void MandelGLWidget::handelPanGesture(QPanGesture *gesture)
{

    switch (gesture->state())
    {
    case Qt::GestureStarted:
        //stop the mandelbrot rendering
        StartInteraction();
        break;
    case Qt::GestureUpdated:
    {
        //stop the mandelbrot rendering
        StartInteraction();
#ifndef QT_NO_CURSOR
        setCursor(Qt::SizeAllCursor);
#endif

        QPointF pixelOffset = gesture->delta();

        UpdateMandelbrotCenter(pixelOffset);
    }
        break;

    case Qt::GestureFinished:
    case Qt::GestureCanceled:
        //resume the mandelbrot rendering
        StopInteraction();
        break;
    default:
#ifndef QT_NO_CURSOR
        setCursor(Qt::ArrowCursor);
#endif
    }
}


void MandelGLWidget::handelPinchGesture(QPinchGesture *gesture)
{
    currentScaleFactor = 1.0f;

    switch (gesture->state())
    {
    case Qt::GestureStarted:
        screenPivot = gesture->centerPoint();

        //stop the mandelbrot rendering
        StartInteraction();
        break;
    case Qt::GestureUpdated:
    {
        //stop the mandelbrot rendering
        StartInteraction();

        QPinchGesture::ChangeFlags changeFlags = gesture->changeFlags();

        // TODO:: fix the rotation bug
        if ( changeFlags & QPinchGesture::RotationAngleChanged)
        {
            qreal value = gesture->rotationAngle();
            qreal lastValue = gesture->lastRotationAngle();
            qreal deltaAngle = (value - lastValue) * DEGREE_TO_RADIAN;

            if ( qAbs(deltaAngle) > PIN_ROTATE_THRESHOLD  )
            {
                // set current gesture as rotate
                currentGesture = MandelGLWidget::PIN_ROTATE;
            }

            // only update the rotation if it is rotate mode
            if ( currentGesture == MandelGLWidget::PIN_ROTATE)
            {
                rotation        += deltaAngle;
                rotationOffset  += deltaAngle;

                UpdateRotationPivot();
            }


        }

        // zoom only if we are not in rotate mode
        if ( currentGesture != MandelGLWidget::PIN_ROTATE && changeFlags & QPinchGesture::ScaleFactorChanged)
        {
            currentScaleFactor = gesture->scaleFactor();

            scaleFactor *= currentScaleFactor;
            imageScale *= currentScaleFactor;

            currentScaleFactor = 1.0f;

            // update the iteration according to the zoom level

            float scaleLevel = scaleFactor / previousScale;
            if( scaleLevel > ZOOM_STEP || scaleLevel  < 1.0f / ZOOM_STEP )
            {
                maxInterations = (scaleFactor < LOG_BASE ? 1.0f : (float(qLn(scaleFactor)) / LN_LOG_BASE)) * INIT_ITERATION;
                previousScale = scaleFactor;
            }

            // update the zoom pivot
            QPointF pivotOffset;
            if ( changeFlags & QPinchGesture::CenterPointChanged )
            {
                pivotOffset = gesture->centerPoint() - gesture->lastCenterPoint();
            }
            //newPivot = p0 + (p1 - p0 ) t  [t | t > 0 && t < 1]
            //pivotOffset = (QPointF(width()/2.0f, height()/2.0f) - screenPivot) * scaleLevel / MAX_ONE_SHOT_ZOOM;

            UpdateMandelbrotCenter(pivotOffset);

            currentGesture = MandelGLWidget::PIN_ZOOM;
        }

    }
        break;

    case Qt::GestureFinished:
    case Qt::GestureCanceled:
        //resume the mandelbrot rendering
        StopInteraction();

        //reset the current gesture to none
        currentGesture = MandelGLWidget::NONE;
        break;

    default:
        break;
    }

}

void MandelGLWidget::handelTapAndHoldGesture(QTapAndHoldGesture *gesture)
{
    Q_UNUSED(gesture);
    // change the visibility of HUD display
    showHUD = !showHUD;
}

void MandelGLWidget::UpdateMandelbrotCenter(QPointF& pixelOffset)
{

    //rotate the movement offset according to current rotation in mobile platform
    QPointF rotatedOffset( pixelOffset.x() * cos(-rotation) - pixelOffset.y() * sin(-rotation),
                           pixelOffset.y() * cos(-rotation) + pixelOffset.x() * sin(-rotation));

    // remap the pixel offset from [0, width][0, height] to [-2, 1][-1, 1]
    centerPos.setX(centerPos.x() + rotatedOffset.x() * projectedScaleFactor.x() / scaleFactor);
    centerPos.setY(centerPos.y() + rotatedOffset.y() * projectedScaleFactor.y() / scaleFactor);

    // update rotation pivot
    UpdateRotationPivot();

    //pan the current texture
    textCoordOffset.setX(textCoordOffset.x() + pixelOffset.x() / float(width()) );
    textCoordOffset.setY(textCoordOffset.y() + pixelOffset.y() / float(height()) );

}


void MandelGLWidget::UpdateRotationPivot()
{
//    float fWidth = float(width());
//    float fHeight = float(height());
//    float stScale = fWidth / fHeight;
//    rotationPivot.setX( stScale * (4.0f * screenPivot.x() / fWidth - 2.0f) + centerPos.x()); // 2* (x - w/2) / w/2 + c.x
//    rotationPivot.setY(2.0f - 4.0 * screenPivot.y() / fHeight + centerPos.y());              // 2* ((h - y) - h/2) / (h/2) + c.y

//    rotationPivotSS.setX(screenPivot.x()/fWidth);
//    rotationPivotSS.setY(screenPivot.y()/fHeight);
    rotationPivot = centerPos;
    rotationPivotSS.setX(0.5f);
    rotationPivotSS.setY(0.5f);
}

void MandelGLWidget::UpdateProjectedScales()
{
    float newScaleFactor = 4.0f / float(height());
    projectedScaleFactor.setX ( -newScaleFactor);
    projectedScaleFactor.setY ( newScaleFactor);
}

void MandelGLWidget::RenderFractal()
{
    BindFBO();

    glDisable(GL_CULL_FACE);
    glClear(GL_COLOR_BUFFER_BIT);// | GL_DEPTH_BUFFER_BIT);

    //render the mandelbrot image beigns
    mandelProgram->bind();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, lookupTextureId);


    // Set vertexarray to the shader
    mandelProgram->enableAttributeArray(posFractLoc);
    mandelProgram->setAttributeArray(posFractLoc, quad_vertices, 2 );

    mandelProgram->enableAttributeArray(uvFractLoc);
    mandelProgram->setAttributeArray(uvFractLoc, quad_uvs, 2 );

    //shader's parameters
    mandelProgram->setUniformValue(mvpFractLoc, modelViewProjection );
    mandelProgram->setUniformValue(scaleFractLoc, scaleFactor);
    mandelProgram->setUniformValue(resFractLoc, whScale);
    mandelProgram->setUniformValue(rotFractLoc, rotation);
    mandelProgram->setUniformValue(rotPivotFractLoc, rotationPivot);
    mandelProgram->setUniformValue(iterFractLoc, int(maxInterations + 0.5f));
    mandelProgram->setUniformValue(centerFractLoc, centerPos);
    mandelProgram->setUniformValue(lookupTextureLoc, 0);

    // draw the quad
    glDrawElements(GL_TRIANGLES, 2*3, GL_UNSIGNED_SHORT, quad_indices );

    glBindTexture(GL_TEXTURE_2D, 0);

    mandelProgram->disableAttributeArray(uvFractLoc);
    mandelProgram->disableAttributeArray(posFractLoc);

    //render the mandelbrot image ends
    mandelProgram->release();

    ReleaseFBO();
}

void MandelGLWidget::StartInteraction()
{
#if  defined ( USE_QT_CONCURRENT )

    if(watcher->isRunning())
    {
        watcher->waitForFinished();
    }

#elif defined ( USE_QT_MULTI_THREAD )

    if(renderThread.isRunning())
    {
        renderThread.quit();
    }
#endif
}

void MandelGLWidget::StopInteraction()
{
#if defined ( USE_QT_CONCURRENT )
    QFuture<bool> future = QtConcurrent::run(renderer, &FractalRenderer::StartRendering);
    watcher->setFuture(future);
#else
#if defined ( USE_QT_MULTI_THREAD )
    if(!renderThread.isRunning())
    {
        renderThread.start();
    }
#endif  //USE_QT_MULTI_THREAD
    emit StartFractalRendering();
#endif  //USE_QT_CONCURRENT

}

void MandelGLWidget::BindFBO()
{
    fbo[currentIndex]->bind();
}

void MandelGLWidget::ReleaseFBO()
{
    fbo[currentIndex]->release();

}

void MandelGLWidget::updateRenderFBO()
{
    currentIndex = (currentIndex + 1) % PING_PONG_COUNT;
    nextIndex = (nextIndex + 1) % PING_PONG_COUNT;
    fboId = fbo[nextIndex]->texture();

    // zero the temp offset after we update the actual computing result
    textCoordOffset.setX(0);
    textCoordOffset.setY(0);
    rotationOffset = 0.0f;
    rotationPivotSS.setX(0.5);
    rotationPivotSS.setY(0.5);
    imageScale = 1.0f;
}
