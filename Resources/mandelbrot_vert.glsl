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
//===============END=====================================

uniform mediump float scale;

varying mediump vec2 TexCoord;


void main(void)
{
    gl_Position = MVP *  vec4(Position, 0.0, 1.0);
    TexCoord = (InTexCoord - 0.5) * 4.0 / scale;
}
