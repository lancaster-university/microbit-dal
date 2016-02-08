/**
  * Class definition for a MicroBitDisplay.
  *
  * A MicroBitDisplay represents the LED matrix array on the MicroBit device.
  */
#include "mbed.h"
#include "MicroBit.h"
#include "MicroBitMatrixMaps.h"
#include "nrf_gpio.h"

const int timings[MICROBIT_DISPLAY_GREYSCALE_BIT_DEPTH] = {1, 23, 70, 163, 351, 726, 1476, 2976};

/**
  * Constructor.
  * Create a representation of a display of a given size.
  * The display is initially blank.
  *
  * @param x the width of the display in pixels.
  * @param y the height of the display in pixels.
  *
  * Example:
  * @code
  * MicroBitDisplay display(MICROBIT_ID_DISPLAY, 5, 5),
  * @endcode
  */
MicroBitDisplay::MicroBitDisplay(uint16_t id, uint8_t x, uint8_t y) :
    font(),
    image(x*2,y)
{
    //set pins as output
    nrf_gpio_range_cfg_output(MICROBIT_DISPLAY_COLUMN_START,MICROBIT_DISPLAY_COLUMN_START + MICROBIT_DISPLAY_COLUMN_COUNT + MICROBIT_DISPLAY_ROW_COUNT);

    this->id = id;
    this->width = x;
    this->height = y;
    this->strobeRow = 0;
    this->strobeBitMsk = MICROBIT_DISPLAY_ROW_RESET;
    this->rotation = MICROBIT_DISPLAY_ROTATION_0;
    this->greyscaleBitMsk = 0x01;
    this->timingCount = 0;
    this->errorTimeout = 0;

    this->setBrightness(MICROBIT_DISPLAY_DEFAULT_BRIGHTNESS);

    this->mode = DISPLAY_MODE_BLACK_AND_WHITE;
    this->animationMode = ANIMATION_MODE_NONE;

    this->lightSensor = NULL;

    uBit.flags |= MICROBIT_FLAG_DISPLAY_RUNNING;
}

/**
  * Internal frame update method, used to strobe the display.
  *
  * TODO: Write a more efficient, complementary variation of this method for the case where
  * MICROBIT_DISPLAY_ROW_COUNT > MICROBIT_DISPLAY_COLUMN_COUNT.
  */
void MicroBitDisplay::systemTick()
{
    if(!(uBit.flags & MICROBIT_FLAG_DISPLAY_RUNNING))
        return;

    if(mode == DISPLAY_MODE_BLACK_AND_WHITE_LIGHT_SENSE)
    {
        renderWithLightSense();
        return;
    }

    // Move on to the next row.
    strobeBitMsk <<= 1;
    strobeRow++;

    //reset the row counts and bit mask when we have hit the max.
    if(strobeRow == MICROBIT_DISPLAY_ROW_COUNT){
        strobeRow = 0;
        strobeBitMsk = MICROBIT_DISPLAY_ROW_RESET;
    }

    if(mode == DISPLAY_MODE_BLACK_AND_WHITE)
        render();

    if(mode == DISPLAY_MODE_GREYSCALE)
    {
        greyscaleBitMsk = 0x01;
        timingCount = 0;
        renderGreyscale();
    }

    // Update text and image animations if we need to.
    this->animationUpdate();
}

void MicroBitDisplay::renderFinish()
{
    //kept inline to reduce overhead
    //clear the old bit pattern for this row.
    //clear port 0 4-7 and retain lower 4 bits
    nrf_gpio_port_write(NRF_GPIO_PORT_SELECT_PORT0, 0xF0 | (nrf_gpio_port_read(NRF_GPIO_PORT_SELECT_PORT0) & 0x0F));

    // clear port 1 8-12 for the current row
    nrf_gpio_port_write(NRF_GPIO_PORT_SELECT_PORT1, strobeBitMsk | 0x1F);
}

