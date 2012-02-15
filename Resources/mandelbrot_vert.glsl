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

//DONT CHANAGE THE ORDER OF THE FOLLOWING ATTRIBUTES/UNIFORMS
//SINCE IT SHARE VARIABLE POSITION INDEXES WITH OTHER SHADERS

#if defined GL_ES
#define double float
#define dvec2 vec2
#else
#extension GL_ARB_gpu_shader_fp64 : enable
#endif


//===============BEGIN===================================
uniform mediump mat4 MVP; // model-view-project matrix
attribute mediump vec2 Position;
attribute highp vec2 InTexCoord;
uniform mediump float scale;        //zoom factor
uniform mediump vec2 resolution;    //use to keep the proportion of mandelbrot set
//===============END=====================================


uniform highp float rotRadian;    //rotation in radian
uniform highp vec2 rotatePivot;
//uniform mediump mat4 rotMat;
uniform highp vec2 center;
varying highp vec2 TexCoord;


void main(void)
{
    gl_Position = MVP *  vec4(Position, 0.0, 1.0);

    // scale the uv from [0, 1] to [-0.5, 0.5], scale it and add the texture coordinate offset
    // translate  -(rotation center) e.g. (0.5, 0.5)
    // rotate the coordinates
    // translate  (rotation center) e.g. (0.5, 0.5)

    highp float stScale = resolution.x / resolution.y;
    highp vec2 scaledTexCoord;
    scaledTexCoord.x = (InTexCoord.x -0.5) * stScale;
    scaledTexCoord.y = InTexCoord.y - 0.5;
    highp vec2 TexCoordOrig = scaledTexCoord * 4.0 / scale + center;

    TexCoordOrig -= rotatePivot;

    TexCoord.x = TexCoordOrig.x * cos(rotRadian) - TexCoordOrig.y * sin(rotRadian);
    TexCoord.y = TexCoordOrig.y * cos(rotRadian) + TexCoordOrig.x * sin(rotRadian);


    TexCoord += rotatePivot;

}
