/*
The MIT License (MIT)

Copyright (c) 2017 Lancaster University.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.
*/

#include "CoordinateSystem.h"

/**
 * Constructor.
 *
 * Creates a new coordinatespace transformation object.
 *
 * @param system the CoordinateSystem to generated as output.
 * @param upsidedown set if the sensor is mounted inverted (upside down) on the device board.
 * @param rotated defines the rotation of the sensor on the PCB, with respect to pin 1 being at the top left corner
 * when viewing the device from its "natural" (user defined) orientation. n.b. if the sensor is upside down, the rotation
 * should be defined w.r.t. lookign at the side of the device where the sensor is mounted.
 */
CoordinateSpace::CoordinateSpace(CoordinateSystem system, bool upsidedown, int rotated)
{
    this->system = system;
    this->upsidedown = upsidedown;
    this->rotated = rotated;
}

/**
 * Transforms a given 3D x,y,z tuple from ENU format into that format defined in this instance.
 *
 * @param a the sample point to convert, in ENU format.
 * @return the equivalent location of 's' in the coordinate space specified in the constructor.
 */
Sample3D CoordinateSpace::transform(Sample3D s)
{
    return transform(s, system);
}

/**
 * Transforms a given 3D x,y,z tuple from ENU format into that format defined in this instance.
 *
 * @param a the sample point to convert, in ENU format.
 * @param system The coordinate system to use in the result.
 * @return the equivalent location of 's' in the coordinate space specified in the constructor, and coordinate system supplied.
 */
Sample3D CoordinateSpace::transform(Sample3D s, CoordinateSystem system)
{
    Sample3D result = s;
    int temp;

    // If we've been asked to supply raw data, simply return it.
    if (system == RAW)
        return result;

    // Firstly, handle any inversions.
    // As we know the input is in ENU format, this means we flip the polarity of the X and Z axes.
    if(upsidedown)
    {
        result.y = -result.y;
        result.z = -result.z;
    }

    // Now, handle any rotations.
    switch (rotated)
    {
        case COORDINATE_SPACE_ROTATED_90:
            temp = -result.x;
            result.x = result.y;
            result.y = temp;
            break;

        case COORDINATE_SPACE_ROTATED_180:
            result.x = -result.x;
            result.y = -result.y;
            break;

        case COORDINATE_SPACE_ROTATED_270:
            temp = result.x;
            result.x = -result.y;
            result.y = temp;
            break;
    }

    // Finally, apply coordinate system transformation.
    switch (system)
    {
        case NORTH_EAST_DOWN:
            result.y = -result.y;
            result.z = -result.z;
            break;

        case SIMPLE_CARTESIAN:
            temp = result.x;
            result.x = result.y;
            result.y = temp;
            result.z = -result.z;
            break;

        default:                    // EAST_NORTH_UP
            break;
    }

    return result;

}

