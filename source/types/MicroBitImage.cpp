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

/**
  * Class definition for a MicroBitImage.
  *
  * An MicroBitImage is a simple bitmap representation of an image.
  * n.b. This is a mutable, managed type.
  */

#include "MicroBitConfig.h"
#include "MicroBitImage.h"
#include "MicroBitFont.h"
#include "MicroBitCompat.h"
#include "ManagedString.h"
#include "ErrorNo.h"


/**
  * The null image. We actally create a small one byte buffer here, just to keep NULL pointers out of the equation.
  */
static const uint16_t empty[] __attribute__ ((aligned (4))) = { 0xffff, 1, 1, 0, };
MicroBitImage MicroBitImage::EmptyImage((ImageData*)(void*)empty);

/**
  * Default Constructor.
  * Creates a new reference to the empty MicroBitImage bitmap
  *
  * @code
  * MicroBitImage i(); //an empty image instance
  * @endcode
  */
MicroBitImage::MicroBitImage()
{
    // Create new reference to the EmptyImage and we're done.
    init_empty();
}


/**
  * Constructor.
  * Create a blank bitmap representation of a given size.
  *
  * @param x the width of the image.
  *
  * @param y the height of the image.
  *
  * Bitmap buffer is linear, with 8 bits per pixel, row by row,
  * top to bottom with no word alignment. Stride is therefore the image width in pixels.
  * in where w and h are width and height respectively, the layout is therefore:
  *
  * |[0,0]...[w,o][1,0]...[w,1]  ...  [[w,h]
  *
  * A copy of the image is made in RAM, as images are mutable.
  *
  * TODO: Consider an immutable flavour, which might save us RAM for animation spritesheets...
  * ...as these could be kept in FLASH.
  */
MicroBitImage::MicroBitImage(const int16_t x, const int16_t y)
{
    this->init(x,y,NULL);
}

/**
  * Copy Constructor.
  * Add ourselves as a reference to an existing MicroBitImage.
  *
  * @param image The MicroBitImage to reference.
  *
  * @code
  * MicroBitImage i("0,1,0,1,0\n");
  * MicroBitImage i2(i); //points to i
  * @endcode
  */
MicroBitImage::MicroBitImage(const MicroBitImage &image)
{
    ptr = image.ptr;
    ptr->incr();
}

/**
  * Constructor.
  * Create a blank bitmap representation of a given size.
  *
  * @param s A text based representation of the image given whitespace delimited numeric values.
  *
  * @code
  * MicroBitImage i("0,1,0,1,0\n1,0,1,0,1\n0,1,0,1,0\n1,0,1,0,1\n0,1,0,1,0\n"); // 5x5 image
  * @endcode
  */
MicroBitImage::MicroBitImage(const char *s)
{
    int width = 0;
    int height = 0;
    int count = 0;
    int digit = 0;

    const char *parseReadPtr;
    int parseValue;
    uint8_t *bitmapPtr;

    if (s == NULL)
    {
        init_empty();
        return;
    }

    // First pass: Parse the string to determine the geometry of the image.
    // We do this from first principles to avoid unecessary load of the strtok() libs etc.
    parseReadPtr = s;

    while (*parseReadPtr)
    {
        if (isdigit(*parseReadPtr))
        {
            // Ignore numbers.
            digit = 1;
        }
        else if (*parseReadPtr =='\n')
        {
            if (digit)
            {
                count++;
                digit = 0;
            }

            height++;

            width = count > width ? count : width;
            count = 0;
        }
        else
        {
            if (digit)
            {
                count++;
                digit = 0;
            }
        }

        parseReadPtr++;
    }

    this->init(width, height, NULL);

    // Second pass: collect the data.
    parseReadPtr = s;
    parseValue = -1;
    bitmapPtr = this->getBitmap();

    while (*parseReadPtr)
    {
        if (isdigit(*parseReadPtr))
        {
            if (parseValue < 0)
            {
                parseValue = 0;
            }
            parseValue = parseValue * 10 + *parseReadPtr - '0';
        }
        else
        {
            if (parseValue >= 0)
            {
                *bitmapPtr = parseValue;
                bitmapPtr++;
                parseValue = -1;
            }
        }

        parseReadPtr++;
    }
}