void MicroBitDisplay::render()
{
    // Simple optimisation. If display is at zero brightness, there's nothign to do.
    if(brightness == 0)
        return;

    int coldata = 0;

    // Calculate the bitpattern to write.
    for (int i = 0; i<MICROBIT_DISPLAY_COLUMN_COUNT; i++)
    {
        int x = matrixMap[i][strobeRow].x;
        int y = matrixMap[i][strobeRow].y;
        int t = x;

        if(rotation == MICROBIT_DISPLAY_ROTATION_90)
        {
                x = width - 1 - y;
                y = t;
        }

        if(rotation == MICROBIT_DISPLAY_ROTATION_180)
        {
                x = width - 1 - x;
                y = height - 1 - y;
        }

        if(rotation == MICROBIT_DISPLAY_ROTATION_270)
        {
                x = y;
                y = height - 1 - t;
        }

        if(image.getBitmap()[y*(width*2)+x])
            coldata |= (1 << i);
    }

    //write the new bit pattern
    //set port 0 4-7 and retain lower 4 bits
    nrf_gpio_port_write(NRF_GPIO_PORT_SELECT_PORT0, (~coldata<<4 & 0xF0) | (nrf_gpio_port_read(NRF_GPIO_PORT_SELECT_PORT0) & 0x0F));

    //set port 1 8-12 for the current row
    nrf_gpio_port_write(NRF_GPIO_PORT_SELECT_PORT1, strobeBitMsk | (~coldata>>4 & 0x1F));

    //timer does not have enough resolution for brightness of 1. 23.53 us
    if(brightness != MICROBIT_DISPLAY_MAXIMUM_BRIGHTNESS && brightness > MICROBIT_DISPLAY_MINIMUM_BRIGHTNESS)
        renderTimer.attach_us(this, &MicroBitDisplay::renderFinish, (((brightness * 950) / (MICROBIT_DISPLAY_MAXIMUM_BRIGHTNESS)) * uBit.getTickPeriod()));

    //this will take around 23us to execute
    if(brightness <= MICROBIT_DISPLAY_MINIMUM_BRIGHTNESS)
        renderFinish();
}

void MicroBitDisplay::renderWithLightSense()
{
    //reset the row counts and bit mask when we have hit the max.
    if(strobeRow == MICROBIT_DISPLAY_ROW_COUNT + 1)
    {

        MicroBitEvent(id, MICROBIT_DISPLAY_EVT_LIGHT_SENSE);

        strobeRow = 0;
        strobeBitMsk = MICROBIT_DISPLAY_ROW_RESET;
    }
    else
    {

        render();
        this->animationUpdate();

        // Move on to the next row.
        strobeBitMsk <<= 1;
        strobeRow++;
    }

}

void MicroBitDisplay::renderGreyscale()
{
    int coldata = 0;

    // Calculate the bitpattern to write.
    for (int i = 0; i<MICROBIT_DISPLAY_COLUMN_COUNT; i++)
    {
        int x = matrixMap[i][strobeRow].x;
        int y = matrixMap[i][strobeRow].y;
        int t = x;

        if(rotation == MICROBIT_DISPLAY_ROTATION_90)
        {
                x = width - 1 - y;
                y = t;
        }

        if(rotation == MICROBIT_DISPLAY_ROTATION_180)
        {
                x = width - 1 - x;
                y = height - 1 - y;
        }

        if(rotation == MICROBIT_DISPLAY_ROTATION_270)
        {
                x = y;
                y = height - 1 - t;
        }

        if(min(image.getBitmap()[y * (width * 2) + x],brightness) & greyscaleBitMsk)
            coldata |= (1 << i);
    }
    //write the new bit pattern
    //set port 0 4-7 and retain lower 4 bits
    nrf_gpio_port_write(NRF_GPIO_PORT_SELECT_PORT0, (~coldata<<4 & 0xF0) | (nrf_gpio_port_read(NRF_GPIO_PORT_SELECT_PORT0) & 0x0F));

    //set port 1 8-12 for the current row
    nrf_gpio_port_write(NRF_GPIO_PORT_SELECT_PORT1, strobeBitMsk | (~coldata>>4 & 0x1F));

    if(timingCount > MICROBIT_DISPLAY_GREYSCALE_BIT_DEPTH-1)
        return;

    greyscaleBitMsk <<= 1;

    if(timingCount < 3)
    {
        wait_us(timings[timingCount++]);
        renderGreyscale();
        return;
    }
    renderTimer.attach_us(this,&MicroBitDisplay::renderGreyscale, timings[timingCount++]);
}

