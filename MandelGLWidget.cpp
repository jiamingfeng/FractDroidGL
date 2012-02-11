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

//#include "bufferswapworker.h"

// updates with highest framerate
#define PERFORMANCE_TEST

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

const float PIN_ZOOM_THRESHOLD = 0.8f;  // pin zoom threhold dot angle

const float MULTI_TOUCH_ROTATE = 0.2f;  // multi touch rotate threhold dot angle

const float RADIAN_TO_DEGREE = 57.2957795f; // 180 / PI

MandelGLWidget::MandelGLWidget(QWidget* parentWindow /* = 0 */)
    : QGLWidget(parentWindow)
{
    mandelProgram = 0;
    fbo           = 0;
    lookupTextureId = 0;    
    modelViewProjection.setToIdentity();
    projectMat.setToIdentity();

    shaderErrors = "";

    // attribute/uniform locations for vertex shader
    projUniformLoc = 0;
    posAttrLoc = 0;
    uvAttrLoc = 0;
    scaleUniformLoc = 0;

    // uniform locations for fragment shader (fast data updating)
    iterUniformLoc = 0;
    centerUniformLoc = 0;
    resolutionLoc = 0;

    // default values for the shader
    centerPos = QVector2D(-0.5f, 0.0f);
    scaleFactor = 1.0f;
    previousScale = 0.9f;
    maxInterations = 64.0f;

    // statistics
    frames = 0;
    textPainter = 0;
    messageLength = 0;

    UpdateProjectedScales();

    // turn of touch events
    setAttribute(Qt::WA_AcceptTouchEvents);

    // disable background filling
    setAutoFillBackground(false);
    setAttribute(Qt::WA_NoSystemBackground, true);
    //setAttribute(Qt::WA_PaintOnScreen, true);
    setAutoBufferSwap(false);

    rotateAngle = 0.0;
    imageScale = 1.0f;
    currentGesture = PIN_ZOOM;
    renderMandelbrot = true;

    // threads
    //swapBufferWorker = new BufferSwapWorker(this);

    //swapBufferWorker->moveToThread(&swapBufferThread);
    //connect(this, SIGNAL(NeedSwapBuffer()), swapBufferWorker, SLOT(SwapBuffer()), Qt::AutoConnection);
    //connect(swapBufferWorker, SIGNAL(DoneSwaping()), this, SLOT(startRendering()), Qt::AutoConnection);
}

MandelGLWidget::~MandelGLWidget()
{
    // timer
    renderLoopTimer->stop();
    delete renderLoopTimer;
    renderLoopTimer = 0;

    // shaders
    delete mandelProgram;
    mandelProgram = 0;

    delete postEffectProgram;
    postEffectProgram = 0;

    // textures
    glDeleteTextures(1, &lookupTextureId);


    // fbo
    delete fbo;
    fbo = 0;

    glDisable(GL_TEXTURE_2D);

    // painter
    delete textPainter;
    textPainter = 0;

    // threads
//    if ( swapBufferThread.isRunning())
//    {
//        swapBufferThread.quit();
//    }

//    delete swapBufferWorker;
}

