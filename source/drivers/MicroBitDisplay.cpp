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
  * Class definition for MicroBitDisplay.
  *
  * A MicroBitDisplay represents the LED matrix array on the micro:bit.
  */
#include "MicroBitConfig.h"
#include "MicroBitDisplay.h"
#include "MicroBitSystemTimer.h"
#include "MicroBitFiber.h"
#include "ErrorNo.h"
#include "NotifyEvents.h"

const int greyScaleTimings[MICROBIT_DISPLAY_GREYSCALE_BIT_DEPTH] = {1, 23, 70, 163, 351, 726, 1476, 2976};

/**
  * Constructor.
  *
  * Create a software representation the micro:bit's 5x5 LED matrix.
  * The display is initially blank.
  *
  * @param id The id the display should use when sending events on the MessageBus. Defaults to MICROBIT_ID_DISPLAY.
  *
  * @param map The mapping information that relates pin inputs/outputs to physical screen coordinates.
  *            Defaults to microbitMatrixMap, defined in MicroBitMatrixMaps.h.
  *
  * @code
  * MicroBitDisplay display;
  * @endcode
  */
MicroBitDisplay::MicroBitDisplay(uint16_t id, const MatrixMap &map) :
    matrixMap(map),
    image(map.width*2,map.height)
{
    uint32_t row_mask;

    this->id = id;
    this->width = map.width;
    this->height = map.height;
    this->rotation = MICROBIT_DISPLAY_ROTATION_0;

    row_mask = 0;
    col_mask = 0;
    strobeRow = 0;
    row_mask = 0;

    for (int i = matrixMap.rowStart; i < matrixMap.rowStart + matrixMap.rows; i++)
        row_mask |= 0x01 << i;

    for (int i = matrixMap.columnStart; i < matrixMap.columnStart + matrixMap.columns; i++)
        col_mask |= 0x01 << i;

    LEDMatrix = new PortOut(Port0, row_mask | col_mask);

    this->greyscaleBitMsk = 0x01;
    this->timingCount = 0;
    this->setBrightness(MICROBIT_DISPLAY_DEFAULT_BRIGHTNESS);
    this->mode = DISPLAY_MODE_BLACK_AND_WHITE;
    this->animationMode = ANIMATION_MODE_NONE;
    this->lightSensor = NULL;

	system_timer_add_component(this);

    status |= MICROBIT_COMPONENT_RUNNING;
}

/**
  * Internal frame update method, used to strobe the display.
  *
  * TODO: Write a more efficient, complementary variation of this method for the case where
  * MICROBIT_DISPLAY_ROW_COUNT > MICROBIT_DISPLAY_COLUMN_COUNT.
  */
void MicroBitDisplay::systemTick()
{
    if(!(status & MICROBIT_COMPONENT_RUNNING))
        return;

    if(mode == DISPLAY_MODE_BLACK_AND_WHITE_LIGHT_SENSE)
    {
        renderWithLightSense();
        return;
    }

    // Move on to the next row.
    strobeRow++;

    //reset the row counts and bit mask when we have hit the max.
    if(strobeRow == matrixMap.rows)
        strobeRow = 0;

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
    *LEDMatrix = 0;
}

void MicroBitDisplay::render()
{
    // Simple optimisation.
    // If display is at zero brightness, there's nothing to do.
    if(brightness == 0)
    {
        renderFinish();
        return;
    }

    // Calculate the bitpattern to write.
    uint32_t row_data = 0x01 << (matrixMap.rowStart + strobeRow);
    uint32_t col_data = 0;

    for (int i = 0; i < matrixMap.columns; i++)
    {
        int index = (i * matrixMap.rows) + strobeRow;

        int x = matrixMap.map[index].x;
        int y = matrixMap.map[index].y;
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
            col_data |= (1 << i);
    }

    // Invert column bits (as we're sinking not sourcing power), and mask off any unused bits.
    col_data = ~col_data << matrixMap.columnStart & col_mask;

    // Write the new bit pattern
    *LEDMatrix = col_data | row_data;

    //timer does not have enough resolution for brightness of 1. 23.53 us
    if(brightness != MICROBIT_DISPLAY_MAXIMUM_BRIGHTNESS && brightness > MICROBIT_DISPLAY_MINIMUM_BRIGHTNESS)
        renderTimer.attach_us(this, &MicroBitDisplay::renderFinish, (((brightness * 950) / (MICROBIT_DISPLAY_MAXIMUM_BRIGHTNESS)) * system_timer_get_period()));

    //this will take around 23us to execute
    if(brightness <= MICROBIT_DISPLAY_MINIMUM_BRIGHTNESS)
        renderFinish();
}