/**
  * Constructor.
  * Create an image from a specially prepared constant array, with no copying. Will call ptr->incr().
  *
  * @param ptr The literal - first two bytes should be 0xff, then width, 0, height, 0, and the bitmap. Width and height are 16 bit. The literal has to be 4-byte aligned.
  *
  * @code
  * static const uint8_t heart[] __attribute__ ((aligned (4))) = { 0xff, 0xff, 10, 0, 5, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 1, 0, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 0, 0, 1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, }; // a cute heart
  * MicroBitImage i((ImageData*)(void*)heart);
  * @endcode
  */
MicroBitImage::MicroBitImage(ImageData *p)
{
    if(p == NULL)
    {
        init_empty();
        return;
    }

    ptr = p;
    ptr->incr();
}

/**
  * Get current ptr, do not decr() it, and set the current instance to empty image.
  *
  * This is to be used by specialized runtimes which pass ImageData around.
  */
ImageData *MicroBitImage::leakData()
{
    ImageData* res = ptr;
    init_empty();
    return res;
}


/**
  * Constructor.
  * Create a bitmap representation of a given size, based on a given buffer.
  *
  * @param x the width of the image.
  *
  * @param y the height of the image.
  *
  * @param bitmap a 2D array representing the image.
  *
  * @code
  * const uint8_t heart[] = { 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 1, 0, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 0, 0, 1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, }; // a cute heart
  * MicroBitImage i(10,5,heart);
  * @endcode
  */
MicroBitImage::MicroBitImage(const int16_t x, const int16_t y, const uint8_t *bitmap)
{
    this->init(x,y,bitmap);
}

/**
  * Destructor.
  *
  * Removes buffer resources held by the instance.
  */
MicroBitImage::~MicroBitImage()
{
    ptr->decr();
}

/**
  * Internal constructor which defaults to the EmptyImage instance variable
  */
void MicroBitImage::init_empty()
{
    ptr = (ImageData*)(void*)empty;
}

/**
  * Internal constructor which provides sanity checking and initialises class properties.
  *
  * @param x the width of the image
  *
  * @param y the height of the image
  *
  * @param bitmap an array of integers that make up an image.
  */
void MicroBitImage::init(const int16_t x, const int16_t y, const uint8_t *bitmap)
{
    //sanity check size of image - you cannot have a negative sizes
    if(x < 0 || y < 0)
    {
        init_empty();
        return;
    }


    // Create a copy of the array
    ptr = (ImageData*)malloc(sizeof(ImageData) + x * y);
    ptr->init();
    ptr->width = x;
    ptr->height = y;

    // create a linear buffer to represent the image. We could use a jagged/2D array here, but experimentation
    // showed this had a negative effect on memory management (heap fragmentation etc).

    if (bitmap)
        this->printImage(x,y,bitmap);
    else
        this->clear();
}

/**
  * Copy assign operation.
  *
  * Called when one MicroBitImage is assigned the value of another using the '=' operator.
  *
  * Decrement our reference count and free up the buffer as necessary.
  *
  * Then, update our buffer to refer to that of the supplied MicroBitImage,
  * and increase its reference count.
  *
  * @param s The MicroBitImage to reference.
  *
  * @code
  * const uint8_t heart[] = { 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 1, 0, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 0, 0, 1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, }; // a cute heart
  * MicroBitImage i(10,5,heart);
  * MicroBitImage i1();
  * i1 = i; // i1 now references i
  * @endcode
  */
MicroBitImage& MicroBitImage::operator = (const MicroBitImage& i)
{
    if(ptr == i.ptr)
        return *this;

    ptr->decr();
    ptr = i.ptr;
    ptr->incr();

    return *this;
}

/**
  * Equality operation.
  *
  * Called when one MicroBitImage is tested to be equal to another using the '==' operator.
  *
  * @param i The MicroBitImage to test ourselves against.
  *
  * @return true if this MicroBitImage is identical to the one supplied, false otherwise.
  *
  * @code
  * MicroBitDisplay display;
  * MicroBitImage i();
  * MicroBitImage i1();
  *
  * if(i == i1) //will be true
  *     display.scroll("true");
  * @endcode
  */
