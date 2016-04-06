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

#ifndef MES_EVENTS_H
#define MES_EVENTS_H

//
// MicroBit Event Service Event ID's and values
//

//
// Events that master devices respond to:
//
#define MES_REMOTE_CONTROL_ID               1001
#define MES_REMOTE_CONTROL_EVT_PLAY         1
#define MES_REMOTE_CONTROL_EVT_PAUSE        2
#define MES_REMOTE_CONTROL_EVT_STOP         3
#define MES_REMOTE_CONTROL_EVT_NEXTTRACK    4
#define MES_REMOTE_CONTROL_EVT_PREVTRACK    5
#define MES_REMOTE_CONTROL_EVT_FORWARD      6
#define MES_REMOTE_CONTROL_EVT_REWIND       7
#define MES_REMOTE_CONTROL_EVT_VOLUMEUP     8
#define MES_REMOTE_CONTROL_EVT_VOLUMEDOWN   9


#define MES_CAMERA_ID                       1002
#define MES_CAMERA_EVT_LAUNCH_PHOTO_MODE    1
#define MES_CAMERA_EVT_LAUNCH_VIDEO_MODE    2
#define MES_CAMERA_EVT_TAKE_PHOTO           3
#define MES_CAMERA_EVT_START_VIDEO_CAPTURE  4
#define MES_CAMERA_EVT_STOP_VIDEO_CAPTURE   5
#define MES_CAMERA_EVT_STOP_PHOTO_MODE      6
#define MES_CAMERA_EVT_STOP_VIDEO_MODE      7
#define MES_CAMERA_EVT_TOGGLE_FRONT_REAR    8


#define MES_ALERTS_ID                       1004
#define MES_ALERT_EVT_DISPLAY_TOAST         1
#define MES_ALERT_EVT_VIBRATE               2
#define MES_ALERT_EVT_PLAY_SOUND            3
#define MES_ALERT_EVT_PLAY_RINGTONE         4
#define MES_ALERT_EVT_FIND_MY_PHONE         5
#define MES_ALERT_EVT_ALARM1                6
#define MES_ALERT_EVT_ALARM2                7
#define MES_ALERT_EVT_ALARM3                8
#define MES_ALERT_EVT_ALARM4                9
#define MES_ALERT_EVT_ALARM5                10
#define MES_ALERT_EVT_ALARM6                11

//
// Events that master devices generate:
//
#define MES_SIGNAL_STRENGTH_ID              1101
#define MES_SIGNAL_STRENGTH_EVT_NO_BAR      1
#define MES_SIGNAL_STRENGTH_EVT_ONE_BAR     2
#define MES_SIGNAL_STRENGTH_EVT_TWO_BAR     3
#define MES_SIGNAL_STRENGTH_EVT_THREE_BAR   4
#define MES_SIGNAL_STRENGTH_EVT_FOUR_BAR    5


#define MES_DEVICE_INFO_ID                  1103
#define MES_DEVICE_ORIENTATION_LANDSCAPE    1
#define MES_DEVICE_ORIENTATION_PORTRAIT     2
#define MES_DEVICE_GESTURE_NONE             3
#define MES_DEVICE_GESTURE_DEVICE_SHAKEN    4
#define MES_DEVICE_DISPLAY_OFF              5
#define MES_DEVICE_DISPLAY_ON               6
#define MES_DEVICE_INCOMING_CALL            7
#define MES_DEVICE_INCOMING_MESSAGE         8


#define MES_DPAD_CONTROLLER_ID              1104
#define MES_DPAD_BUTTON_A_DOWN              1
#define MES_DPAD_BUTTON_A_UP                2
#define MES_DPAD_BUTTON_B_DOWN              3
#define MES_DPAD_BUTTON_B_UP                4
#define MES_DPAD_BUTTON_C_DOWN              5
#define MES_DPAD_BUTTON_C_UP                6
#define MES_DPAD_BUTTON_D_DOWN              7
#define MES_DPAD_BUTTON_D_UP                8
#define MES_DPAD_BUTTON_1_DOWN              9
#define MES_DPAD_BUTTON_1_UP                10
#define MES_DPAD_BUTTON_2_DOWN              11
#define MES_DPAD_BUTTON_2_UP                12
#define MES_DPAD_BUTTON_3_DOWN              13
#define MES_DPAD_BUTTON_3_UP                14
#define MES_DPAD_BUTTON_4_DOWN              15
#define MES_DPAD_BUTTON_4_UP                16

//
// Events that typically use radio broadcast:
//
#define MES_BROADCAST_GENERAL_ID            2000

#endif
