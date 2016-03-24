#ifndef MICROBIT_DISPLAY_H
#define MICROBIT_DISPLAY_H

#include "mbed.h"
#include "MicroBitConfig.h"
#include "ManagedString.h"
#include "MicroBitComponent.h"
#include "MicroBitImage.h"
#include "MicroBitFont.h"
#include "MicroBitMatrixMaps.h"
#include "MicroBitLightSensor.h"

/**
  * Event codes raised by MicroBitDisplay
  */
#define MICROBIT_DISPLAY_EVT_ANIMATION_COMPLETE         1
#define MICROBIT_DISPLAY_EVT_FREE                       2
#define MICROBIT_DISPLAY_EVT_LIGHT_SENSE                4

//
// Internal constants
//

#define MICROBIT_DISPLAY_SPACING                1
#define MICROBIT_DISPLAY_GREYSCALE_BIT_DEPTH    8
#define MICROBIT_DISPLAY_ANIMATE_DEFAULT_POS    -255

enum AnimationMode {
    ANIMATION_MODE_NONE,
    ANIMATION_MODE_STOPPED,
    ANIMATION_MODE_SCROLL_TEXT,
    ANIMATION_MODE_PRINT_TEXT,
    ANIMATION_MODE_SCROLL_IMAGE,
    ANIMATION_MODE_ANIMATE_IMAGE,
    ANIMATION_MODE_PRINT_CHARACTER
};

enum DisplayMode {
    DISPLAY_MODE_BLACK_AND_WHITE,
    DISPLAY_MODE_GREYSCALE,
    DISPLAY_MODE_BLACK_AND_WHITE_LIGHT_SENSE
};

enum DisplayRotation {
    MICROBIT_DISPLAY_ROTATION_0,
    MICROBIT_DISPLAY_ROTATION_90,
    MICROBIT_DISPLAY_ROTATION_180,
    MICROBIT_DISPLAY_ROTATION_270
};

/**
  * Class definition for a MicroBitDisplay.
  *
  * A MicroBitDisplay represents the LED matrix array on the MicroBit device.
  */
class MicroBitDisplay : public MicroBitComponent
{
    uint8_t width;
    uint8_t height;
    uint8_t brightness;
    uint8_t strobeRow;
    uint8_t rotation;
    uint8_t mode;
    uint8_t greyscaleBitMsk;
    uint8_t timingCount;
    uint32_t col_mask;

    Timeout renderTimer;
    PortOut *LEDMatrix;

    //
    // State used by all animation routines.
    //

    // The animation mode that's currently running (if any)
    volatile AnimationMode animationMode;

    // The time in milliseconds between each frame update.
    uint16_t animationDelay;

    // The time in milliseconds since the frame update.
    uint16_t animationTick;

    // Stop playback of any animations
    void stopAnimation(int delay);

    //
    // State for scrollString() method.
    // This is a surprisingly intricate method.
    //
    // The text being displayed.
    ManagedString scrollingText;

    // The index of the character currently being displayed.
    uint16_t scrollingChar;

    // The number of pixels the current character has been shifted on the display.
    uint8_t scrollingPosition;

    //
    // State for printString() method.
    //
    // The text being displayed. NULL if no message is scheduled for playback.
    // We *could* get some reuse in here with the scroll* variables above,
    // but best to keep it clean in case kids try concurrent operation (they will!),
    // given the small RAM overhead needed to maintain orthogonality.
    ManagedString printingText;

    // The index of the character currently being displayed.
    uint16_t printingChar;

    //
    // State for scrollImage() method.
    //
    // The image being displayed.
    MicroBitImage scrollingImage;

    // The number of pixels the image has been shifted on the display.
    int16_t scrollingImagePosition;

    // The number of pixels the image is shifted on the display in each quantum.
    int8_t scrollingImageStride;

    // A pointer to an instance of light sensor, if in use
    MicroBitLightSensor* lightSensor;

    // Flag to indicate if image has been rendered to screen yet (or not)
    bool scrollingImageRendered;

    const MatrixMap &matrixMap;

    // Internal methods to handle animation.

    /**
      *  Periodic callback, that we use to perform any animations we have running.
      */
    void animationUpdate();

    /**
      *  Called by the display in an interval determined by the brightness of the display, to give an impression
      *  of brightness.
      */
    void renderFinish();

    /**
      * Translates a bit mask to a bit mask suitable for the nrf PORT0 and PORT1.
      * Brightness has two levels on, or off.
      */
    void render();

    /**
      * Renders the current image, and drops the fourth frame to allow for
      * sensors that require the display to operate.
      */
    void renderWithLightSense();

