/* mbed Microcontroller Library
 * Copyright (c) 2015 ARM Limited
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef KBD_SERVICE_BASE_H_
#define KBD_SERVICE_BASE_H_

#include <errno.h>
#include "mbed.h"
#include "CircularBuffer.h"

#include "HIDServiceBase.h"
#include "Keyboard_types.h"

/* TODO: make this easier to configure by application (e.g. as a template parameter for
 * KeyboardService) */
#ifndef KEYBUFFER_SIZE
#define KEYBUFFER_SIZE 32
#endif

/**
 * Report descriptor for a standard 101 keys keyboard, following the HID specification example:
 * - 8 bytes input report (1 byte for modifiers and 6 for keys)
 * - 1 byte output report (LEDs)
 */
static report_map_t KEYBOARD_REPORT_MAP = {
    USAGE_PAGE(1),      0x01,       // Generic Desktop Ctrls
    USAGE(1),           0x06,       // Keyboard
    COLLECTION(1),      0x01,       // Application
    USAGE_PAGE(1),      0x07,       //   Kbrd/Keypad
    USAGE_MINIMUM(1),   0xE0,
    USAGE_MAXIMUM(1),   0xE7,
    LOGICAL_MINIMUM(1), 0x00,
    LOGICAL_MAXIMUM(1), 0x01,
    REPORT_SIZE(1),     0x01,       //   1 byte (Modifier)
    REPORT_COUNT(1),    0x08,
    INPUT(1),           0x02,       //   Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position
    REPORT_COUNT(1),    0x01,       //   1 byte (Reserved)
    REPORT_SIZE(1),     0x08,
    INPUT(1),           0x01,       //   Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position
    REPORT_COUNT(1),    0x05,       //   5 bits (Num lock, Caps lock, Scroll lock, Compose, Kana)
    REPORT_SIZE(1),     0x01,
    USAGE_PAGE(1),      0x08,       //   LEDs
    USAGE_MINIMUM(1),   0x01,       //   Num Lock
    USAGE_MAXIMUM(1),   0x05,       //   Kana
    OUTPUT(1),          0x02,       //   Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile
    REPORT_COUNT(1),    0x01,       //   3 bits (Padding)
    REPORT_SIZE(1),     0x03,
    OUTPUT(1),          0x01,       //   Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile
    REPORT_COUNT(1),    0x06,       //   6 bytes (Keys)
    REPORT_SIZE(1),     0x08,
    LOGICAL_MINIMUM(1), 0x00,
    LOGICAL_MAXIMUM(1), 0x65,       //   101 keys
    USAGE_PAGE(1),      0x07,       //   Kbrd/Keypad
    USAGE_MINIMUM(1),   0x00,
    USAGE_MAXIMUM(1),   0x65,
    INPUT(1),           0x00,       //   Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position
    END_COLLECTION(0),
};

/// "keys pressed" report
static uint8_t inputReportData[] = { 0, 0, 0, 0, 0, 0, 0, 0 };
/// "keys released" report
static const uint8_t emptyInputReportData[] = { 0, 0, 0, 0, 0, 0, 0, 0 };


/**
 * @class KeyBuffer
 *
 * Buffer used to store keys to send.
 * Internally, it is a CircularBuffer, with the added capability of putting the last char back in,
 * when we're unable to send it (ie. when BLE stack is busy)
 */
class KeyBuffer: public CircularBuffer<uint8_t, KEYBUFFER_SIZE>
{
public:
    KeyBuffer() :
        CircularBuffer(),
        dataIsPending (false),
        keyUpIsPending (false)
    {
    }

    /**
     * Mark a character as pending. When a freshly popped character cannot be sent, because the
     * underlying stack is busy, we set it as pending, and it will get popped in priority by @ref
     * getPending once reports can be sent again.
     *
     * @param data  The character to send in priority. The second keyUp report is implied.
     */
    void setPending(uint8_t data)
    {
        MBED_ASSERT(dataIsPending == false);

        dataIsPending = true;
        pendingData = data;
        keyUpIsPending = true;
    }

    /**
     * Get pending char. Either from the high priority buffer (set with setPending), or from the
     * circular buffer.
     *
     * @param   data Filled with the pending data, when present
     * @return  true if data was filled
     */
    bool getPending(uint8_t &data)
    {
        if (dataIsPending) {
            dataIsPending = false;
            data = pendingData;
            return true;
        }

        return pop(data);
    }

    bool isSomethingPending(void)
    {
        return dataIsPending || keyUpIsPending || !empty();
    }

    /**
     * Signal that a keyUp report is pending. This means that a character has successfully been
     * sent, but the subsequent keyUp report failed. This report is of highest priority than the
     * next character.
     */
    void setKeyUpPending(void)
    {
        keyUpIsPending = true;
    }

    /**
     * Signal that no high-priority report is pending anymore, we can go back to the normal queue.
     */
    void clearKeyUpPending(void)
    {
        keyUpIsPending = false;
    }

    bool isKeyUpPending(void)
    {
        return keyUpIsPending;
    }

protected:
    bool dataIsPending;
    uint8_t pendingData;
    bool keyUpIsPending;
};


