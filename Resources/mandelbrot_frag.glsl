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

//#define ENABLE_PERIODICITY_CHECK
//#define SHOW_PERIODICITY_CHECK


// enable/disable fall back shader code for Tegra 2 GPU
//#define FALL_BACK

#ifdef FALL_BACK
const int FIX_ITERATION = 256;
#endif

uniform sampler2D lookUpTexture;
uniform int maxIterations;

uniform mediump float rotRadian;    //rotation in radian
uniform mediump vec2 rotatePivot;
uniform mediump vec2 center;

varying mediump vec2 TexCoord;

const mediump float epsilon = 1e-6;
const mediump float log2 = log(2.0);

void main (void)
{
#ifdef FALL_BACK
    int iterationCount = FIX_ITERATION;
#else
    int iterationCount = maxIterations;
#endif

    highp dvec2 c;
    mediump dvec2 TexCoordMod;

    TexCoordMod.x = double(TexCoord.x) - rotatePivot.x + center.x;
    TexCoordMod.y = double(TexCoord.y) - rotatePivot.y + center.y;

    c.x = TexCoordMod.x * cos(rotRadian) - TexCoordMod.y * sin(rotRadian) + rotatePivot.x;
    c.y = TexCoordMod.y * cos(rotRadian) + TexCoordMod.x * sin(rotRadian) + rotatePivot.y;

    mediump vec2 s = vec2(1.0, 0.0);
    
    // optimization. early out

    // cardioid
    // q = ( x - 1/4 )^2 + y^2
    // q ( q + ( x - 1/4 )) < 1/4 y^2
    mediump float cx = float(c.x - 0.25);
    mediump float cy2 = float(c.y * c.y);
    mediump float q = cx * cx + cy2;

    if ( 4.0 * q * (q + cx) >= cy2 )
    {
        // period-2 bulb
        // (x + 1) ^2 + y ^ 2 < 1/16
        mediump float cxp12 = float((c.x + 1.0) * (c.x + 1.0));

        if ( cxp12 + cy2 >= 0.0625 )
        {
#ifdef ENABLE_PERIODICITY_CHECK
            int check = 3;
            int checkCounter = 0;

            int update = 10;
            int updateCounter = 0;

            highp double checkPeriodicityX = 0.0;
            highp double checkPeriodicityY = 0.0;

            bool periodicityCheckFailed = false;

#endif

            highp dvec2 z = c;

            // tegra 2 CPU need to have constant loop count
            // (e.g.  i < 64 instead of i < maxIterations)
            int i;

            for ( i = 0; i < iterationCount && dot(z, z) < 4.0; i ++)
            {
                z = dvec2(z.x*z.x - z.y*z.y, 2.0*z.x*z.y) + c;

#ifdef ENABLE_PERIODICITY_CHECK

                //periodicity check
                if ( abs(checkPeriodicityX - z.x) < epsilon && abs(checkPeriodicityY - z.y) < epsilon)
                {
                    periodicityCheckFailed = true;
                    break;
                }

                //Update history
                if (check == checkCounter)
                {
                    checkCounter = 0;

                    //Double the value of check
                    if (update == updateCounter)
                    {
                        updateCounter = 0;
                        check *= 2;
                    }
                    updateCounter++;

                    checkPeriodicityX = z.x;
                    checkPeriodicityY = z.y;
                }
                checkCounter++;
#endif

            }

#ifdef ENABLE_PERIODICITY_CHECK
            if ( periodicityCheckFailed == false && i < iterationCount + 1 )
#else
            if ( i < iterationCount + 1 )
#endif
            {
                //Normalized Iteration Count to get a smoother image
                //smooth iter = iter + ( log(log(bailout)-log(log(cabs(z))) )/log(2)

                s.x = (float(i) - log(log(float(dot(z, z)))/ 2.0) / log2) / float(iterationCount);
                //bgColor = texture2D(lookUpTexture, s).bgra;
            }
        }
    }

#ifdef SHOW_PERIODICITY_CHECK
    mediump vec4 bgColor = vec4(1.0, 0.0, 1.0, 1.0);
#else
    mediump vec4 bgColor = texture2D(lookUpTexture, s).bgra;
#endif


    gl_FragColor = bgColor;
}
