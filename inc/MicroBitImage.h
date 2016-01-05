#ifndef MICROBIT_IMAGE_H
#define MICROBIT_IMAGE_H

#include "mbed.h"
#include "RefCounted.h"

struct ImageData : RefCounted
{
    uint16_t width;     // Width in pixels
    uint16_t height;    // Height in pixels
    uint8_t data[0];    // 2D array representing the bitmap image
};

/**
  * Class definition for a MicroBitImage.
  *
  * An MicroBitImage is a simple bitmap representation of an image.
  * n.b. This is a mutable, managed type.
  */
class MicroBitImage
{
    ImageData *ptr;     // Pointer to payload data
    
    
    /**
      * Internal constructor which provides sanity checking and initialises class properties.
      *
      * @param x the width of the image
      * @param y the height of the image
      * @param bitmap an array of integers that make up an image.
      */
    void init(const int16_t x, const int16_t y, const uint8_t *bitmap);
    
    /**
      * Internal constructor which defaults to the Empty Image instance variable
      */
    void init_empty();
    
    public:
    static MicroBitImage EmptyImage;    // Shared representation of a null image.

    /**
      * Get current ptr, do not decr() it, and set the current instance to empty image.
      * This is to be used by specialized runtimes which pass ImageData around.
      */
    ImageData *leakData();

    /**
      * Return a 2D array representing the bitmap image.
      */
    uint8_t *getBitmap()
    {
        return ptr->data;
    }
    
    /**
      * Constructor. 
      * Create an image from a specially prepared constant array, with no copying. Will call ptr->incr().
      *
      * @param ptr The literal - first two bytes should be 0xff, then width, 0, height, 0, and the bitmap. Width and height are 16 bit. The literal has to be 4-byte aligned.
      * 
      * Example:
      * @code 
      * static const uint8_t heart[] __attribute__ ((aligned (4))) = { 0xff, 0xff, 10, 0, 5, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 1, 0, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 0, 0, 1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, }; // a cute heart
      * MicroBitImage i((ImageData*)(void*)heart);
      * @endcode
      */
    MicroBitImage(ImageData *ptr);
    
    /**
      * Default Constructor. 
      * Creates a new reference to the empty MicroBitImage bitmap 
      *
      * Example:
      * @code
      * MicroBitImage i(); //an empty image
      * @endcode
      */
    MicroBitImage();
    
    
    /**
      * Copy Constructor. 
      * Add ourselves as a reference to an existing MicroBitImage.
      * 
      * @param image The MicroBitImage to reference.
      *
      * Example:
      * @code
      * MicroBitImage i("0,1,0,1,0\n");
      * MicroBitImage i2(i); //points to i
      * @endcode
      */
    MicroBitImage(const MicroBitImage &image);
    
    /**
      * Constructor. 
      * Create a blank bitmap representation of a given size.
      * 
      * @param s A text based representation of the image given whitespace delimited numeric values.
      *
      * Example:
      * @code
      * MicroBitImage i("0,1,0,1,0\n1,0,1,0,1\n0,1,0,1,0\n1,0,1,0,1\n0,1,0,1,0\n"); // 5x5 image
      * @endcode
      */
    explicit MicroBitImage(const char *s);

    /**
      * Constructor. 
      * Create a blank bitmap representation of a given size.
      * 
      * @param x the width of the image.
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
      * Example:
      * @code
      * MicroBitImage i(5,5); // a blank 5x5 image 
      * @endcode    
      */
    MicroBitImage(const int16_t x, const int16_t y);

    /**
      * Constructor. 
      * Create a bitmap representation of a given size, based on a given buffer.
      * 
      * @param x the width of the image.
      * @param y the height of the image.
      * @param bitmap a 2D array representing the image.
      * 
      * Example:
      * @code
      * const uint8_t heart[] = { 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 1, 0, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 0, 0, 1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, }; // a cute heart
      * MicroBitImage i(10,5,heart); 
      * @endcode     
      */
    MicroBitImage(const int16_t x, const int16_t y, const uint8_t *bitmap);

    /**
      * Destructor. 
      * Removes buffer resources held by the instance.
      */
    ~MicroBitImage();

    /**
      * Copy assign operation. 
      *
      * Called when one MicroBitImage is assigned the value of another using the '=' operator.
      * Decrement our reference count and free up the buffer as necessary.
      * Then, update our buffer to refer to that of the supplied MicroBitImage,
      * and increase its reference count.
      *
      * @param s The MicroBitImage to reference.
      * 
      * Example:
      * @code
      * const uint8_t heart[] = { 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 1, 0, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 0, 0, 1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, }; // a cute heart
      * MicroBitImage i(10,5,heart); 
      * MicroBitImage i1();
      * i1 = 1; // i1 now references i 
      * @endcode
      */
    MicroBitImage& operator = (const MicroBitImage& i);


