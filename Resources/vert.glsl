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

//===============BEGIN===================================
uniform mediump mat4 MVP; // model-view-project matrix
attribute mediump vec2 Position;
attribute mediump vec2 InTexCoord;
uniform mediump float scale;
uniform mediump vec2 resolution;    //use to keep the proportion of mandelbrot set
//===============END=====================================

uniform mediump vec2 TexCoordoffset;
uniform mediump float RotationOffset;

varying mediump vec2 TexCoord;


void main(void)
{
    gl_Position = MVP * vec4(Position, 0.0, 1.0);

    // scale the uv from [0, 1] to [-0.5, 0.5], scale it and add the texture coordinate offset
    // translate  -(rotation center) e.g. (0.5, 0.5)
    // rotate the coordinates
    // translate  (rotation center) e.g. (0.5, 0.5)

    //(InTexCoord - 0.5) / scale  + 0.5 - TexCoordoffset - 0.5;
    mediump vec2 TexCoordOrig = (InTexCoord - 0.5) / scale - TexCoordoffset;

    // keept the width/height scale while rotating
    mediump float stScale = resolution.x / resolution.y;

    TexCoordOrig.x *= stScale;

    TexCoord.x = TexCoordOrig.x * cos(-RotationOffset) - TexCoordOrig.y * sin(-RotationOffset);
    TexCoord.y = TexCoordOrig.y * cos(-RotationOffset) + TexCoordOrig.x * sin(-RotationOffset);

    TexCoord.x /= stScale;

    TexCoord += 0.5;



}
