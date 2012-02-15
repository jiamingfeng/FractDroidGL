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

#if defined GL_ES
#define double float
#define dvec2 vec2
#else
#extension GL_ARB_gpu_shader_fp64 : enable
#endif


uniform sampler2D lookUpTexture;
uniform mediump float maxIterations;

uniform highp double rotRadian;    //rotation in radian
uniform highp dvec2 rotatePivot;
uniform highp dvec2 center;

varying mediump vec2 TexCoord;

void main (void)
{
    highp dvec2 c;
    highp dvec2 TexCoordMod;

    TexCoordMod.x = double(TexCoord.x) - rotatePivot.x + center.x;
    TexCoordMod.y = double(TexCoord.y) - rotatePivot.y + center.y;

    c.x = TexCoordMod.x * cos(rotRadian) - TexCoordMod.y * sin(rotRadian) + rotatePivot.x;
    c.y = TexCoordMod.y * cos(rotRadian) + TexCoordMod.x * sin(rotRadian) + rotatePivot.y;

    mediump vec3 color = texture2D(lookUpTexture, vec2(1.0, 0.0)).bgr;
    
    // optimization. early out

    // cardioid
    // q = ( x - 1/4 )^2 + y^2
    // q ( q + ( x - 1/4 )) < 1/4 y^2
    mediump double cx = c.x - 0.25;
    mediump double cy2 = c.y * c.y;
    mediump double q = cx * cx + cy2;

    // period-2 bulb
    // (x + 1) ^2 + y ^ 2 < 1/16
    mediump double cxp12 = (c.x + 1.0) * (c.x + 1.0);

    if ( (4.0 * q * (q + cx) > cy2) && (cxp12 + cy2 > 0.0625))
    {
        highp dvec2 z = c;

        // tegra 2 CPU need to have constant loop count
        // (e.g.  i < 64 instead of i < maxIterations)
        mediump float i;

        for ( i = 0.0; i < maxIterations && dot(z, z) < 4.0; i += 1.0)
        {
            z = dvec2(z.x*z.x - z.y*z.y, 2.0*z.x*z.y) + c;
        }

        if ( i < maxIterations + 1.0 )
        {
            //Normalized Iteration Count to get a smoother image
            //smooth iter = iter + ( log(log(bailout)-log(log(cabs(z))) )/log(2)

            mediump vec2 s = vec2( (i - log(log(float(dot(z, z)))/ 2.0) / log(2.0)) / maxIterations, 0.0);
            color = texture2D(lookUpTexture, s).bgr;
        }
    }

    gl_FragColor = vec4(color, 1.0);
}
