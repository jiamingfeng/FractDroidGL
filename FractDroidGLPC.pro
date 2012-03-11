#-----------------------------------------------------------
#
# Project created by NecessitasQtCreator 2012-01-16T22:14:51
#
#-----------------------------------------------------------

QT       += core gui opengl

TARGET = FractDroidGL
TEMPLATE = app


SOURCES += main.cpp\
        MandelGLWidget.cpp \
    fractDroidGL.cpp \
    fractalrenderer.cpp

HEADERS  += MandelGLWidget.h \
    fractDroidGL.h \
    fractalrenderer.h

RESOURCES += FractDroidGL.qrc

OTHER_FILES += \
    android/src/org/kde/necessitas/ministro/IMinistro.aidl \
    android/src/org/kde/necessitas/ministro/IMinistroCallback.aidl \
    android/src/org/kde/necessitas/origo/QtActivity.java \
    android/src/org/kde/necessitas/origo/QtApplication.java \
    android/AndroidManifest.xml \
    android/res/drawable/icon.png \
    android/res/drawable-mdpi/icon.png \
    android/res/drawable-xhdpi/icon.png \
    android/res/values/libs.xml \
    android/res/values/strings.xml \
    android/res/values-it/strings.xml \
    android/res/values-de/strings.xml \
    android/res/values-ru/strings.xml \
    android/res/values-ms/strings.xml \
    android/res/values-et/strings.xml \
    android/res/values-id/strings.xml \
    android/res/values-zh-rTW/strings.xml \
    android/res/values-fa/strings.xml \
    android/res/values-zh-rCN/strings.xml \
    android/res/drawable-ldpi/icon.png \
    android/res/values-nb/strings.xml \
    android/res/values-nl/strings.xml \
    android/res/values-ja/strings.xml \
    android/res/values-rs/strings.xml \
    android/res/values-es/strings.xml \
    android/res/values-pl/strings.xml \
    android/res/values-ro/strings.xml \
    android/res/values-fr/strings.xml \
    android/res/values-pt-rBR/strings.xml \
    android/res/values-el/strings.xml \
    android/res/drawable-hdpi/icon.png \
    Resources/lookup.png \
    Resources/vert.glsl \
    Resources/mandelbrot_vert.glsl \
    Resources/mandelbrot_frag.glsl \
    Resources/frag.glsl





