/**
  * Periodic callback, that we use to perform any animations we have running.
  */
void
MicroBitDisplay::animationUpdate()
{
    // If there's no ongoing animation, then nothing to do.
    if (animationMode == ANIMATION_MODE_NONE)
        return;

    animationTick += FIBER_TICK_PERIOD_MS;

    if(animationTick >= animationDelay)
    {
        animationTick = 0;

        if (animationMode == ANIMATION_MODE_SCROLL_TEXT)
            this->updateScrollText();

        if (animationMode == ANIMATION_MODE_PRINT_TEXT)
            this->updatePrintText();

        if (animationMode == ANIMATION_MODE_SCROLL_IMAGE)
            this->updateScrollImage();

        if (animationMode == ANIMATION_MODE_ANIMATE_IMAGE)
            this->updateAnimateImage();

        if(animationMode == ANIMATION_MODE_PRINT_CHARACTER)
        {
            animationMode = ANIMATION_MODE_NONE;
            this->sendAnimationCompleteEvent();
        }
    }
}

/**
  * Broadcasts an event onto the shared MessageBus
  * @param eventCode The ID of the event that has occurred.
  */
void MicroBitDisplay::sendAnimationCompleteEvent()
{
    // Signal that we've completed an animation.
    MicroBitEvent(id,MICROBIT_DISPLAY_EVT_ANIMATION_COMPLETE);

    // Wake up a fiber that was blocked on the animation (if any).
    MicroBitEvent(MICROBIT_ID_NOTIFY_ONE, MICROBIT_DISPLAY_EVT_FREE);
}

/**
  * Internal scrollText update method.
  * Shift the screen image by one pixel to the left. If necessary, paste in the next char.
  */
void MicroBitDisplay::updateScrollText()
{
    image.shiftLeft(1);
    scrollingPosition++;

    if (scrollingPosition == width + MICROBIT_DISPLAY_SPACING)
    {
        scrollingPosition = 0;

        image.print(scrollingChar < scrollingText.length() ? scrollingText.charAt(scrollingChar) : ' ',width,0);

        if (scrollingChar > scrollingText.length())
        {
            animationMode = ANIMATION_MODE_NONE;
            this->sendAnimationCompleteEvent();
            return;
        }
        scrollingChar++;
   }
}

/**
  * Internal printText update method.
  * Paste in the next char in the string.
  */
void MicroBitDisplay::updatePrintText()
{
    image.print(printingChar < printingText.length() ? printingText.charAt(printingChar) : ' ',0,0);

    if (printingChar > printingText.length())
    {
        animationMode = ANIMATION_MODE_NONE;

        this->sendAnimationCompleteEvent();
        return;
    }

    printingChar++;
}

/**
  * Internal scrollImage update method.
  * Paste the stored bitmap at the appropriate point.
  */
void MicroBitDisplay::updateScrollImage()
{
    image.clear();

    if (((image.paste(scrollingImage, scrollingImagePosition, 0, 0) == 0) && scrollingImageRendered) || scrollingImageStride == 0)
    {
        animationMode = ANIMATION_MODE_NONE;
        this->sendAnimationCompleteEvent();

        return;
    }

    scrollingImagePosition += scrollingImageStride;
    scrollingImageRendered = true;
}

/**
  * Internal animateImage update method.
  * Paste the stored bitmap at the appropriate point and stop on the last frame.
  */
void MicroBitDisplay::updateAnimateImage()
{
    //wait until we have rendered the last position to give a continuous animation.
    if (scrollingImagePosition <= -scrollingImage.getWidth() + (MICROBIT_DISPLAY_WIDTH + scrollingImageStride) && scrollingImageRendered)
    {
        animationMode = ANIMATION_MODE_NONE;
        this->clear();
        this->sendAnimationCompleteEvent();
        return;
    }

    if(scrollingImagePosition > 0)
        image.shiftLeft(-scrollingImageStride);

    image.paste(scrollingImage, scrollingImagePosition, 0, 0);

    if(scrollingImageStride == 0)
    {
        animationMode = ANIMATION_MODE_NONE;
        this->sendAnimationCompleteEvent();
    }

    scrollingImageRendered = true;

    scrollingImagePosition += scrollingImageStride;
}

