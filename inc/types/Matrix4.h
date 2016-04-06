/*
The MIT License (MIT)

Copyright (c) 2016 British Broadcasting Corporation.
This software is provided by Lancaster University by arrangement with the BBC.

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

#ifndef MICROBIT_MATRIX4_H
#define MICROBIT_MATRIX4_H

#include "MicroBitConfig.h"

/**
* Class definition for a simple matrix, that is optimised for nx4 or 4xn matrices.
*
* This class is heavily optimised for these commonly used matrices as used in 3D geometry.
* Whilst this class does support basic operations on matrices of any dimension, it is not intended as a
* general purpose matrix class as inversion operations are only provided for 4x4 matrices.
* For programmers needing more flexible Matrix support, the Matrix and MatrixMath classes from
* Ernsesto Palacios provide a good basis:
*
* https://developer.mbed.org/cookbook/MatrixClass
* https://developer.mbed.org/users/Yo_Robot/code/MatrixMath/
*/
class Matrix4
{
	float   *data;         // Linear buffer representing the matrix.
	int     rows;           // The number of rows in the matrix.
	int     cols;           // The number of columns in the matrix.

public:

	/**
	  * Constructor.
	  * Create a matrix of the given size.
	  *
	  * @param rows the number of rows in the matrix to be created.
	  *
	  * @param cols the number of columns in the matrix to be created.
	  *
	  * @code
	  * Matrix4(10, 4);        // Creates a Matrix with 10 rows and 4 columns.
	  * @endcode
	  */
	Matrix4(int rows, int cols);

	/**
	  * Constructor.
	  * Create a matrix that is an identical copy of the given matrix.
	  *
	  * @param matrix The matrix to copy.
	  *
	  * @code
	  * Matrix newMatrix(matrix);        .
	  * @endcode
	  */
	Matrix4(const Matrix4 &matrix);

	/**
	  * Determines the number of columns in this matrix.
	  *
	  * @return The number of columns in the matrix.
	  *
	  * @code
	  * int c = matrix.width();
	  * @endcode
	  */
	int width();

	/**
	  * Determines the number of rows in this matrix.
	  *
	  * @return The number of rows in the matrix.
	  *
	  * @code
	  * int r = matrix.height();
	  * @endcode
	  */
	int height();

	/**
	  * Reads the matrix element at the given position.
	  *
	  * @param row The row of the element to read.
	  *
	  * @param col The column of the element to read.
	  *
	  * @return The value of the matrix element at the given position. 0 is returned if the given index is out of range.
	  *
	  * @code
	  * float v = matrix.get(1,2);
	  * @endcode
	  */
	float get(int row, int col);

	/**
	  * Writes the matrix element at the given position.
	  *
	  * @param row The row of the element to write.
	  *
	  * @param col The column of the element to write.
	  *
	  * @param v The new value of the element.
	  *
	  * @code
	  * matrix.set(1,2,42.0);
	  * @endcode
	  */
	void set(int row, int col, float v);

	/**
	  * Transposes this matrix.
	  *
	  * @return the resultant matrix.
	  *
	  * @code
	  * matrix.transpose();
	  * @endcode
	  */
	Matrix4 transpose();

	/**
	  * Multiplies this matrix with the given matrix (if possible).
	  *
	  * @param matrix the matrix to multiply this matrix's values against.
	  *
	  * @param transpose Transpose the matrices before multiplication. Defaults to false.
	  *
	  * @return the resultant matrix. An empty matrix is returned if the operation canot be completed.
	  *
	  * @code
	  * Matrix result = matrixA.multiply(matrixB);
	  * @endcode
	  */
	Matrix4 multiply(Matrix4 &matrix, bool transpose = false);

	/**
	  * Multiplies the transpose of this matrix with the given matrix (if possible).
      *
	  * @param matrix the matrix to multiply this matrix's values against.
	  *
	  * @return the resultant matrix. An empty matrix is returned if the operation canot be completed.
	  *
	  * @code
	  * Matrix result = matrixA.multiplyT(matrixB);
	  * @endcode
	  */
	Matrix4 multiplyT(Matrix4 &matrix);

	/**
	  * Performs an optimised inversion of a 4x4 matrix.
	  * Only 4x4 matrices are supported by this operation.
	  *
	  * @return the resultant matrix. An empty matrix is returned if the operation canot be completed.
	  *
	  * @code
	  * Matrix result = matrixA.invert();
	  * @endcode
	  */
	Matrix4 invert();

	/**
	  * Destructor.
	  *
	  * Frees any memory consumed by this Matrix4 instance.
	  */
	~Matrix4();
};

/**
  * Multiplies the transpose of this matrix with the given matrix (if possible).
  *
  * @return the resultant matrix. An empty matrix is returned if the operation canot be completed.
  *
  * @code
  * Matrix result = matrixA.multiplyT(matrixB);
  * @endcode
  */
inline Matrix4 Matrix4::multiplyT(Matrix4 &matrix)
{
    return multiply(matrix, true);
}

#endif