    /**
      * Translates a bit mask into a timer interrupt that gives the appearence of greyscale.
      */
    void renderGreyscale();

    /**
      * Internal scrollText update method.
      * Shift the screen image by one pixel to the left. If necessary, paste in the next char.
      */
    void updateScrollText();

    /**
      * Internal printText update method.
      * Paste in the next char in the string.
      */
    void updatePrintText();

    /**
      * Internal scrollImage update method.
      * Paste the stored bitmap at the appropriate point.
      */
    void updateScrollImage();


    /**
      * Internal animateImage update method.
      * Paste the stored bitmap at the appropriate point and stop on the last frame.
      */
    void updateAnimateImage();

    /**
     * Broadcasts an event onto the defult EventModel indicating that the
     * current animation has completed.
     */
    void sendAnimationCompleteEvent();

    /**
      * Blocks the current fiber until the display is available (i.e. not effect is being displayed).
      * Animations are queued until their time to display.
      */
    void waitForFreeDisplay();

    /**
      * Blocks the current fiber until the current animation has finished.
      * If the scheduler is not running, this call will essentially perform a spinning wait.
      */
    void fiberWait();

    /**
      * Enables or disables the display entirely, and releases the pins for other uses.
      *
      * @param enableDisplay true to enabled the display, or false to disable it.
      */
    void setEnable(bool enableDisplay);

public:
    // The mutable bitmap buffer being rendered to the LED matrix.
    MicroBitImage image;

    /**
      * Constructor.
      * Create a representation of a display of a given size.
      * The display is initially blank.
      *
      * @param id The ID display should use when sending events on the MessageBus.
      * @param map The mapping information that relates pin inputs/outputs to physical screen coordinates.
      *
      * Example:
      * @code
      * MicroBitDisplay display(MICROBIT_ID_DISPLAY, microbitMatrixMap),
      * @endcode
      */
    MicroBitDisplay(uint16_t id = MICROBIT_ID_DISPLAY, const MatrixMap &map = microbitMatrixMap);

    /**
      * Stops any currently running animation, and any that are waiting to be displayed.
      */
    void stopAnimation();

    /**
      * Frame update method, invoked periodically to strobe the display.
      */
    virtual void systemTick();

    /**
     * Prints the given character to the display, if it is not in use.
     *
     * @param c The character to display.
     * @param delay Optional parameter - the time for which to show the character. Zero displays the character forever.
     * @return MICROBIT_OK, MICROBIT_BUSY is the screen is in use, or MICROBIT_INVALID_PARAMETER.
     *
     * Example:
     * @code
     * uBit.display.printAsync('p');
     * uBit.display.printAsync('p',100);
     * @endcode
     */
    int printCharAsync(char c, int delay = 0);

    /**
      * Prints the given string to the display, one character at a time.
      * Uses the given delay between characters.
      * Returns immediately, and executes the animation asynchronously.
      *
      * @param s The string to display.
      * @param delay The time to delay between characters, in milliseconds. Must be > 0.
      * @return MICROBIT_OK, or MICROBIT_INVALID_PARAMETER.
      *
      * Example:
      * @code
      * uBit.display.printAsync("abc123",400);
      * @endcode
      */
    int printAsync(ManagedString s, int delay = MICROBIT_DEFAULT_PRINT_SPEED);

    /**
     * Prints the given image to the display, if the display is not in use.
     * Returns immediately, and executes the animation asynchronously.
     *
     * @param i The image to display.
     * @param x The horizontal position on the screen to display the image (default 0)
     * @param y The vertical position on the screen to display the image (default 0)
     * @param alpha Treats the brightness level '0' as transparent (default 0)
     * @param delay The time to delay between characters, in milliseconds. set to 0 to display forever. (default 0).
     *
     * Example:
     * @code
     * MicrobitImage i("1,1,1,1,1\n1,1,1,1,1\n");
     * uBit.display.print(i,400);
     * @endcode
     */
    int printAsync(MicroBitImage i, int x, int y, int alpha, int delay = 0);

    /**
      * Prints the given character to the display.
      *
      * @param c The character to display.
      * @param delay The time to delay between characters, in milliseconds. Must be > 0.
      * @return MICROBIT_OK, MICROBIT_CANCELLED or MICROBIT_INVALID_PARAMETER.
      *
      * Example:
      * @code
      * uBit.display.print('p');
      * @endcode
      */
    int printChar(char c, int delay = 0);