/**
  * Resets the current given animation.
  */
void MicroBitDisplay::stopAnimation()
{
    // Reset any ongoing animation.
    if (animationMode != ANIMATION_MODE_NONE)
    {
        animationMode = ANIMATION_MODE_NONE;

        // Indicate that we've completed an animation.
        MicroBitEvent(id,MICROBIT_DISPLAY_EVT_ANIMATION_COMPLETE);

        // Wake up aall fibers that may blocked on the animation (if any).
        MicroBitEvent(MICROBIT_ID_NOTIFY, MICROBIT_DISPLAY_EVT_FREE);
    }

    // Clear the display and setup the animation timers.
    this->image.clear();
}

/**
  * Blocks the current fiber until the display is available (i.e. not effect is being displayed).
  * Animations are queued until their time to display.
  *
  */
void MicroBitDisplay::waitForFreeDisplay()
{
    // If there's an ongoing animation, wait for our turn to display.
    if (animationMode != ANIMATION_MODE_NONE && animationMode != ANIMATION_MODE_STOPPED)
        fiber_wait_for_event(MICROBIT_ID_NOTIFY, MICROBIT_DISPLAY_EVT_FREE);
}


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
int MicroBitDisplay::printAsync(char c, int delay)
{
    //sanitise this value
    if(delay < 0)
        return MICROBIT_INVALID_PARAMETER;

    // If the display is free, it's our turn to display.
    if (animationMode == ANIMATION_MODE_NONE || animationMode == ANIMATION_MODE_STOPPED)
    {
        image.print(c, 0, 0);

        if (delay > 0)
        {
            animationDelay = delay;
            animationTick = 0;
            animationMode = ANIMATION_MODE_PRINT_CHARACTER;
        }
    }
    else
    {
        return MICROBIT_BUSY;
    }

    return MICROBIT_OK;
}

/**
  * Prints the given string to the display, one character at a time, if the display is not in use.
  * Returns immediately, and executes the animation asynchronously.
  *
  * @param s The string to display.
  * @param delay The time to delay between characters, in milliseconds. Must be > 0.
  * @return MICROBIT_OK, MICROBIT_BUSY if the display is already in use, or MICROBIT_INVALID_PARAMETER.
  *
  * Example:
  * @code
  * uBit.display.printAsync("abc123",400);
  * @endcode
  */
int MicroBitDisplay::printAsync(ManagedString s, int delay)
{
    //sanitise this value
    if(delay <= 0 )
        return MICROBIT_INVALID_PARAMETER;

    if (animationMode == ANIMATION_MODE_NONE || animationMode == ANIMATION_MODE_STOPPED)
    {
        printingChar = 0;
        printingText = s;
        animationDelay = delay;
        animationTick = 0;

        animationMode = ANIMATION_MODE_PRINT_TEXT;
    }
    else
    {
        return MICROBIT_BUSY;
    }

    return MICROBIT_OK;

}

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
int MicroBitDisplay::printAsync(MicroBitImage i, int x, int y, int alpha, int delay)
{
    if(delay < 0)
        return MICROBIT_INVALID_PARAMETER;

    if (animationMode == ANIMATION_MODE_NONE || animationMode == ANIMATION_MODE_STOPPED)
    {
        image.paste(i, x, y, alpha);

        if(delay > 0)
        {
            animationDelay = delay;
            animationTick = 0;
            animationMode = ANIMATION_MODE_PRINT_CHARACTER;
        }
    }
    else
    {
        return MICROBIT_BUSY;
    }

    return MICROBIT_OK;
}

/**
  * Prints the given character to the display, and wait for it to complete.
  *
  * @param c The character to display.
  * @param delay The time to delay between characters, in milliseconds. Must be > 0.
  * @return MICROBIT_OK, MICROBIT_CANCELLED or MICROBIT_INVALID_PARAMETER.
  *
  * Example:
  * @code
  * uBit.display.print('p');
  * uBit.display.print('p',100);
  * @endcode
  */