void MandelGLWidget::initializeGL()
{    

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
    bool isCompiled = mandelbrotVertexShader.compileSourceFile(":/FractDroidGL/Resources/mandelbrot_vert.glsl");

    if( isCompiled == false)
    {
        shaderErrors = mandelbrotVertexShader.log();
    }

    QGLShader mandelbrotFragShader(QGLShader::Fragment, context());
    isCompiled = mandelbrotFragShader.compileSourceFile(":/FractDroidGL/Resources/mandelbrot_frag.glsl");

    if( isCompiled == false)
    {
        shaderErrors += mandelbrotFragShader.log();
    }

    if( shaderErrors.isEmpty() == false)
    {
        QString title = "Shader compile errors";
        QMessageBox msgBox(this);
        msgBox.setWindowTitle(title);
        msgBox.setText(shaderErrors);
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.exec();

        this->close();
    }

    mandelProgram->addShader(&mandelbrotVertexShader);
    mandelProgram->addShader(&mandelbrotFragShader);

    mandelProgram->link();
    mandelProgram->bind();

    // Get the attribute/uniform locations from vertex shader
    projUniformLoc = mandelProgram->uniformLocation("MVP");
    posAttrLoc  = mandelProgram->attributeLocation("Position");
    uvAttrLoc   = mandelProgram->attributeLocation("InTexCoord");



    // Set vertexarray to the shader
    mandelProgram->enableAttributeArray(posAttrLoc);
    //mandelProgram->setAttributeArray(posAttrLoc, quad_vertices, 2 );

    mandelProgram->enableAttributeArray(uvAttrLoc);
    //mandelProgram->setAttributeArray(uvAttrLoc, quad_uvs, 2 );

    // Get the uniform locations from fragment shader
    iterUniformLoc = mandelProgram->uniformLocation("maxIterations");
    scaleUniformLoc = mandelProgram->uniformLocation("scale");
    centerUniformLoc = mandelProgram->uniformLocation("center");
    resolutionLoc = mandelProgram->uniformLocation("resolution");
    lookupTextureLoc = mandelProgram->uniformLocation("lookUpTexture");

    //set up the post effect shader program to do the final rendering
    postEffectProgram = new QGLShaderProgram(context());

    // add vertex and fragment shaders for post effect program
    QGLShader vertexShader(QGLShader::Vertex, context());
    isCompiled = vertexShader.compileSourceFile(":/FractDroidGL/Resources/vert.glsl");

    if( isCompiled == false)
    {
        shaderErrors = vertexShader.log();
    }

    QGLShader fragShader(QGLShader::Fragment, context());
    isCompiled = fragShader.compileSourceFile(":/FractDroidGL/Resources/frag.glsl");

    if( isCompiled == false)
    {
        shaderErrors += fragShader.log();
    }

    if( shaderErrors.isEmpty() == false)
    {
        QString title = "Shader compile errors";
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

    texCoodOffsetLoc = postEffectProgram->uniformLocation("TexCoordoffset");
    imagescaleUniformLoc = postEffectProgram->uniformLocation("scale");
    fboTextureLoc = postEffectProgram->uniformLocation("fboTexture");

    postEffectProgram->enableAttributeArray(posAttrLoc);
    postEffectProgram->enableAttributeArray(uvAttrLoc);

    //init fbo
    fbo = new QGLFramebufferObject(width(), height());

    // Clear the background with dark-green
    //glClearColor(0.1f, 0.4f, 0.1f, 1.0f);
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

    //swapBufferThread.start();

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


    // reset the projection matrix
    modelViewProjection.ortho(QRectF(0.0f, float(width), float(height), 0.0f));

    projectMat.ortho(QRectF(0.0f, float(width), float(height), 0.0f));

    // update projection scale
    UpdateProjectedScales();

    // nuke the framebuffer
    delete fbo;
    fbo = new QGLFramebufferObject(width, height);

    // re-compute the rect for HUD after resizing
    ComputeHUDRect(true);

    renderMandelbrot = true;

}

void MandelGLWidget::paintGL()
{
    if(renderMandelbrot)
    {
        fbo->bind();

        glClear(GL_COLOR_BUFFER_BIT);// | GL_DEPTH_BUFFER_BIT);

        //render the mandelbrot image beigns
        mandelProgram->bind();

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, lookupTextureId);

        // Set vertexarray to the shader
        mandelProgram->enableAttributeArray(posAttrLoc);
        mandelProgram->setAttributeArray(posAttrLoc, quad_vertices, 2 );

        mandelProgram->enableAttributeArray(uvAttrLoc);
        mandelProgram->setAttributeArray(uvAttrLoc, quad_uvs, 2 );

        // fragment shader uniforms
        mandelProgram->setUniformValue(iterUniformLoc, maxInterations);
        mandelProgram->setUniformValue(scaleUniformLoc, scaleFactor);
        mandelProgram->setUniformValue(centerUniformLoc, centerPos);
        mandelProgram->setUniformValue(resolutionLoc,
            QVector2D(float(this->width()), float(this->height())));
        mandelProgram->setUniformValue(lookupTextureLoc, 0);

        //vertex shader uniforms
        mandelProgram->setUniformValue( projUniformLoc, modelViewProjection );

        // draw the quad
        glDrawElements(GL_TRIANGLES, 2*3, GL_UNSIGNED_SHORT, quad_indices );

        //glBindTexture(GL_TEXTURE_2D, 0);

        //mandelProgram->disableAttributeArray(uvAttrLoc);
        //mandelProgram->disableAttributeArray(posAttrLoc);

        //render the mandelbrot image ends
        mandelProgram->release();

        fbo->release();

        renderMandelbrot = false;
    }

    glClear(GL_COLOR_BUFFER_BIT);

    //render the post effect
    postEffectProgram->bind();

    //glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, fbo->texture());

    // Set vertexarray to the shader
    postEffectProgram->enableAttributeArray(posAttrLoc);
    postEffectProgram->setAttributeArray(posAttrLoc, quad_vertices, 2 );

    postEffectProgram->enableAttributeArray(uvAttrLoc);
    postEffectProgram->setAttributeArray(uvAttrLoc, quad_uvs, 2 );

    //vertex shader uniforms
    postEffectProgram->setUniformValue( projUniformLoc, projectMat );
    postEffectProgram->setUniformValue( imagescaleUniformLoc, imageScale );
    postEffectProgram->setUniformValue( texCoodOffsetLoc, textCoordOffset);

    postEffectProgram->setUniformValue( fboTextureLoc, 0);

    // draw the quad
    glDrawElements(GL_TRIANGLES, 2*3, GL_UNSIGNED_SHORT, quad_indices );

    //glBindTexture(GL_TEXTURE_2D, 0);

    //postEffectProgram->disableAttributeArray(uvAttrLoc);
    //postEffectProgram->disableAttributeArray(posAttrLoc);

    //render the mandelbrot image ends
    postEffectProgram->release();

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

    //shader paremters
    hudMessage += "\nIters: ";
    tempStr.setNum(maxInterations, 'f', 0);
    hudMessage += tempStr;

    hudMessage += "\nRot: ";
    tempStr.setNum(rotateAngle, 'f', 2);
    hudMessage += tempStr;

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

    //renderLoopTimer->stop();

    //doneCurrent();

    swapBuffers();
    //emit NeedSwapBuffer();
}