    /**
      * Prints the given string to the display, one character at a time.
      * Uses the given delay between characters.
      * Blocks the calling thread until all the text has been displayed.
      *
      * @param s The string to display.
      * @param delay The time to delay between characters, in milliseconds. Must be > 0.
      * @return MICROBIT_OK, MICROBIT_CANCELLED or MICROBIT_INVALID_PARAMETER.
      *
      * Example:
      * @code
      * uBit.display.print("abc123",400);
      * @endcode
      */
    int print(ManagedString s, int delay = MICROBIT_DEFAULT_PRINT_SPEED);

    /**
      * Prints the given image to the display.
      * Blocks the calling thread until all the text has been displayed.
      *
      * @param i The image to display.
      * @param delay The time to display the image for, or zero to show the image forever. Must be >= 0.
      * @return MICROBIT_OK, MICROBIT_BUSY if the display is already in use, or MICROBIT_INVALID_PARAMETER.
      *
      * Example:
      * @code
      * MicrobitImage i("1,1,1,1,1\n1,1,1,1,1\n");
      * uBit.display.print(i,400);
      * @endcode
      */
    int print(MicroBitImage i, int x, int y, int alpha, int delay = 0);

    /**
      * Scrolls the given string to the display, from right to left.
      * Uses the given delay between characters.
      * Returns immediately, and executes the animation asynchronously.
      *
      * @param s The string to display.
      * @param delay The time to delay between each update to the display, in milliseconds. Must be > 0.
      * @return MICROBIT_OK, MICROBIT_BUSY if the display is already in use, or MICROBIT_INVALID_PARAMETER.
      *
      * Example:
      * @code
      * uBit.display.scrollAsync("abc123",100);
      * @endcode
      */
    int scrollAsync(ManagedString s, int delay = MICROBIT_DEFAULT_SCROLL_SPEED);

    /**
      * Scrolls the given image across the display, from right to left.
      * Returns immediately, and executes the animation asynchronously.
      *
      * @param image The image to display.
      * @param delay The time to delay between each update to the display, in milliseconds. Must be > 0.
      * @param stride The number of pixels to move in each update. Default value is the screen width.
      * @return MICROBIT_OK, MICROBIT_BUSY if the display is already in use, or MICROBIT_INVALID_PARAMETER.
      *
      * Example:
      * @code
      * MicrobitImage i("1,1,1,1,1\n1,1,1,1,1\n");
      * uBit.display.scrollAsync(i,100,1);
      * @endcode
      */
    int scrollAsync(MicroBitImage image, int delay = MICROBIT_DEFAULT_SCROLL_SPEED, int stride = MICROBIT_DEFAULT_SCROLL_STRIDE);

    /**
      * Scrolls the given string to the display, from right to left.
      * Uses the given delay between characters.
      * Blocks the calling thread until all the text has been displayed.
      *
      * @param s The string to display.
      * @param delay The time to delay between each update to the display, in milliseconds. Must be > 0.
      * @return MICROBIT_OK, MICROBIT_CANCELLED or MICROBIT_INVALID_PARAMETER.
      *
      * Example:
      * @code
      * uBit.display.scroll("abc123",100);
      * @endcode
      */
    int scroll(ManagedString s, int delay = MICROBIT_DEFAULT_SCROLL_SPEED);

    /**
      * Scrolls the given image across the display, from right to left.
      * Blocks the calling thread until all the text has been displayed.
      *
      * @param image The image to display.
      * @param delay The time to delay between each update to the display, in milliseconds. Must be > 0.
      * @param stride The number of pixels to move in each update. Default value is the screen width.
      * @return MICROBIT_OK, MICROBIT_CANCELLED or MICROBIT_INVALID_PARAMETER.
      *
      * Example:
      * @code
      * MicrobitImage i("1,1,1,1,1\n1,1,1,1,1\n");
      * uBit.display.scroll(i,100,1);
      * @endcode
      */
    int scroll(MicroBitImage image, int delay = MICROBIT_DEFAULT_SCROLL_SPEED, int stride = MICROBIT_DEFAULT_SCROLL_STRIDE);

    /**
      * "Animates" the current image across the display with a given stride, finishing on the last frame of the animation.
      * Returns immediately.
      *
      * @param image The image to display.
      * @param delay The time to delay between each update to the display, in milliseconds. Must be > 0.
      * @param stride The number of pixels to move in each update. Default value is the screen width.
      * @return MICROBIT_OK, MICROBIT_BUSY if the screen is in use, or MICROBIT_INVALID_PARAMETER.
      *
      * Example:
      * @code
      * const int heart_w = 10;
      * const int heart_h = 5;
      * const uint8_t heart[] = { 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 1, 0, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 0, 0, 1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, };
      *
      * MicroBitImage i(heart_w,heart_h,heart);
      * uBit.display.animateAsync(i,100,5);
      * @endcode
      */
    int animateAsync(MicroBitImage image, int delay, int stride, int startingPosition = MICROBIT_DISPLAY_ANIMATE_DEFAULT_POS);