void MicroBitDisplay::renderWithLightSense()
{
    //reset the row counts and bit mask when we have hit the max.
    if(strobeRow == matrixMap.rows + 1)
    {
        MicroBitEvent(id, MICROBIT_DISPLAY_EVT_LIGHT_SENSE);
        strobeRow = 0;
    }
    else
    {
        render();
        this->animationUpdate();

        // Move on to the next row.
        strobeRow++;
    }

}

void MicroBitDisplay::renderGreyscale()
{
    // Simple optimisation.
    // If display is at zero brightness, there's nothing to do.
    if(brightness == 0)
    {
        renderFinish();
        return;
    }

    uint32_t row_data = 0x01 << (matrixMap.rowStart + strobeRow);
    uint32_t col_data = 0;

    // Calculate the bitpattern to write.
    for (int i = 0; i < matrixMap.columns; i++)
    {
        int index = (i * matrixMap.rows) + strobeRow;

        int x = matrixMap.map[index].x;
        int y = matrixMap.map[index].y;
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
            col_data |= (1 << i);
    }

    // Invert column bits (as we're sinking not sourcing power), and mask off any unused bits.
    col_data = ~col_data << matrixMap.columnStart & col_mask;

    // Write the new bit pattern
    *LEDMatrix = col_data | row_data;

    if(timingCount > MICROBIT_DISPLAY_GREYSCALE_BIT_DEPTH-1)
        return;

    greyscaleBitMsk <<= 1;

    if(timingCount < 3)
    {
        wait_us(greyScaleTimings[timingCount++]);
        renderGreyscale();
        return;
    }
    renderTimer.attach_us(this,&MicroBitDisplay::renderGreyscale, greyScaleTimings[timingCount++]);
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

    animationTick += system_timer_get_period();

    if(animationTick >= animationDelay)
    {
        animationTick = 0;

        if (animationMode == ANIMATION_MODE_SCROLL_TEXT)
            this->updateScrollText();

        if (animationMode == ANIMATION_MODE_PRINT_TEXT)
            this->updatePrintText();

        if (animationMode == ANIMATION_MODE_SCROLL_IMAGE)
            this->updateScrollImage();

        if (animationMode == ANIMATION_MODE_ANIMATE_IMAGE || animationMode == ANIMATION_MODE_ANIMATE_IMAGE_WITH_CLEAR)
            this->updateAnimateImage();

        if(animationMode == ANIMATION_MODE_PRINT_CHARACTER)
        {
            animationMode = ANIMATION_MODE_NONE;
            this->sendAnimationCompleteEvent();
        }
    }
}

/**
  * Broadcasts an event onto the defult EventModel indicating that the
  * current animation has completed.
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
  * Paste the next character in the string.
  */