void MandelGLWidget::startRendering()
{
    if(renderLoopTimer)
    {
        renderLoopTimer->start();
    }
}

void MandelGLWidget::DrawHUD()
{
    ComputeHUDRect();
    textPainter->setRenderHint(QPainter::TextAntialiasing);
    textPainter->setPen(Qt::white);
    textPainter->fillRect(hudRect, QColor(0, 0, 0, 127));
    textPainter->drawText(hudRect, Qt::AlignCenter, hudMessage);
}

void MandelGLWidget::ComputeHUDRect(bool forceUpdate)
{
    if( messageLength == 0 || messageLength != hudMessage.length() || forceUpdate)
    {
        QFontMetrics metrics(font());
        int border = qMax(4, metrics.leading());

        QRect rect = metrics.boundingRect(0, 0, width() - border, height(),
                                          Qt::AlignLeft, hudMessage);
        hudRect = QRect((width() - rect.width() - border), border * 2,
                     rect.width(), rect.height());

        messageLength = hudMessage.length();
    }
}

// events
#if !defined (QT_OPENGL_ES_2)
void MandelGLWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        lastDragPos = event->pos();

        //stop the mandelbrot rendering
        renderMandelbrot = false;
    }
}

void MandelGLWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() == Qt::LeftButton) 
    {
        pixelOffset = event->pos() - lastDragPos;
        lastDragPos = event->pos();

        //update shader position
        UpdateMandelbrotPos();

        //pan the current texture
        textCoordOffset.setX(textCoordOffset.x() + pixelOffset.x() / width() );
        textCoordOffset.setY(textCoordOffset.y() + pixelOffset.y() / height() );
    }
}

void MandelGLWidget::mouseReleaseEvent(QMouseEvent *event)
{
    // resume the rendering after mouse released
    renderMandelbrot = true;
    textCoordOffset.setX(0);
    textCoordOffset.setY(0);
}

#endif

void MandelGLWidget::keyPressEvent(QKeyEvent *event)
{

    switch (event->key())
    {

    // zoom in
    case Qt::Key_Z:
        scaleFactor *= 1.2f;
        maxInterations *= 1.02f;

        renderMandelbrot = true;
        break;
    // zoom out
    case Qt::Key_X:
        scaleFactor *= 0.8f;
        maxInterations /= 1.02f;

        renderMandelbrot = true;
        break;
    case Qt::Key_Escape:
        this->close();
        break;
    default:
        QGLWidget::keyPressEvent(event);
    }
}