bool MicroBitImage::operator== (const MicroBitImage& i)
{
    if (ptr == i.ptr)
        return true;
    else
        return (ptr->width == i.ptr->width && ptr->height == i.ptr->height && (memcmp(getBitmap(), i.ptr->data, getSize())==0));
}


/**
  * Resets all pixels in this image to 0.
  *
  * @code
  * MicroBitImage i("0,1,0,1,0\n1,0,1,0,1\n0,1,0,1,0\n1,0,1,0,1\n0,1,0,1,0\n"); // 5x5 image
  * i.clear();
  * @endcode
  */
void MicroBitImage::clear()
{
    memclr(getBitmap(), getSize());
}

/**
  * Sets the pixel at the given co-ordinates to a given value.
  *
  * @param x The co-ordinate of the pixel to change.
  *
  * @param y The co-ordinate of the pixel to change.
  *
  * @param value The new value of the pixel (the brightness level 0-255)
  *
  * @return MICROBIT_OK, or MICROBIT_INVALID_PARAMETER.
  *
  * @code
  * MicroBitImage i("0,1,0,1,0\n1,0,1,0,1\n0,1,0,1,0\n1,0,1,0,1\n0,1,0,1,0\n"); // 5x5 image
  * i.setPixelValue(0,0,255);
  * @endcode
  *
  * @note all coordinates originate from the top left of an image.
  */
int MicroBitImage::setPixelValue(int16_t x , int16_t y, uint8_t value)
{
    //sanity check
    if(x >= getWidth() || y >= getHeight() || x < 0 || y < 0)
        return MICROBIT_INVALID_PARAMETER;

    this->getBitmap()[y*getWidth()+x] = value;
    return MICROBIT_OK;
}

/**
  * Retrieves the value of a given pixel.
  *
  * @param x The x co-ordinate of the pixel to read. Must be within the dimensions of the image.
  *
  * @param y The y co-ordinate of the pixel to read. Must be within the dimensions of the image.
  *
  * @return The value assigned to the given pixel location (the brightness level 0-255), or MICROBIT_INVALID_PARAMETER.
  *
  * @code
  * MicroBitImage i("0,1,0,1,0\n1,0,1,0,1\n0,1,0,1,0\n1,0,1,0,1\n0,1,0,1,0\n"); // 5x5 image
  * i.getPixelValue(0,0); //should be 0;
  * @endcode
  */
int MicroBitImage::getPixelValue(int16_t x , int16_t y)
{
    //sanity check
    if(x >= getWidth() || y >= getHeight() || x < 0 || y < 0)
        return MICROBIT_INVALID_PARAMETER;

    return this->getBitmap()[y*getWidth()+x];
}

/**
  * Replaces the content of this image with that of a given 2D array representing
  * the image.
  *
  * @param width the width of the image. Must be within the dimensions of the image.
  *
  * @param height the height of the image. Must be within the dimensions of the image.
  *
  * @param bitmap a 2D array representing the image.
  *
  * @return MICROBIT_OK on success, or MICROBIT_INVALID_PARAMETER.
  *
  * @code
  * const uint8_t heart[] = { 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 1, 0, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 0, 0, 1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, }; // a cute heart
  * MicroBitImage i(10, 5);
  * i.printImage(10,5,heart);
  * @endcode
  *
  * @note all coordinates originate from the top left of an image.
  */
int MicroBitImage::printImage(int16_t width, int16_t height, const uint8_t *bitmap)
{
    const uint8_t *pIn;
    uint8_t *pOut;
    int pixelsToCopyX, pixelsToCopyY;

    // Sanity check.
    if (width <= 0 || height <= 0 || bitmap == NULL)
        return MICROBIT_INVALID_PARAMETER;

    // Calculate sane start pointer.
    pixelsToCopyX = min(width,this->getWidth());
    pixelsToCopyY = min(height,this->getHeight());

    pIn = bitmap;
    pOut = this->getBitmap();

    // Copy the image, stride by stride.
    for (int i=0; i<pixelsToCopyY; i++)
    {
        memcpy(pOut, pIn, pixelsToCopyX);
        pIn += width;
        pOut += this->getWidth();
    }

    return MICROBIT_OK;
}