void MicroBitDisplay::updatePrintText()
{
    image.print(printingChar < printingText.length() ? printingText.charAt(printingChar) : ' ',0,0);
    if (printingChar >= printingText.length())
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
        if (animationMode == ANIMATION_MODE_ANIMATE_IMAGE_WITH_CLEAR)
            this->clear();

        animationMode = ANIMATION_MODE_NONE;

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
  * Blocks the current fiber until the display is available (i.e. does not effect is being displayed).
  * Animations are queued until their time to display.
  */
void MicroBitDisplay::waitForFreeDisplay()
{
    // If there's an ongoing animation, wait for our turn to display.
    while (animationMode != ANIMATION_MODE_NONE && animationMode != ANIMATION_MODE_STOPPED)
        fiber_wait_for_event(MICROBIT_ID_NOTIFY, MICROBIT_DISPLAY_EVT_FREE);
}

/**
  * Blocks the current fiber until the current animation has finished.
  * If the scheduler is not running, this call will essentially perform a spinning wait.
  */
void MicroBitDisplay::fiberWait()
{
    if (fiber_wait_for_event(MICROBIT_ID_DISPLAY, MICROBIT_DISPLAY_EVT_ANIMATION_COMPLETE) == MICROBIT_NOT_SUPPORTED)
        while(animationMode != ANIMATION_MODE_NONE && animationMode != ANIMATION_MODE_STOPPED)
            __WFE();
}

/**
  * Prints the given character to the display, if it is not in use.
  *
  * @param c The character to display.
  *
  * @param delay Optional parameter - the time for which to show the character. Zero displays the character forever,
  *              or until the Displays next use.
  *
  * @return MICROBIT_OK, MICROBIT_BUSY is the screen is in use, or MICROBIT_INVALID_PARAMETER.
  *
  * @code
  * display.printAsync('p');
  * display.printAsync('p',100);
  * @endcode
  */
int MicroBitDisplay::printCharAsync(char c, int delay)
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
  * Prints the given ManagedString to the display, one character at a time.
  * Returns immediately, and executes the animation asynchronously.
  *
  * @param s The string to display.
  *
  * @param delay The time to delay between characters, in milliseconds. Must be > 0.
  *              Defaults to: MICROBIT_DEFAULT_PRINT_SPEED.
  *
  * @return MICROBIT_OK, or MICROBIT_INVALID_PARAMETER.
  *
  * @code
  * display.printAsync("abc123",400);
  * @endcode
  */
int MicroBitDisplay::printAsync(ManagedString s, int delay)
{
    //sanitise this value
    if (delay < 0 || (delay == 0 && s.length() > 1))
        return MICROBIT_INVALID_PARAMETER;

    if (s.length() == 1)
        return printCharAsync(s.charAt(0), delay);

    if (animationMode == ANIMATION_MODE_NONE || animationMode == ANIMATION_MODE_STOPPED)
    {
        printingChar = 0;
        printingText = s;
        animationDelay = delay;
        animationTick = delay-1;

        animationMode = ANIMATION_MODE_PRINT_TEXT;
    }
    else
    {
        return MICROBIT_BUSY;
    }

    return MICROBIT_OK;
}

/**
  * Prints the given ManagedString to the display, one character at a time.
  * Returns immediately, and executes the animation asynchronously.
  *
  * If the string is greater than one charcter in length, the screen
  * will be cleared after MICROBIT_DEFAULT_PRINT_SPEED milliseconds.
  * Otherwise, that character will be left on the screen indefinitely.
  *
  * @param s The string to display.
  *
  * @return MICROBIT_OK, or MICROBIT_INVALID_PARAMETER.
  *
  * @code
  * display.printAsync("abc123");
  * @endcode
  */
int MicroBitDisplay::printAsync(ManagedString s)
{
    int delay = MICROBIT_DEFAULT_PRINT_SPEED;

    if(s.length() == 1)
        delay = 0;

    return printAsync(s, delay);
}


/**
  * Prints the given image to the display, if the display is not in use.
  * Returns immediately, and executes the animation asynchronously.
  *
  * @param i The image to display.
  *
  * @param x The horizontal position on the screen to display the image. Defaults to 0.
  *
  * @param y The vertical position on the screen to display the image. Defaults to 0.
  *
  * @param alpha Treats the brightness level '0' as transparent. Defaults to 0.
  *
  * @param delay The time to delay between characters, in milliseconds. Defaults to 0.
  *
  * @code
  * MicrobitImage i("1,1,1,1,1\n1,1,1,1,1\n");
  * display.print(i,400);
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
  * Prints the given character to the display.
  *
  * @param c The character to display.
  *
  * @param delay Optional parameter - the time for which to show the character. Zero displays the character forever,
  *              or until the Displays next use.
  *
  * @return MICROBIT_OK, MICROBIT_CANCELLED or MICROBIT_INVALID_PARAMETER.
  *
  * @code
  * display.printAsync('p');
  * display.printAsync('p',100);
  * @endcode
  */
int MicroBitDisplay::printChar(char c, int delay)
{
    if (delay < 0)
        return MICROBIT_INVALID_PARAMETER;

    // If there's an ongoing animation, wait for our turn to display.
    this->waitForFreeDisplay();

    // If the display is free, it's our turn to display.
    // If someone called stopAnimation(), then we simply skip...
    if (animationMode == ANIMATION_MODE_NONE)
    {
        this->printCharAsync(c, delay);

        if (delay > 0)
            fiberWait();
    }
    else
    {
        return MICROBIT_CANCELLED;
    }

    return MICROBIT_OK;
}

/**
  * Prints the given string to the display, one character at a time.
  *
  * Blocks the calling thread until all the text has been displayed.
  *
  * @param s The string to display.
  *
  * @param delay The time to delay between characters, in milliseconds. Defaults
  *              to: MICROBIT_DEFAULT_PRINT_SPEED.
  *
  * @return MICROBIT_OK, MICROBIT_CANCELLED or MICROBIT_INVALID_PARAMETER.
  *
  * @code
  * display.print("abc123",400);
  * @endcode
  */
int MicroBitDisplay::print(ManagedString s, int delay)
{
    //sanitise this value
    if(delay < 0 || (delay == 0 && s.length() > 1))
        return MICROBIT_INVALID_PARAMETER;

    // If there's an ongoing animation, wait for our turn to display.
    this->waitForFreeDisplay();

    // If the display is free, it's our turn to display.
    // If someone called stopAnimation(), then we simply skip...
    if (animationMode == ANIMATION_MODE_NONE)
    {
        int ret = this->printAsync(s, delay);
        if (s.length() == 1)
            return ret;
        
        fiberWait();
    }
    else
    {
        return MICROBIT_CANCELLED;
    }

    return MICROBIT_OK;
}

/**
  * Prints the given string to the display, one character at a time.
  *
  * Blocks the calling thread until all the text has been displayed.
  * 
  * If the string is greater than one charcter in length, the screen
  * will be cleared after MICROBIT_DEFAULT_PRINT_SPEED milliseconds.
  * Otherwise, that character will be left on the screen indefinitely.
  *
  * @param s The string to display.
  *
  * @return MICROBIT_OK, MICROBIT_CANCELLED or MICROBIT_INVALID_PARAMETER.
  *
  * @code
  * display.print("abc123");
  * @endcode
  */
int MicroBitDisplay::print(ManagedString s)
{
    int delay = MICROBIT_DEFAULT_PRINT_SPEED;

    if(s.length() == 1)
        delay = 0;

    return print(s, delay);
}


/**
  * Prints the given image to the display.
  * Blocks the calling thread until all the image has been displayed.
  *
  * @param i The image to display.
  *
  * @param x The horizontal position on the screen to display the image. Defaults to 0.
  *
  * @param y The vertical position on the screen to display the image. Defaults to 0.
  *
  * @param alpha Treats the brightness level '0' as transparent. Defaults to 0.
  *
  * @param delay The time to display the image for, or zero to show the image forever. Defaults to 0.
  *
  * @return MICROBIT_OK, MICROBIT_BUSY if the display is already in use, or MICROBIT_INVALID_PARAMETER.
  *
  * @code
  * MicrobitImage i("1,1,1,1,1\n1,1,1,1,1\n");
  * display.print(i,400);
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
            fiberWait();
    }
    else
    {
        return MICROBIT_CANCELLED;
    }

    return MICROBIT_OK;
}

/**
  * Scrolls the given string to the display, from right to left.
  * Returns immediately, and executes the animation asynchronously.
  *
  * @param s The string to display.
  *
  * @param delay The time to delay between characters, in milliseconds. Defaults
  *              to: MICROBIT_DEFAULT_SCROLL_SPEED.
  *
  * @return MICROBIT_OK, MICROBIT_BUSY if the display is already in use, or MICROBIT_INVALID_PARAMETER.
  *
  * @code
  * display.scrollAsync("abc123",100);
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
  *
  * @param delay The time between updates, in milliseconds. Defaults
  *              to: MICROBIT_DEFAULT_SCROLL_SPEED.
  *
  * @param stride The number of pixels to shift by in each update. Defaults to MICROBIT_DEFAULT_SCROLL_STRIDE.
  *
  * @return MICROBIT_OK, MICROBIT_BUSY if the display is already in use, or MICROBIT_INVALID_PARAMETER.
  *
  * @code
  * MicrobitImage i("1,1,1,1,1\n1,1,1,1,1\n");
  * display.scrollAsync(i,100,1);
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
  * Scrolls the given string across the display, from right to left.
  * Blocks the calling thread until all text has been displayed.
  *
  * @param s The string to display.
  *
  * @param delay The time to delay between characters, in milliseconds. Defaults
  *              to: MICROBIT_DEFAULT_SCROLL_SPEED.
  *
  * @return MICROBIT_OK, MICROBIT_CANCELLED or MICROBIT_INVALID_PARAMETER.
  *
  * @code
  * display.scroll("abc123",100);
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
        fiberWait();
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
  *
  * @param delay The time between updates, in milliseconds. Defaults
  *              to: MICROBIT_DEFAULT_SCROLL_SPEED.
  *
  * @param stride The number of pixels to shift by in each update. Defaults to MICROBIT_DEFAULT_SCROLL_STRIDE.
  *
  * @return MICROBIT_OK, MICROBIT_CANCELLED or MICROBIT_INVALID_PARAMETER.
  *
  * @code
  * MicrobitImage i("1,1,1,1,1\n1,1,1,1,1\n");
  * display.scroll(i,100,1);
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
        fiberWait();
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
  *
  * @param delay The time to delay between each update of the display, in milliseconds.
  *
  * @param stride The number of pixels to shift by in each update.
  *
  * @param startingPosition the starting position on the display for the animation
  *                         to begin at. Defaults to MICROBIT_DISPLAY_ANIMATE_DEFAULT_POS.
  *
  * @param autoClear defines whether or not the display is automatically cleared once the animation is complete. By default, the display is cleared. Set this parameter to zero to disable the autoClear operation.
  *
  * @return MICROBIT_OK, MICROBIT_BUSY if the screen is in use, or MICROBIT_INVALID_PARAMETER.
  *
  * @code
  * const int heart_w = 10;
  * const int heart_h = 5;
  * const uint8_t heart[] = { 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 1, 0, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 0, 0, 1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, };
  *
  * MicroBitImage i(heart_w,heart_h,heart);
  * display.animateAsync(i,100,5);
  * @endcode
  */
int MicroBitDisplay::animateAsync(MicroBitImage image, int delay, int stride, int startingPosition, int autoClear)
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
        animationMode = autoClear ? ANIMATION_MODE_ANIMATE_IMAGE_WITH_CLEAR : ANIMATION_MODE_ANIMATE_IMAGE;
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
  *
  * @param delay The time to delay between each update of the display, in milliseconds.
  *
  * @param stride The number of pixels to shift by in each update.
  *
  * @param startingPosition the starting position on the display for the animation
  *                         to begin at. Defaults to MICROBIT_DISPLAY_ANIMATE_DEFAULT_POS.
  *
  * @param autoClear defines whether or not the display is automatically cleared once the animation is complete. By default, the display is cleared. Set this parameter to zero to disable the autoClear operation.
  *
  * @return MICROBIT_OK, MICROBIT_CANCELLED or MICROBIT_INVALID_PARAMETER.
  *
  * @code
  * const int heart_w = 10;
  * const int heart_h = 5;
  * const uint8_t heart[] = { 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 1, 0, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 0, 0, 1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, };
  *
  * MicroBitImage i(heart_w,heart_h,heart);
  * display.animate(i,100,5);
  * @endcode
  */
int MicroBitDisplay::animate(MicroBitImage image, int delay, int stride, int startingPosition, int autoClear)
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
        this->animateAsync(image, delay, stride, startingPosition, autoClear);

        // Wait for completion.
        //TODO: Put this in when we merge tight-validation
        //if (delay > 0)
            fiberWait();
    }
    else
    {
        return MICROBIT_CANCELLED;
    }

    return MICROBIT_OK;
}