int MicroBitDisplay::print(char c, int delay)
{
    if (delay < 0)
        return MICROBIT_INVALID_PARAMETER;

    // If there's an ongoing animation, wait for our turn to display.
    this->waitForFreeDisplay();

    // If the display is free, it's our turn to display.
    // If someone called stopAnimation(), then we simply skip...
    if (animationMode == ANIMATION_MODE_NONE)
    {
        this->printAsync(c, delay);
        if (delay > 0)
            fiber_wait_for_event(MICROBIT_ID_DISPLAY, MICROBIT_DISPLAY_EVT_ANIMATION_COMPLETE);
    }
    else
    {
        return MICROBIT_CANCELLED;
    }

    return MICROBIT_OK;
}

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
int MicroBitDisplay::print(ManagedString s, int delay)
{
    //sanitise this value
    if(delay <= 0 )
        return MICROBIT_INVALID_PARAMETER;

    // If there's an ongoing animation, wait for our turn to display.
    this->waitForFreeDisplay();

    // If the display is free, it's our turn to display.
    // If someone called stopAnimation(), then we simply skip...
    if (animationMode == ANIMATION_MODE_NONE)
    {
        this->printAsync(s, delay);
        fiber_wait_for_event(MICROBIT_ID_DISPLAY, MICROBIT_DISPLAY_EVT_ANIMATION_COMPLETE);
    }
    else
    {
        return MICROBIT_CANCELLED;
    }

    return MICROBIT_OK;
}

/**
  * Prints the given image to the display.
  * Blocks the calling thread until all the text has been displayed.
  *
  * @param i The image to display.
  * @param delay The time to display the image for, or zero to show the image forever. Must be >= 0.
  * @return MICROBIT_OK, MICROBIT_CANCELLED or MICROBIT_INVALID_PARAMETER.
  *
  * Example:
  * @code
  * MicrobitImage i("1,1,1,1,1\n1,1,1,1,1\n");
  * uBit.display.print(i,400);
  * @endcode
  */
int MicroBitDisplay::print(MicroBitImage i, int x, int y, int alpha, int delay)
{
    if(delay < 0)
        return MICROBIT_INVALID_PARAMETER;

    // If there's an ongoing animation, wait for our turn to display.
    this->waitForFreeDisplay();

    // If the display is free, it's our turn to display.
    // If someone called stopAnimation(), then we simply skip...
    if (animationMode == ANIMATION_MODE_NONE)
    {
        this->printAsync(i, x, y, alpha, delay);
        if (delay > 0)
            fiber_wait_for_event(MICROBIT_ID_DISPLAY, MICROBIT_DISPLAY_EVT_ANIMATION_COMPLETE);
    }
    else
    {
        return MICROBIT_CANCELLED;
    }

    return MICROBIT_OK;
}

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
int MicroBitDisplay::scrollAsync(ManagedString s, int delay)
{
    //sanitise this value
    if(delay <= 0)
        return MICROBIT_INVALID_PARAMETER;

    // If the display is free, it's our turn to display.
    if (animationMode == ANIMATION_MODE_NONE || animationMode == ANIMATION_MODE_STOPPED)
    {
        scrollingPosition = width-1;
        scrollingChar = 0;
        scrollingText = s;

        animationDelay = delay;
        animationTick = 0;
        animationMode = ANIMATION_MODE_SCROLL_TEXT;
    }
    else
    {
        return MICROBIT_BUSY;
    }

    return MICROBIT_OK;
}

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
int MicroBitDisplay::scrollAsync(MicroBitImage image, int delay, int stride)
{
    //sanitise the delay value
    if(delay <= 0)
        return MICROBIT_INVALID_PARAMETER;

    // If the display is free, it's our turn to display.
    if (animationMode == ANIMATION_MODE_NONE || animationMode == ANIMATION_MODE_STOPPED)
    {
        scrollingImagePosition = stride < 0 ? width : -image.getWidth();
        scrollingImageStride = stride;
        scrollingImage = image;
        scrollingImageRendered = false;

        animationDelay = stride == 0 ? 0 : delay;
        animationTick = 0;
        animationMode = ANIMATION_MODE_SCROLL_IMAGE;
    }
    else
    {
        return MICROBIT_BUSY;
    }

    return MICROBIT_OK;
}

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
  * uBit.display.scrollString("abc123",100);
  * @endcode
  */
