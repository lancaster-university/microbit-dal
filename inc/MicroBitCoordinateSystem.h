#ifndef MICROBIT_COORDINATE_SYSTEM_H
#define MICROBIT_COORDINATE_SYSTEM_H

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
enum MicroBitCoordinateSystem
{
    RAW,
    SIMPLE_CARTESIAN,
    NORTH_EAST_DOWN
};

#endif