/**
  * Configures the brightness of the display.
  *
  * @param b The brightness to set the brightness to, in the range 0 - 255.
  *
  * @return MICROBIT_OK, or MICROBIT_INVALID_PARAMETER
  *
  * @code
  * display.setBrightness(255); //max brightness
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
  * Configures the mode of the display.
  *
  * @param mode The mode to swap the display into. One of: DISPLAY_MODE_GREYSCALE,
  *             DISPLAY_MODE_BLACK_AND_WHITE, DISPLAY_MODE_BLACK_AND_WHITE_LIGHT_SENSE
  *
  * @code
  * display.setDisplayMode(DISPLAY_MODE_GREYSCALE); //per pixel brightness
  * @endcode
  */
void MicroBitDisplay::setDisplayMode(DisplayMode mode)
{
    if(mode == DISPLAY_MODE_BLACK_AND_WHITE_LIGHT_SENSE)
    {
        //to reduce the artifacts on the display - increase the tick
        if(system_timer_get_period() != MICROBIT_LIGHT_SENSOR_TICK_PERIOD)
            system_timer_set_period(MICROBIT_LIGHT_SENSOR_TICK_PERIOD);
    }

    if(this->mode == DISPLAY_MODE_BLACK_AND_WHITE_LIGHT_SENSE && mode != DISPLAY_MODE_BLACK_AND_WHITE_LIGHT_SENSE)
    {
        delete this->lightSensor;

        this->lightSensor = NULL;
    }

    this->mode = mode;
}