    /**
      * Equality operation.
      *
      * Called when one MicroBitImage is tested to be equal to another using the '==' operator.
      *
      * @param i The MicroBitImage to test ourselves against.
      * @return true if this MicroBitImage is identical to the one supplied, false otherwise.
      * 
      * Example:
      * @code
      * MicroBitImage i(); 
      * MicroBitImage i1();
      *
      * if(i == i1) //will be true
      *     print("true"); 
      * @endcode
      */
    bool operator== (const MicroBitImage& i);
    
    /**
      * Clears all pixels in this image
      *
      * Example:
      * @code
      * MicroBitImage i("0,1,0,1,0\n1,0,1,0,1\n0,1,0,1,0\n1,0,1,0,1\n0,1,0,1,0\n"); // 5x5 image
      * i.clear();
      * @endcode
      */
    void clear();

    /**
      * Sets the pixel at the given co-ordinates to a given value.
      * @param x The co-ordinate of the pixel to change w.r.t. top left origin.
      * @param y The co-ordinate of the pixel to change w.r.t. top left origin.
      * @param value The new value of the pixel (the brightness level 0-255)
      * @return MICROBIT_OK, or MICROBIT_INVALID_PARAMETER.
      *
      * Example:
      * @code
      * MicroBitImage i("0,1,0,1,0\n1,0,1,0,1\n0,1,0,1,0\n1,0,1,0,1\n0,1,0,1,0\n"); // 5x5 image
      * i.setPixelValue(0,0,255);
      * @endcode
      */
    int setPixelValue(int16_t x , int16_t y, uint8_t value);

    /**
      * Determines the value of a given pixel. 
      *
      * @param x The x co-ordinate of the pixel to read. Must be within the dimensions of the image.
      * @param y The y co-ordinate of the pixel to read. Must be within the dimensions of the image.
      * @return The value assigned to the given pixel location (the brightness level 0-255), or MICROBIT_INVALID_PARAMETER.
      *
      * Example:
      * @code
      * MicroBitImage i("0,1,0,1,0\n1,0,1,0,1\n0,1,0,1,0\n1,0,1,0,1\n0,1,0,1,0\n"); // 5x5 image
      * i.getPixelValue(0,0); //should be 0;
      * @endcode
      */
    int getPixelValue(int16_t x , int16_t y);

    /**
      * Replaces the content of this image with that of a given 
      * 2D array representing the image.
      * Origin is in the top left corner of the image.
      *
      * @param x the width of the image. Must be within the dimensions of the image.
      * @param y the width of the image. Must be within the dimensions of the image.
      * @param bitmap a 2D array representing the image.
      * @return MICROBIT_OK on success, or MICROBIT_INVALID_PARAMETER.
      * 
      * Example:
      * @code
      * const uint8_t heart[] = { 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 1, 0, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 0, 0, 1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, }; // a cute heart
      * MicroBitImage i(); 
      * i.printImage(0,0,heart); 
      * @endcode
      */
    int printImage(int16_t x, int16_t y, const uint8_t *bitmap);
    
    /**
      * Pastes a given bitmap at the given co-ordinates.
      * Any pixels in the relvant area of this image are replaced.
      * 
      * @param image The MicroBitImage to paste.
      * @param x The leftmost X co-ordinate in this image where the given image should be pasted.
      * @param y The uppermost Y co-ordinate in this image where the given image should be pasted.
      * @param alpha set to 1 if transparency clear pixels in given image should be treated as transparent. Set to 0 otherwise.
      * @return The number of pixels written, or MICROBIT_INVALID_PARAMETER.
      * 
      * Example:
      * @code
      * const uint8_t heart[] = { 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 1, 0, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 0, 0, 1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, }; // a cute heart
      * MicroBitImage i(10,5,heart); //if you show this image - you will see a big heart
      * i.paste(-5,0,i); //displays a small heart :) 
      * @endcode
      */
    int paste(const MicroBitImage &image, int16_t x, int16_t y, uint8_t alpha);
 
     /**
      * Prints a character to the display at the given location
      *
      * @param c The character to display.
      * @param x The x co-ordinate of on the image to place the top left of the character
      * @param y The y co-ordinate of on the image to place the top left of the character
      * @return MICROBIT_OK on success, or MICROBIT_INVALID_PARAMETER.
      * 
      * Example:
      * @code
      * MicroBitImage i(5,5); 
      * i.print('a',0,0);
      * @endcode
      */
    int print(char c, int16_t x, int16_t y);
 
