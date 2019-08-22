import sys

LOG_SIZE = 512
log_start = 85

def process_flags(num):
    flags = {
        RADIO_STATUS_RX_EN = 0x00000001,
        RADIO_STATUS_RX_RDY = 0x00000002,

        RADIO_STATUS_DISABLE = 0x00000004,
        RADIO_STATUS_DISABLED = 0x00000008,

        RADIO_STATUS_TX_EN = 0x00000010,
        RADIO_STATUS_TX_RDY = 0x00000020,
        RADIO_STATUS_TX_ST = 0x00000040,
        RADIO_STATUS_TX_END = 0x00000080,

        RADIO_STATUS_TRANSMIT = 0x00000100,
        RADIO_STATUS_FORWARD = 0x00000200,
        RADIO_STATUS_RECEIVING = 0x00000400,
        RADIO_STATUS_STORE = 0x00000800,
        RADIO_STATUS_DISCOVERING = 0x00001000,
        RADIO_STATUS_SLEEPING = 0x00002000,
        RADIO_STATUS_WAKE_CONFIGURED = 0x00004000,
        RADIO_STATUS_EXPECT_RESPONSE = 0x00008000,
        RADIO_STATUS_FIRST_PACKET = 0x00010000,
        RADIO_STATUS_SAMPLING = 0x00020000,
        RADIO_STATUS_DIRECTING = 0x00040000,
        RADIO_STATUS_QUEUE_KEEP_ALIVE = 0x00080000
    }

    lineno = (num & 0x7FF00000) >> 20
    isSet = num & (1 << 31)
    logflags = num & 0x000FFFFF

    flag_str = ""

    if isSet > 0:
        flag_str = "S: "
    else:
        flag_str = "U: "

    flag_str += str(lineno) + " " + hex(logflags) + " "
    for flag in flags.keys():
        if flags[flag] & logflags:
            flag_str += flag + " "

    print(flag_str)

iterator = log_start
log_end = iterator - 1

while iterator != log_end:
    process_flags(log[iterator])
    iterator = (iterator + 1) % LOG_SIZE