/**
  * Retrieves the mode of the display.
  *
  * @return the current mode of the display
  */
int MicroBitDisplay::getDisplayMode()
{
    return this->mode;
}

/**
  * Fetches the current brightness of this display.
  *
  * @return the brightness of this display, in the range 0..255.
  *
  * @code
  * display.getBrightness(); //the current brightness
  * @endcode
  */
int MicroBitDisplay::getBrightness()
{
    return this->brightness;
}

/**
  * Rotates the display to the given position.
  *
  * Axis aligned values only.
  *
  * @code
  * display.rotateTo(MICROBIT_DISPLAY_ROTATION_180); //rotates 180 degrees from original orientation
  * @endcode
  */
void MicroBitDisplay::rotateTo(DisplayRotation rotation)
{
    this->rotation = rotation;
}

/**
 * Enables or disables the display entirely, and releases the pins for other uses.
 *
 * @param enableDisplay true to enabled the display, or false to disable it.
 */
void MicroBitDisplay::setEnable(bool enableDisplay)
{
    // If we're already in the correct state, then there's nothing to do.
    if(((status & MICROBIT_COMPONENT_RUNNING) && enableDisplay) || (!(status & MICROBIT_COMPONENT_RUNNING) && !enableDisplay))
        return;

    uint32_t rmask = 0;
    uint32_t cmask = 0;

    for (int i = matrixMap.rowStart; i < matrixMap.rowStart + matrixMap.rows; i++)
        rmask |= 0x01 << i;

    for (int i = matrixMap.columnStart; i < matrixMap.columnStart + matrixMap.columns; i++)
        cmask |= 0x01 << i;

    if (enableDisplay)
    {
        PortOut p(Port0, rmask | cmask);
        status |= MICROBIT_COMPONENT_RUNNING;
    }
    else
    {
        PortIn p(Port0, rmask | cmask);
        p.mode(PullNone);
        status &= ~MICROBIT_COMPONENT_RUNNING;
    }
}