int MicroBitDisplay::scroll(ManagedString s, int delay)
{
    //sanitise this value
    if(delay <= 0)
        return MICROBIT_INVALID_PARAMETER;

    // If there's an ongoing animation, wait for our turn to display.
    this->waitForFreeDisplay();

    // If the display is free, it's our turn to display.
    // If someone called stopAnimation(), then we simply skip...
    if (animationMode == ANIMATION_MODE_NONE)
    {
        // Start the effect.
        this->scrollAsync(s, delay);

        // Wait for completion.
        fiber_wait_for_event(MICROBIT_ID_DISPLAY, MICROBIT_DISPLAY_EVT_ANIMATION_COMPLETE);
    }
    else
    {
        return MICROBIT_CANCELLED;
    }

    return MICROBIT_OK;
}

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
int MicroBitDisplay::scroll(MicroBitImage image, int delay, int stride)
{
    //sanitise the delay value
    if(delay <= 0)
        return MICROBIT_INVALID_PARAMETER;

    // If there's an ongoing animation, wait for our turn to display.
    this->waitForFreeDisplay();

    // If the display is free, it's our turn to display.
    // If someone called stopAnimation(), then we simply skip...
    if (animationMode == ANIMATION_MODE_NONE)
    {
        // Start the effect.
        this->scrollAsync(image, delay, stride);

        // Wait for completion.
        fiber_wait_for_event(MICROBIT_ID_DISPLAY, MICROBIT_DISPLAY_EVT_ANIMATION_COMPLETE);
    }
    else
    {
        return MICROBIT_CANCELLED;
    }

    return MICROBIT_OK;
}

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
int MicroBitDisplay::animateAsync(MicroBitImage image, int delay, int stride, int startingPosition)
{
    //sanitise the delay value
    if(delay <= 0)
        return MICROBIT_INVALID_PARAMETER;

    // If the display is free, we can display.
    if (animationMode == ANIMATION_MODE_NONE || animationMode == ANIMATION_MODE_STOPPED)
    {
        // Assume right to left functionality, to align with scrollString()
        stride = -stride;

        //calculate starting position which is offset by the stride
        scrollingImagePosition = (startingPosition == MICROBIT_DISPLAY_ANIMATE_DEFAULT_POS) ? MICROBIT_DISPLAY_WIDTH + stride : startingPosition;
        scrollingImageStride = stride;
        scrollingImage = image;
        scrollingImageRendered = false;

        animationDelay = stride == 0 ? 0 : delay;
        animationTick = delay-1;
        animationMode = ANIMATION_MODE_ANIMATE_IMAGE;
    }
    else
    {
        return MICROBIT_BUSY;
    }

    return MICROBIT_OK;
}

/**
  * "Animates" the current image across the display with a given stride, finishing on the last frame of the animation.
  * Blocks the calling thread until the animation is complete.
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
  * uBit.display.animate(i,100,5);
  * @endcode
  */
int MicroBitDisplay::animate(MicroBitImage image, int delay, int stride, int startingPosition)
{
    //sanitise the delay value
    if(delay <= 0)
        return MICROBIT_INVALID_PARAMETER;

    // If there's an ongoing animation, wait for our turn to display.
    this->waitForFreeDisplay();

    // If the display is free, it's our turn to display.
    // If someone called stopAnimation(), then we simply skip...
    if (animationMode == ANIMATION_MODE_NONE)
    {
        // Start the effect.
        this->animateAsync(image, delay, stride, startingPosition);

        // Wait for completion.
        //TODO: Put this in when we merge tight-validation
        //if (delay > 0)
            fiber_wait_for_event(MICROBIT_ID_DISPLAY, MICROBIT_DISPLAY_EVT_ANIMATION_COMPLETE);
    }
    else
    {
        return MICROBIT_CANCELLED;
    }
    return MICROBIT_OK;
}


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
int MicroBitDisplay::setBrightness(int b)
{
    //sanitise the brightness level
    if(b < 0 || b > 255)
        return MICROBIT_INVALID_PARAMETER;

    this->brightness = b;

    return MICROBIT_OK;
}