/**
 * @class KeyboardService
 * @brief HID-over-Gatt keyboard service
 *
 * Send keyboard reports over BLE. Users should rely on the high-level functions provided by the
 * Stream API. Because we can't send batches of HID reports, we store pending keys in a circular
 * buffer and rely on the report ticker to spread them over time.
 *
 * @code
 * BLE ble;
 * KeyboardService kbd(ble);
 *
 * void once_connected_and_paired_callback(void)
 * {
 *     // Sequentially send keys 'Shift'+'h', 'e', 'l', 'l', 'o', '!' and <enter>
 *     kbd.printf("Hello!\n");
 * }
 * @endcode
 */
class KeyboardService : public HIDServiceBase
{
public:
    KeyboardService(BLE &_ble) :
        HIDServiceBase(_ble,
                KEYBOARD_REPORT_MAP, sizeof(KEYBOARD_REPORT_MAP),
                emptyInputReportData,
                sizeof(inputReportData),
                24 /* ms */),
        failedReports(0)
    {
    }

    virtual void onConnection(const Gap::ConnectionCallbackParams_t *params)
    {
        HIDServiceBase::onConnection(params);

        /* Drain buffer, in case we've been disconnected while transmitting */
        if (!reportTickerIsActive && keyBuffer.isSomethingPending())
            startReportTicker();
    }

    virtual void onDisconnection(const Gap::DisconnectionCallbackParams_t *params)
    {
        stopReportTicker();
        HIDServiceBase::onDisconnection(params);
    }

    /**
     * Send raw report. Should only be called by sendCallback.
     */
    virtual ble_error_t send(const report_t report)
    {
        static unsigned int consecutiveFailures = 0;
        ble_error_t ret = HIDServiceBase::send(report);

        /*
         * Wait until a buffer is available (onDataSent)
         * TODO. This won't work, because BUSY error is not only returned when we're short of
         * notification buffers, but in other cases as well (e.g. when disconnected). We need to
         * find a reliable way of knowing when we actually need to wait for onDataSent to be called.
        if (ret == BLE_STACK_BUSY)
            stopReportTicker();
         */
        if (ret == BLE_STACK_BUSY)
            consecutiveFailures++;
        else
            consecutiveFailures = 0;

        if (consecutiveFailures > 20) {
            /*
             * We're not transmitting anything anymore. Might as well avoid overloading the
             * system in case it can magically fix itself. Ticker will start again on next _putc
             * call. It could also be started on next connection, but we can't register a callback
             * for that, currently.
             */
            stopReportTicker();
            consecutiveFailures = 0;
        }

        return ret;
    }

    /**
     * Send an empty report, representing keyUp event
     */
    ble_error_t keyUpCode(void)
    {
        return send(emptyInputReportData);
    }

    /**
     * Send a character, defined by a modifier (CTRL, SHIFT, ALT) and the key
     *
     * @param key Character to send (as defined in USB HID Usage Tables)
     * @param modifier Optional modifiers (logical OR of enum MODIFIER_KEY)
     *
     * @returns BLE_ERROR_NONE on success, or an error code otherwise.
     */
    ble_error_t keyDownCode(uint8_t key, uint8_t modifier)
    {
        inputReportData[0] = modifier;
        inputReportData[2] = keymap[key].usage;

        return send(inputReportData);
    }

    /**
     * Push a key on the internal FIFO
     *
     * @param c ASCII character to send
     *
     * @returns 0 on success, or ENOMEM when the FIFO is full.
     */
    virtual int putc(int c) {
        if (keyBuffer.full()) {
            return ENOMEM;
        }

        keyBuffer.push((unsigned char)c);

        if (!reportTickerIsActive)
            startReportTicker();

        return 0;
    }

    uint8_t lockStatus() {
        // TODO: implement numlock/capslock/scrolllock
        return 0;
    }

    /**
     * Pop a key from the internal FIFO, and attempt to send it over BLE
     */
    virtual void sendCallback(void) {
        ble_error_t ret;
        uint8_t c;

        if (!keyBuffer.isSomethingPending()) {
            /* Stop until the next call to putc */
            stopReportTicker();
            return;
        }

        if (!keyBuffer.isKeyUpPending()) {
            bool hasData = keyBuffer.getPending(c);

            /*
             * If something is pending and is not a keyUp, getPending *must* return something. The
             * following is only a sanity check.
             */
            MBED_ASSERT(hasData);

            if (hasData) {
                ret = keyDownCode(c, keymap[c].modifier);
                if (ret) {
                    keyBuffer.setPending(c);
                    failedReports++;
                    return;
                }
            }
        }

        ret = keyUpCode();
        if (ret) {
            keyBuffer.setKeyUpPending();
            failedReports++;
        } else {
            keyBuffer.clearKeyUpPending();
        }
    }

    /**
     * Restart report ticker if it was disabled, after too many consecutive failures.
     *
     * This is called by the BLE stack.
     *
     * @param count Number of reports (notifications) sent
     */
    virtual void onDataSent(unsigned count)
    {
        (void) count;

        if (!reportTickerIsActive && keyBuffer.isSomethingPending())
            startReportTicker();
    }

    uint32_t failedReports;

protected:
    KeyBuffer keyBuffer;
};

#endif
