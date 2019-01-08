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

#ifndef COORDINATE_SYSTEM_H
#define COORDINATE_SYSTEM_H
#include "MicroBitConfig.h"

/**
 * Co-ordinate systems that can be used.
 * RAW: Unaltered data. Data will be returned directly from the accelerometer.
 *
 * SIMPLE_CARTESIAN: Data will be returned based on an easy to understand alignment, consistent with the cartesian system taught in schools.
 * When held upright, facing the user:
 *
 *                            /
 *    +--------------------+ z
 *    |                    |
 *    |       .....        |
 *    | *     .....      * |
 * ^  |       .....        |
 * |  |                    |
 * y  +--------------------+  x-->
 *
 *
 * NORTH_EAST_DOWN: Data will be returned based on the industry convention of the North East Down (NED) system.
 * When held upright, facing the user:
 *
 *                            z
 *    +--------------------+ /
 *    |                    |
 *    |       .....        |
 *    | *     .....      * |
 * ^  |       .....        |
 * |  |                    |
 * x  +--------------------+  y-->
 *
 */

#define COORDINATE_SPACE_ROTATED_0      0
#define COORDINATE_SPACE_ROTATED_90     1
#define COORDINATE_SPACE_ROTATED_180    2
#define COORDINATE_SPACE_ROTATED_270    3

enum CoordinateSystem
{
    RAW,
    SIMPLE_CARTESIAN,
    NORTH_EAST_DOWN,
    EAST_NORTH_UP
};

struct Sample3D
{
    int         x;
    int         y;
    int         z;

    Sample3D()
    {
        this->x = 0;
        this->y = 0;
        this->z = 0;
    }

    Sample3D(int x, int y, int z)
    {
        this->x = x;
        this->y = y;
        this->z = z;
    }

    Sample3D operator-(const Sample3D& other) const
    {
        Sample3D result;

        result.x = x - other.x;
        result.y = y - other.y;
        result.z = z - other.z;

        return result;
    }

    Sample3D operator+(const Sample3D& other) const
    {
        Sample3D result;

        result.x = x + other.x;
        result.y = y + other.y;
        result.z = z + other.z;

        return result;
    }

    bool operator==(const Sample3D& other) const
    {
        return x == other.x && y == other.y && z == other.z;
    }

    bool operator!=(const Sample3D& other) const
    {
        return !(x == other.x && y == other.y && z == other.z);
    }

    float dSquared(Sample3D &s)
    {
        float dx = x - s.x;
        float dy = y - s.y;
        float dz = z - s.z;

        return (dx*dx) + (dy*dy) + (dz*dz);
    }

};


class CoordinateSpace
{
    public:

        CoordinateSystem    system;
        bool                upsidedown;
        int                 rotated;

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
        CoordinateSpace(CoordinateSystem system, bool upsidedown = false, int rotated = COORDINATE_SPACE_ROTATED_0);

        /**
         * Transforms a given 3D x,y,z tuple from ENU format into that format defined in this instance.
         *
         * @param a the sample point to convert, in ENU format.
         * @return the equivalent location of 's' in the coordinate space specified in the constructor.
         */
        Sample3D transform(Sample3D s);

        /**
         * Transforms a given 3D x,y,z tuple from ENU format into that format defined in this instance.
         *
         * @param a the sample point to convert, in ENU format.
         * @param system The coordinate system to use in the result.
         * @return the equivalent location of 's' in the coordinate space specified in the constructor, and coordinate system supplied.
         */
        Sample3D transform(Sample3D s, CoordinateSystem system);

};
#endif