/**
  * Enables the display, should only be called if the display is disabled.
  *
  * @code
  * display.enable(); //Enables the display mechanics
  * @endcode
  *
  * @note Only enables the display if the display is currently disabled.
  */
void MicroBitDisplay::enable()
{
    setEnable(true);
}

/**
  * Disables the display, which releases control of the GPIO pins used by the display,
  * which are exposed on the edge connector.
  *
  * @code
  * display.disable(); //disables the display
  * @endcode
  *
  * @note Only disables the display if the display is currently enabled.
  */
void MicroBitDisplay::disable()
{
    setEnable(false);
}

/**
  * Clears the display of any remaining pixels.
  *
  * `display.image.clear()` can also be used!
  *
  * @code
  * display.clear(); //clears the display
  * @endcode
  */
void MicroBitDisplay::clear()
{
    image.clear();
}

/**
  * Updates the font that will be used for display operations.
  *
  * @param font the new font that will be used to render characters.
  *
  * @note DEPRECATED! Please use MicroBitFont::setSystemFont() instead.
  */
void MicroBitDisplay::setFont(MicroBitFont font)
{
	MicroBitFont::setSystemFont(font);
}

/**
  * Retrieves the font object used for rendering characters on the display.
  *
  * @note DEPRECATED! Please use MicroBitFont::getSystemFont() instead.
  */
MicroBitFont MicroBitDisplay::getFont()
{
	return MicroBitFont::getSystemFont();
}

/**
  * Captures the bitmap currently being rendered on the display.
  *
  * @return a MicroBitImage containing the captured data.
  */
MicroBitImage MicroBitDisplay::screenShot()
{
    return image.crop(0,0,MICROBIT_DISPLAY_WIDTH,MICROBIT_DISPLAY_HEIGHT);
}

/**
  * Gives a representative figure of the light level in the current environment
  * where are micro:bit is situated.
  *
  * Internally, it constructs an instance of a MicroBitLightSensor if not already configured
  * and sets the display mode to DISPLAY_MODE_BLACK_AND_WHITE_LIGHT_SENSE.
  *
  * This also changes the tickPeriod to MICROBIT_LIGHT_SENSOR_TICK_SPEED so
  * that the display does not suffer from artifacts.
  *
  * @return an indicative light level in the range 0 - 255.
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
        this->lightSensor = new MicroBitLightSensor(matrixMap);
    }

    return this->lightSensor->read();
}

/**
  * Destructor for MicroBitDisplay, where we deregister this instance from the array of system components.
  */
MicroBitDisplay::~MicroBitDisplay()
{
    system_timer_remove_component(this);
}