    /**
      * "Animates" the current image across the display with a given stride, finishing on the last frame of the animation.
      * Blocks the calling thread until the animation is complete.
      *
      * @param image The image to display.
      * @param delay The time to delay between each update to the display, in milliseconds. Must be > 0.
      * @param stride The number of pixels to move in each update. Default value is the screen width.
      * @return MICROBIT_OK, MICROBIT_CANCELLED or MICROBIT_INVALID_PARAMETER.
      *
      * Example:
      * @code
      * const int heart_w = 10;
      * const int heart_h = 5;
      * const uint8_t heart[] = { 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 1, 0, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 0, 0, 1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, };
      *
      * MicroBitImage i(heart_w,heart_h,heart);
      * uBit.display.animate(i,100,5);
      * @endcode
      */
    int animate(MicroBitImage image, int delay, int stride, int startingPosition = MICROBIT_DISPLAY_ANIMATE_DEFAULT_POS);

    /**
      * Sets the display brightness to the specified level.
      * @param b The brightness to set the brightness to, in the range 0..255.
      * @return MICROBIT_OK, or MICROBIT_INVALID_PARAMETER
      *
      * Example:
      * @code
      * uBit.display.setBrightness(255); //max brightness
      * @endcode
      */
    int setBrightness(int b);

    /**
      * Sets the mode of the display.
      * @param mode The mode to swap the display into. (can be either DISPLAY_MODE_GREYSCALE, DISPLAY_MODE_BLACK_AND_WHITE, DISPLAY_MODE_BLACK_AND_WHITE_LIGHT_SENSE)
      *
      * Example:
      * @code
      * uBit.display.setDisplayMode(DISPLAY_MODE_GREYSCALE); //per pixel brightness
      * @endcode
      */
    void setDisplayMode(DisplayMode mode);

    /**
      * Gets the mode of the display.
      * @return the current mode of the display
      */
    int getDisplayMode();

    /**
      * Fetches the current brightness of this display.
      * @return the brightness of this display, in the range 0..255.
      *
      * Example:
      * @code
      * uBit.display.getBrightness(); //the current brightness
      * @endcode
      */
    int getBrightness();

    /**
      * Rotates the display to the given position.
      * Axis aligned values only.
      *
      * Example:
      * @code
      * uBit.display.rotateTo(MICROBIT_DISPLAY_ROTATION_180); //rotates 180 degrees from original orientation
      * @endcode
      */
    void rotateTo(DisplayRotation position);

    /**
      * Enables the display, should only be called if the display is disabled.
      *
      * Example:
      * @code
      * uBit.display.enable(); //reenables the display mechanics
      * @endcode
      */
    void enable();

    /**
      * Disables the display, should only be called if the display is enabled.
      * Display must be disabled to avoid MUXing of edge connector pins.
      *
      * Example:
      * @code
      * uBit.display.disable(); //disables the display
      * @endcode
      */
    void disable();

    /**
      * Clears the current image on the display.
      * Simplifies the process, you can also use uBit.display.image.clear
      *
      * Example:
      * @code
      * uBit.display.clear(); //clears the display
      * @endcode
      */
    void clear();

    /**
      * Updates the font that will be used for display operations.
	  * DEPRECATED: Please use MicroBitFont::setSystemFont() instead.
	  *
      * @param font the new font that will be used to render characters.
      */
    void setFont(MicroBitFont font);

    /**
      * Retreives the font object used for rendering characters on the display.
	  * DEPRECATED: Please use MicroBitFont::getSystemFont() instead.
      */
    MicroBitFont getFont();

    /**
      * Captures the bitmap currently being rendered on the display.
      */
    MicroBitImage screenShot();

    /**
      * Constructs an instance of a MicroBitLightSensor if not already configured
      * and sets the display mode to DISPLAY_MODE_BLACK_AND_WHITE_LIGHT_SENSE.
      *
      * This also changes the tickPeriod to MICROBIT_LIGHT_SENSOR_TICK_SPEED so
      * that the display does not suffer from artifacts.
      *
      * @note this will return 0 on the first call to this method, a light reading
      * will be available after the display has activated the light sensor for the
      * first time.
      */
    int readLightLevel();

    /**
      * Destructor for MicroBitDisplay, so that we deregister ourselves as a systemComponent
      */
    ~MicroBitDisplay();
};

#endif