/**
  * Sets the mode of the display.
  * @param mode The mode to swap the display into. (can be either DISPLAY_MODE_GREYSCALE, DISPLAY_MODE_BLACK_AND_WHITE, DISPLAY_MODE_BLACK_AND_WHITE_LIGHT_SENSE)
  *
  * Example:
  * @code
  * uBit.display.setDisplayMode(DISPLAY_MODE_GREYSCALE); //per pixel brightness
  * @endcode
  */
void MicroBitDisplay::setDisplayMode(DisplayMode mode)
{
    if(mode == DISPLAY_MODE_BLACK_AND_WHITE_LIGHT_SENSE)
    {
        //to reduce the artifacts on the display - increase the tick
        if(uBit.getTickPeriod() != MICROBIT_LIGHT_SENSOR_TICK_PERIOD)
            uBit.setTickPeriod(MICROBIT_LIGHT_SENSOR_TICK_PERIOD);
    }

    if(this->mode == DISPLAY_MODE_BLACK_AND_WHITE_LIGHT_SENSE && mode != DISPLAY_MODE_BLACK_AND_WHITE_LIGHT_SENSE)
    {

        //if we previously were in light sense mode - return to our default.
        if(uBit.getTickPeriod() != MICROBIT_DEFAULT_TICK_PERIOD)
            uBit.setTickPeriod(MICROBIT_DEFAULT_TICK_PERIOD);

        delete this->lightSensor;

        this->lightSensor = NULL;
    }

    this->mode = mode;
}

/**
  * Gets the mode of the display.
  * @return the current mode of the display
  */
int MicroBitDisplay::getDisplayMode()
{
    return this->mode;
}

/**
  * Fetches the current brightness of this display.
  * @return the brightness of this display, in the range 0..255.
  *
  * Example:
  * @code
  * uBit.display.getBrightness(); //the current brightness
  * @endcode
  */
int MicroBitDisplay::getBrightness()
{
    return this->brightness;
}

/**
  * Rotates the display to the given position.
  * Axis aligned values only.
  *
  * Example:
  * @code
  * uBit.display.rotateTo(MICROBIT_DISPLAY_ROTATION_180); //rotates 180 degrees from original orientation
  * @endcode
  */
void MicroBitDisplay::rotateTo(DisplayRotation rotation)
{
    this->rotation = rotation;
}

/**
  * Enables the display, should only be called if the display is disabled.
  *
  * Example:
  * @code
  * uBit.display.enable(); //reenables the display mechanics
  * @endcode
  */
void MicroBitDisplay::enable()
{
    if(!(uBit.flags & MICROBIT_FLAG_DISPLAY_RUNNING))
    {
        setBrightness(brightness);
        uBit.flags |= MICROBIT_FLAG_DISPLAY_RUNNING;            //set the display running flag
    }
}

/**
  * Disables the display, should only be called if the display is enabled.
  * Display must be disabled to avoid MUXing of edge connector pins.
  *
  * Example:
  * @code
  * uBit.display.disable(); //disables the display
  * @endcode
  */
void MicroBitDisplay::disable()
{
    if(uBit.flags & MICROBIT_FLAG_DISPLAY_RUNNING)
    {
        uBit.flags &= ~MICROBIT_FLAG_DISPLAY_RUNNING;           //unset the display running flag
    }
}

/**
  * Clears the current image on the display.
  * Simplifies the process, you can also use uBit.display.image.clear
  *
  * Example:
  * @code
  * uBit.display.clear(); //clears the display
  * @endcode
  */
void MicroBitDisplay::clear()
{
    image.clear();
}

/**
 * Defines the length of time that the device will remain in a error state before resetting.
 * @param iteration The number of times the error code will be displayed before resetting. Set to zero to remain in error state forever.
 *
 * Example:
 * @code
 * uBit.display.setErrorTimeout(4);
 * @endcode
 */
void MicroBitDisplay::setErrorTimeout(int iterations)
{
    this->errorTimeout = iterations;
}

/**
  * Displays "=(" and an accompanying status code infinitely.
  * @param statusCode the appropriate status code - 0 means no code will be displayed. Status codes must be in the range 0-255.
  *
  * Example:
  * @code
  * uBit.display.error(20);
  * @endcode
  */
