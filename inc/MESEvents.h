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
// TODO Multiple DISPLAY_TOAST event values
#define MES_ALERT_EVT_DISPLAY_TOAST         1
#define MES_ALERT_EVT_VIBRATE               2
// TODO PLAY_SOUND and ALARMN to be combined
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


#endif
