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

#include <QtGui>

FractDroidGL::FractDroidGL(QWidget *parent, Qt::WFlags flags)
	: QMainWindow(parent, flags)
{
	//ui.setupUi(this);
	setWindowTitle(tr("Mandelbrot"));
	glWidget = new MandelGLWidget(this);

	QGridLayout *layout = new QGridLayout;
	layout->addWidget(glWidget, 0, 0);
	setLayout(layout);
}

FractDroidGL::~FractDroidGL()
{

}
