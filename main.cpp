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

#include "fractDroidGL.h"
#include "MandelGLWidget.h"
#include "graphicsview.h"
#include <QtGui/QApplication>

int main(int argc, char *argv[])
{
    QApplication::setAttribute(Qt::AA_X11InitThreads);
	QApplication a(argc, argv);

    MandelGLWidget w;
#if !defined (QT_OPENGL_ES_2)
    w.resize(1280, 720);
    w.show();
#else
    w.showMaximized();
#endif

	return a.exec();
}