bool MandelGLWidget::event(QEvent *event)
{
    switch (event->type())
    {
    case QEvent::TouchBegin:
    case QEvent::TouchUpdate:
    case QEvent::TouchEnd:
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

                    //stop the mandelbrot rendering
                    renderMandelbrot = false;

                }
                else if ( touchEvent->touchPointStates() & Qt::TouchPointMoved )
                {
                    pixelOffset = touchPoint0.pos() - lastDragPos;
                    lastDragPos = touchPoint0.pos();

                    UpdateMandelbrotPos();

                    //pan the current texture
                    textCoordOffset.setX(textCoordOffset.x() + pixelOffset.x() / width() );
                    textCoordOffset.setY(textCoordOffset.y() + pixelOffset.y() / height() );
                }
                else if ( touchEvent->touchPointStates() & Qt::TouchPointReleased )
                {
                    // resume the rendering after mouse released
                    renderMandelbrot = true;
                    textCoordOffset.setX(0);
                    textCoordOffset.setY(0);
                }
            }

            // multi touch zoom / rotate
            else if (touchPoints.count() == 2)
            {                
                // determine scale factor

                // calculate the vector for two touch points
                const QTouchEvent::TouchPoint &touchPoint1 = touchPoints.last();
                if ( touchEvent->touchPointStates() & Qt::TouchPointPressed )
                {
                    renderMandelbrot = false;
                    // p1: touch point 0
                    // p2: touch point 1
                    //originalLine = QLineF(touchPoint0.startPos(), touchPoint1.startPos());
                    originalP0P1 = touchPoint0.startPos() - touchPoint1.startPos();
                }
                else if ( touchEvent->touchPointStates() & Qt::TouchPointMoved )
                {
                    // determine the type of gesture by checking the vector formed by touch points
                    //QVector2D v0(touchPoint0.pos() - originalLine.p1());     //touch point 0 vector
                    //QVector2D v1(touchPoint1.pos() - originalLine.p2());     // touch point 1 vector
                    //QVector2D v2(originalLine.p1() - originalLine.p2());     // touch points 0->1 vector (original)
                    //QVector2D v3(touchPoint0.pos() - touchPoint1.pos());     // touch points 0->1 vector (now)

                    //v0.normalize();
                    //v1.normalize();
                    //v2.normalize();
                    //v3.normalize();

                    //qreal dotV0V2 = QVector2D::dotProduct(v0, -v2);
                    //qreal dotV1V2 = QVector2D::dotProduct(v1,  v2);
                    //qreal dotRotate = QVector2D::dotProduct(v2, v3);
                    //qreal dotv0v1 = QVector2D::dotProduct(v0,  v1);


                    //if ( dotv0v1 < 0.0f )
                    {
                        //qreal additionAngle = qAcos(dotRotate) * RADIAN_TO_DEGREE;


                        //if(qAbs(additionAngle) < 5.0)
                        {
                            //currentGesture = MandelGLWidget::PIN_ZOOM;
                            //QLineF currentLine(touchPoint0.pos(), touchPoint1.pos());
                            //qreal currentScaleFactor = currentLine.length() / originalLine.length();
                            QPointF currentP0P1 = touchPoint0.pos() - touchPoint1.pos();
                            qreal currentScaleFactor = currentP0P1.manhattanLength() / originalP0P1.manhattanLength();


                            originalP0P1 = currentP0P1;

                            scaleFactor *= currentScaleFactor;

                            float scaleLevel = scaleFactor / previousScale;


                            if( scaleLevel > ZOOM_STEP )
                            {
                                maxInterations *= INTERATION_STEP;
                                previousScale = scaleFactor;
                            }
                            else if ( scaleLevel  < 1.0f / ZOOM_STEP )
                            {
                                maxInterations /= INTERATION_STEP;
                                previousScale = scaleFactor;
                            }

                            imageScale *= currentScaleFactor;

                            //make sure reset the texture coordiniate offset in zoom mode
                            textCoordOffset.setX(0);
                            textCoordOffset.setY(0);

                            //additionAngle = 0.0f;
                        }

//                        rotateAngle += additionAngle;

//                        QMatrix4x4 view;
//                        view.rotate((int)rotateAngle % 360, 0, 0, 1);
//                        modelViewProjection = modelViewProjection * view;
                    }
                }


                else if (touchEvent->touchPointStates() & Qt::TouchPointReleased)
                {
//                    QPointF currentP0P1 = touchPoint0.pos() - touchPoint1.pos();
//                    qreal currentScaleFactor = currentP0P1.manhattanLength() / originalP0P1.manhattanLength();

//                    originalP0P1 = currentP0P1;

//                    // if one of the fingers is released, remember the current scale
//                    // factor so that adding another finger later will continue zooming
//                    // by adding new scale factor to the existing remembered value.
//                    scaleFactor *= currentScaleFactor;
//                    currentScaleFactor = 1;

                    renderMandelbrot = true;
                    imageScale = 1.0f;
                }
            }
            return true;
        }
    default:
        return QGLWidget::event(event);
    }
    return true;
}

void MandelGLWidget::UpdateMandelbrotPos()
{
    // remap the pixel offset from [0, width][0, height] to [-2, 1][-1, 1]
    centerPos.setX(centerPos.x() + pixelOffset.x() * projectedScaleFactor.x() / scaleFactor);
    centerPos.setY(centerPos.y() + pixelOffset.y() * projectedScaleFactor.y() / scaleFactor);
    //centerPos += QVector2D(pixelOffset.x() * -0.00234375f,
    //                       pixelOffset.y() * -0.00277778f);
    //centerPos.setX(centerPos.x() + pixelOffset.x() * -0.00234375f);
    //centerPos.setY(centerPos.y() + pixelOffset.y() * -0.00277778f);
}

void MandelGLWidget::UpdateProjectedScales()
{
    float newScaleFactor = 4.0f / float(height());
    projectedScaleFactor.setX ( -newScaleFactor);//-3.0f / float(width())
    projectedScaleFactor.setY ( newScaleFactor);//2.0f / float(height())
}
