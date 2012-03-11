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

uniform sampler2D fboTexture;
varying mediump vec2 TexCoord;

void main(void)
{
    if(TexCoord.x > 1.0 || TexCoord.x < 0.0 || TexCoord.y > 1.0 || TexCoord.y < 0.0)
        gl_FragColor = vec4(0.0, 0.0, 0.0, 0.5);
    else
        gl_FragColor = texture2D(fboTexture, TexCoord);
}