void MicroBitDisplay::error(int statusCode)
{
    extern InterruptIn resetButton;

    __disable_irq(); //stop ALL interrupts

    if(statusCode < 0 || statusCode > 255)
        statusCode = 0;

    disable(); //relinquish PWMOut's control

    uint8_t strobeRow = 0;
    uint8_t strobeBitMsk = MICROBIT_DISPLAY_ROW_RESET;
    uint8_t count = errorTimeout ? errorTimeout : 1;

    //point to the font stored in Flash
    const unsigned char * fontLocation = MicroBitFont::defaultFont;

    //get individual digits of status code, and place it into a single array/
    const uint8_t* chars[MICROBIT_DISPLAY_ERROR_CHARS] = { panicFace, fontLocation+((((statusCode/100 % 10)+48)-MICROBIT_FONT_ASCII_START) * 5), fontLocation+((((statusCode/10 % 10)+48)-MICROBIT_FONT_ASCII_START) * 5), fontLocation+((((statusCode % 10)+48)-MICROBIT_FONT_ASCII_START) * 5)};

    //enter infinite loop.
    while(count)
    {
        //iterate through our chars :)
        for(int characterCount = 0; characterCount < MICROBIT_DISPLAY_ERROR_CHARS; characterCount++)
        {
            int outerCount = 0;

            //display the current character
            while(outerCount < 500)
            {
                int coldata = 0;

                int i = 0;

                //if we have hit the row limit - reset both the bit mask and the row variable
                if(strobeRow == 3)
                {
                    strobeRow = 0;
                    strobeBitMsk = MICROBIT_DISPLAY_ROW_RESET;
                }

                // Calculate the bitpattern to write.
                for (i = 0; i<MICROBIT_DISPLAY_COLUMN_COUNT; i++)
                {

                    int bitMsk = 0x10 >> matrixMap[i][strobeRow].x; //chars are right aligned but read left to right
                    int y = matrixMap[i][strobeRow].y;

                    if(chars[characterCount][y] & bitMsk)
                        coldata |= (1 << i);
                }

                nrf_gpio_port_write(NRF_GPIO_PORT_SELECT_PORT0, 0xF0); //clear port 0 4-7
                nrf_gpio_port_write(NRF_GPIO_PORT_SELECT_PORT1, strobeBitMsk | 0x1F); // clear port 1 8-12

                //write the new bit pattern
                nrf_gpio_port_write(NRF_GPIO_PORT_SELECT_PORT0, ~coldata<<4 & 0xF0); //set port 0 4-7
                nrf_gpio_port_write(NRF_GPIO_PORT_SELECT_PORT1, strobeBitMsk | (~coldata>>4 & 0x1F)); //set port 1 8-12

                //set i to an obscene number.
                i = 1000;

                //burn cycles
                while(i>0)
                {
                    // Check if the reset button has been pressed. Interrupts are disabled, so the normal method can't be relied upon...
                    if (resetButton == 0)
                        microbit_reset();

                    i--;
                }

                //update the bit mask and row count
                strobeBitMsk <<= 1;
                strobeRow++;
                outerCount++;
            }
        }

        if (errorTimeout)
            count--;
    }

    microbit_reset(); 
}

/**
  * Updates the font property of this object with the new font.
  * @param font the new font that will be used to render characters..
  */
void MicroBitDisplay::setFont(MicroBitFont font)
{
    this->font = font;
}

/**
  * Retreives the font object used for rendering characters on the display.
  */
MicroBitFont MicroBitDisplay::getFont()
{
    return this->font;
}

/**
  * Captures the bitmap currently being rendered on the display.
  */
MicroBitImage MicroBitDisplay::screenShot()
{
    return image.crop(0,0,MICROBIT_DISPLAY_WIDTH,MICROBIT_DISPLAY_HEIGHT);
}

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
int MicroBitDisplay::readLightLevel()
{
    if(mode != DISPLAY_MODE_BLACK_AND_WHITE_LIGHT_SENSE)
    {
        setDisplayMode(DISPLAY_MODE_BLACK_AND_WHITE_LIGHT_SENSE);
        this->lightSensor = new MicroBitLightSensor();
    }

    return this->lightSensor->read();
}

/**
  * Destructor for MicroBitDisplay, so that we deregister ourselves as a systemComponent
  */
MicroBitDisplay::~MicroBitDisplay()
{
    uBit.removeSystemComponent(this);
}
