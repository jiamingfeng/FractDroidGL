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

#ifndef FRACTDROIDGL_H
#define FRACTDROIDGL_H

#include <QtGui/QMainWindow>
//#include "ui_qmandelbrot.h"

class MandelGLWidget;

class FractDroidGL : public QMainWindow
{
	Q_OBJECT

public:
    FractDroidGL(QWidget *parent = 0, Qt::WFlags flags = 0);
    ~FractDroidGL();

private:
	//Ui::QMandelbrotClass ui;
	MandelGLWidget* glWidget;
};

#endif // FRACTDROIDGL_H