    /**
      * Shifts the pixels in this Image a given number of pixels to the Left.
      *
      * @param n The number of pixels to shift.
      * @return MICROBIT_OK on success, or MICROBIT_INVALID_PARAMETER.
      * 
      * Example:
      * @code
      * const uint8_t heart[] = { 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 1, 0, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 0, 0, 1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, }; // a cute heart
      * MicroBitImage i(10,5,heart); //if you show this image - you will see a big heart
      * i.shiftLeft(5); //displays a small heart :) 
      * @endcode
      */
    int shiftLeft(int16_t n);

    /**
      * Shifts the pixels in this Image a given number of pixels to the Right.
      *
      * @param n The number of pixels to shift.
      * @return MICROBIT_OK on success, or MICROBIT_INVALID_PARAMETER.
      * 
      * Example:
      * @code
      * const uint8_t heart[] = { 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 1, 0, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 0, 0, 1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, }; // a cute heart
      * MicroBitImage i(10,5,heart);
      * i.shiftLeft(5); //displays a small heart :) 
      * i.shiftRight(5); //displays a big heart :) 
      * @endcode
      */
    int shiftRight(int16_t n);
    
    /**
      * Shifts the pixels in this Image a given number of pixels to Upward.
      *
      * @param n The number of pixels to shift.
      * @return MICROBIT_OK on success, or MICROBIT_INVALID_PARAMETER.
      * 
      * Example:
      * @code
      * const uint8_t heart[] = { 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 1, 0, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 0, 0, 1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, }; // a cute heart
      * MicroBitImage i(10,5,heart);
      * i.shiftUp(1);
      * @endcode
      */
    int shiftUp(int16_t n);
    
    /**
      * Shifts the pixels in this Image a given number of pixels to Downward.
      *
      * @param n The number of pixels to shift.
      * @return MICROBIT_OK on success, or MICROBIT_INVALID_PARAMETER.
      * 
      * Example:
      * @code
      * const uint8_t heart[] = { 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 1, 0, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 0, 0, 1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, }; // a cute heart
      * MicroBitImage i(10,5,heart);
      * i.shiftDown(1);
      * @endcode
      */
    int shiftDown(int16_t n);

    /**
      * Gets the width of this image.
      *
      * @return The width of this image.
      * 
      * Example:
      * @code
      * const uint8_t heart[] = { 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 1, 0, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 0, 0, 1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, }; // a cute heart
      * MicroBitImage i(10,5,heart);
      * i.getWidth(); //equals 10...
      * @endcode
      */
    int getWidth() const
    {
        return ptr->width;
    }

    /**
      * Gets the height of this image.
      *
      * @return The height of this image.
      * 
      * Example:
      * @code
      * const uint8_t heart[] = { 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 1, 0, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 0, 0, 1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, }; // a cute heart
      * MicroBitImage i(10,5,heart);
      * i.getHeight(); //equals 5...
      * @endcode
      */
    int getHeight() const
    {
        return ptr->height;
    }
    
    /**
      * Gets number of bytes in the bitmap, ie., width * height.
      *
      * @return The size of the bitmap.
      * 
      * Example:
      * @code
      * const uint8_t heart[] = { 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 1, 0, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 0, 0, 1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, }; // a cute heart
      * MicroBitImage i(10,5,heart);
      * i.getSize(); //equals 50...
      * @endcode
      */
    int getSize() const
    {
        return ptr->width * ptr->height;
    }

    /**
      * Converts the bitmap to a csv string.
      *
      * Example:
      * @code
      * const uint8_t heart[] = { 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 1, 0, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 0, 0, 1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, }; // a cute heart
      * MicroBitImage i(10,5,heart);
      * uBit.serial.printString(i.toString()); // "0,1,0,1,0,0,0,0,0,0\n..."
      * @endcode
      */
    ManagedString toString();

    /**
      * Crops the image to the given dimensions
      *
      * @param startx the location to start the crop in the x-axis
      * @param starty the location to start the crop in the y-axis
      * @param width the width of the desired cropped region
      * @param height the height of the desired cropped region
      *
      * @return an instance of MicroBitImage with your cropped region
      *
      * Example:
      * @code
      * const uint8_t heart[] = { 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 1, 0, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 0, 0, 1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, }; // a cute heart
      * MicroBitImage i(10,5,heart);
      * uBit.serial.printImage(i.crop(0,0,2,2)); // "0,1\n1,1\n"
      * @endcode
      */
    MicroBitImage crop(int startx, int starty, int finx, int finy);

    /**
      * Check if image is read-only (i.e., residing in flash).
      */
    bool isReadOnly();

    /**
      * Create a copy of the image bitmap. Used particularly, when isReadOnly() is true.
      *
      * @return an instance of MicroBitImage which can be modified independently of the current instance
      */
    MicroBitImage clone();
};

#endif