/**
  * Pastes a given bitmap at the given co-ordinates.
  *
  * Any pixels in the relevant area of this image are replaced.
  *
  * @param image The MicroBitImage to paste.
  *
  * @param x The leftmost X co-ordinate in this image where the given image should be pasted. Defaults to 0.
  *
  * @param y The uppermost Y co-ordinate in this image where the given image should be pasted. Defaults to 0.
  *
  * @param alpha set to 1 if transparency clear pixels in given image should be treated as transparent. Set to 0 otherwise.  Defaults to 0.
  *
  * @return The number of pixels written.
  *
  * @code
  * const uint8_t heart[] = { 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 1, 0, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 0, 0, 1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, }; // a cute heart
  * MicroBitImage i(10,5,heart); // a big heart
  * i.paste(i, -5, 0); // a small heart
  * @endcode
  */
int MicroBitImage::paste(const MicroBitImage &image, int16_t x, int16_t y, uint8_t alpha)
{
    uint8_t *pIn, *pOut;
    int cx, cy;
    int pxWritten = 0;

    // Sanity check.
    // We permit writes that overlap us, but ones that are clearly out of scope we can filter early.
    if (x >= getWidth() || y >= getHeight() || x+image.getWidth() <= 0 || y+image.getHeight() <= 0)
        return 0;

    //Calculate the number of byte we need to copy in each dimension.
    cx = x < 0 ? min(image.getWidth() + x, getWidth()) : min(image.getWidth(), getWidth() - x);
    cy = y < 0 ? min(image.getHeight() + y, getHeight()) : min(image.getHeight(), getHeight() - y);

    // Calculate sane start pointer.
    pIn = image.ptr->data;
    pIn += (x < 0) ? -x : 0;
    pIn += (y < 0) ? -image.getWidth()*y : 0;

    pOut = getBitmap();
    pOut += (x > 0) ? x : 0;
    pOut += (y > 0) ? getWidth()*y : 0;

    // Copy the image, stride by stride
    // If we want primitive transparecy, we do this byte by byte.
    // If we don't, use a more efficient block memory copy instead. Every little helps!

    if (alpha)
    {
        for (int i=0; i<cy; i++)
        {
            for (int j=0; j<cx; j++)
            {
                // Copy this byte if appropriate.
                if (*(pIn+j) != 0){
                    *(pOut+j) = *(pIn+j);
                    pxWritten++;
                }
            }

            pIn += image.getWidth();
            pOut += getWidth();
        }
    }
    else
    {
        for (int i=0; i<cy; i++)
        {
            memcpy(pOut, pIn, cx);

            pxWritten += cx;
            pIn += image.getWidth();
            pOut += getWidth();
        }
    }

    return pxWritten;
}

/**
  * Prints a character to the display at the given location
  *
  * @param c The character to display.
  *
  * @param x The x co-ordinate of on the image to place the top left of the character. Defaults to 0.
  *
  * @param y The y co-ordinate of on the image to place the top left of the character. Defaults to 0.
  *
  * @return MICROBIT_OK on success, or MICROBIT_INVALID_PARAMETER.
  *
  * @code
  * MicroBitImage i(5,5);
  * i.print('a');
  * @endcode
  */
int MicroBitImage::print(char c, int16_t x, int16_t y)
{
    unsigned char v;
    int x1, y1;

    MicroBitFont font = MicroBitFont::getSystemFont();

    // Sanity check. Silently ignore anything out of bounds.
    if (x >= getWidth() || y >= getHeight() || c < MICROBIT_FONT_ASCII_START || c > font.asciiEnd)
        return MICROBIT_INVALID_PARAMETER;

    // Paste.
    int offset = (c-MICROBIT_FONT_ASCII_START) * 5;

    for (int row=0; row<MICROBIT_FONT_HEIGHT; row++)
    {
        v = (char)*(font.characters + offset);

        offset++;

        // Update our Y co-ord write position
        y1 = y+row;

        for (int col = 0; col < MICROBIT_FONT_WIDTH; col++)
        {
            // Update our X co-ord write position
            x1 = x+col;

            if (x1 < getWidth() && y1 < getHeight())
                this->getBitmap()[y1*getWidth()+x1] = (v & (0x10 >> col)) ? 255 : 0;
        }
    }

    return MICROBIT_OK;
}


/**
  * Shifts the pixels in this Image a given number of pixels to the left.
  *
  * @param n The number of pixels to shift.
  *
  * @return MICROBIT_OK on success, or MICROBIT_INVALID_PARAMETER.
  *
  * @code
  * const uint8_t heart[] = { 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 1, 0, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 0, 0, 1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, }; // a cute heart
  * MicroBitImage i(10,5,heart); // a big heart
  * i.shiftLeft(5); // a small heart
  * @endcode
  */
int MicroBitImage::shiftLeft(int16_t n)
{
    uint8_t *p = getBitmap();
    int pixels = getWidth()-n;

    if (n <= 0 )
        return MICROBIT_INVALID_PARAMETER;

    if(n >= getWidth())
    {
        clear();
        return MICROBIT_OK;
    }

    for (int y = 0; y < getHeight(); y++)
    {
        // Copy, and blank fill the rightmost column.
        memcpy(p, p+n, pixels);
        memclr(p+pixels, n);
        p += getWidth();
    }

    return MICROBIT_OK;
}

/**
  * Shifts the pixels in this Image a given number of pixels to the right.
  *
  * @param n The number of pixels to shift.
  *
  * @return MICROBIT_OK on success, or MICROBIT_INVALID_PARAMETER.
  *
  * @code
  * const uint8_t heart[] = { 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 1, 0, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 0, 0, 1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, }; // a cute heart
  * MicroBitImage i(10,5,heart); // a big heart
  * i.shiftLeft(5); // a small heart
  * i.shiftRight(5); // a big heart
  * @endcode
  */
int MicroBitImage::shiftRight(int16_t n)
{
    uint8_t *p = getBitmap();
    int pixels = getWidth()-n;

    if (n <= 0)
        return MICROBIT_INVALID_PARAMETER;

    if(n >= getWidth())
    {
        clear();
        return MICROBIT_OK;
    }

    for (int y = 0; y < getHeight(); y++)
    {
        // Copy, and blank fill the leftmost column.
        memmove(p+n, p, pixels);
        memclr(p, n);
        p += getWidth();
    }

    return MICROBIT_OK;
}


/**
  * Shifts the pixels in this Image a given number of pixels to upward.
  *
  * @param n The number of pixels to shift.
  *
  * @return MICROBIT_OK on success, or MICROBIT_INVALID_PARAMETER.
  *
  * @code
  * const uint8_t heart[] = { 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 1, 0, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 0, 0, 1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, }; // a cute heart
  * MicroBitImage i(10,5,heart);
  * i.shiftUp(1);
  * @endcode
  */
int MicroBitImage::shiftUp(int16_t n)
{
    uint8_t *pOut, *pIn;

    if (n <= 0 )
        return MICROBIT_INVALID_PARAMETER;

    if(n >= getHeight())
    {
        clear();
        return MICROBIT_OK;
    }

    pOut = getBitmap();
    pIn = getBitmap()+getWidth()*n;

    for (int y = 0; y < getHeight(); y++)
    {
        // Copy, and blank fill the leftmost column.
        if (y < getHeight()-n)
            memcpy(pOut, pIn, getWidth());
        else
            memclr(pOut, getWidth());

        pIn += getWidth();
        pOut += getWidth();
    }

    return MICROBIT_OK;
}


/**
  * Shifts the pixels in this Image a given number of pixels to downward.
  *
  * @param n The number of pixels to shift.
  *
  * @return MICROBIT_OK on success, or MICROBIT_INVALID_PARAMETER.
  *
  * @code
  * const uint8_t heart[] = { 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 1, 0, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 0, 0, 1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, }; // a cute heart
  * MicroBitImage i(10,5,heart);
  * i.shiftDown(1);
  * @endcode
  */
int MicroBitImage::shiftDown(int16_t n)
{
    uint8_t *pOut, *pIn;

    if (n <= 0 )
        return MICROBIT_INVALID_PARAMETER;

    if(n >= getHeight())
    {
        clear();
        return MICROBIT_OK;
    }

    pOut = getBitmap() + getWidth()*(getHeight()-1);
    pIn = pOut - getWidth()*n;

    for (int y = 0; y < getHeight(); y++)
    {
        // Copy, and blank fill the leftmost column.
        if (y < getHeight()-n)
            memcpy(pOut, pIn, getWidth());
        else
            memclr(pOut, getWidth());

        pIn -= getWidth();
        pOut -= getWidth();
    }

    return MICROBIT_OK;
}


/**
  * Converts the bitmap to a csv ManagedString.
  *
  * @code
  * const uint8_t heart[] = { 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 1, 0, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 0, 0, 1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, }; // a cute heart
  * MicroBitImage i(10,5,heart);
  * uBit.serial.printString(i.toString()); // "0,1,0,1,0,0,0,0,0,0\n..."
  * @endcode
  */
ManagedString MicroBitImage::toString()
{
    //width including commans and \n * height
    int stringSize = getSize() * 2;

    //plus one for string terminator
    char parseBuffer[stringSize + 1];

    parseBuffer[stringSize] = '\0';

    uint8_t *bitmapPtr = getBitmap();

    int parseIndex = 0;
    int widthCount = 0;

    while (parseIndex < stringSize)
    {
        if(*bitmapPtr)
            parseBuffer[parseIndex] = '1';
        else
            parseBuffer[parseIndex] = '0';

        parseIndex++;

        if(widthCount == getWidth()-1)
        {
            parseBuffer[parseIndex] = '\n';
            widthCount = 0;
        }
        else
        {
            parseBuffer[parseIndex] = ',';
            widthCount++;
        }

        parseIndex++;
        bitmapPtr++;
    }

    return ManagedString(parseBuffer);
}

/**
  * Crops the image to the given dimensions.
  *
  * @param startx the location to start the crop in the x-axis
  *
  * @param starty the location to start the crop in the y-axis
  *
  * @param width the width of the desired cropped region
  *
  * @param height the height of the desired cropped region
  *
  * @code
  * const uint8_t heart[] = { 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 1, 0, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 0, 0, 1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, }; // a cute heart
  * MicroBitImage i(10,5,heart);
  * i.crop(0,0,2,2).toString() // "0,1\n1,1\n"
  * @endcode
  */
MicroBitImage MicroBitImage::crop(int startx, int starty, int cropWidth, int cropHeight)
{
    int newWidth = startx + cropWidth;
    int newHeight = starty + cropHeight;

    if (newWidth >= getWidth() || newWidth <=0)
        newWidth = getWidth();

    if (newHeight >= getHeight() || newHeight <= 0)
        newHeight = getHeight();

    //allocate our storage.
    uint8_t cropped[newWidth * newHeight];

    //calculate the pointer to where we want to begin cropping
    uint8_t *copyPointer = getBitmap() + (getWidth() * starty) + startx;

    //get a reference to our storage
    uint8_t *pastePointer = cropped;

    //go through row by row and select our image.
    for (int i = starty; i < newHeight; i++)
    {
        memcpy(pastePointer, copyPointer, newWidth);

        copyPointer += getWidth();
        pastePointer += newHeight;
    }

    return MicroBitImage(newWidth, newHeight, cropped);
}

/**
  * Check if image is read-only (i.e., residing in flash).
  */
bool MicroBitImage::isReadOnly()
{
    return ptr->isReadOnly();
}

/**
  * Create a copy of the image bitmap. Used particularly, when isReadOnly() is true.
  *
  * @return an instance of MicroBitImage which can be modified independently of the current instance
  */
MicroBitImage MicroBitImage::clone()
{
    return MicroBitImage(getWidth(), getHeight(), getBitmap());
}
