#ifndef JMX_H
#define JMX_H

#include "SLIP.h"
#include <stdint.h>

#define JMX_ESCAPE_CHAR            SLIP_ESC
#define JMX_END_CHAR            SLIP_END

#define OBJECT_START            '{'
#define OBJECT_END                '}'

#define STRING_TOKEN            '"'
#define PAIR_SEPARATOR_TOKEN    ','
#define FIELD_SEPARATOR_TOKEN   ':'

#define WHITE_SPACE                ' '

#define JSON_FIELD_MAX_LEN        8

#define STATUS_OK                1
#define STATUS_SUCCESS            0
#define STATUS_ERROR            -1
#define STATUS_LOCKED            -2
#define STATUS_SLIP_ESC            -3

#define STRING_INDICATOR        1
#define NUMBER_INDICATOR        2

#define MAX_JSON_NUMBER            17 + NULL_TERMINATOR
#define IS_DIGIT(X) (X >= 48 && X < 58 )


enum JMXState {
    J_STATE_NONE = 0x00,
    J_STATE_INIT = 0x01,

    // a flag used to indicate the presence of a user buffer, it indicates that we do not call the resulting function,
    // and instead pass the functionality to our external caller.
    J_STATE_USER_BUFFER = 0x02,
    J_STATE_USER_PACKET = 0x04
};

enum JMXParserState {
    // PARSER STATES
    P_STATE_NONE = 0x00,
    P_STATE_KEY = 0x01,
    P_STATE_VALUE = 0x02,

    // a flag used to indicate that we have reached our max object depth (of 1);
    P_STATE_MAX_OBJECT = 0x80
};

enum JMXTokenState {
    // TOKEN STATES
    T_STATE_NONE = 0x00,
    T_STATE_STRING = 0x01,
    T_STATE_NUMBER = 0x02,
    T_STATE_DYNAMIC_STRING = 0x04,
    T_STATE_STREAM_BUFFER = 0x08
};

enum SerialState {
    STATE_IDLE,
    STATE_PROCESSING,
    STATE_ERROR
};

#ifdef __cplusplus
extern "C" {
#endif

/**
  * A simple initialisation call that sets our pointer values to well known c constants.
  */
void jmx_init(void);

/**
  * Accepts a single character and updates the state of this instance of JMX parser.
  *
  * @param c the character to process
  *
  * @return PARSER_OK if the parser has moved to a valid state, PARSER_ERROR if the parser has moved to an invalid state
  *         and PARSER_SUCCESS if the JSON packet has been processed successfully.
  *
  * @note On PARSER_ERROR, all state for the parser will be reset, thus you will have to parse from the beginning of the next packet.
  */
int jmx_parse(char c);

/**
  * Returns the previous char, captured by jmx.
  */
char jmx_previous();

/**
  * Accepts a single character and updates the state of this instance of JMX parser, and also maintains a "serial_state" varaible
  *
  * @param c the character to process
  *
  * @return 0 if the parser consumed the character, 1 if it cannot.
  */
int jmx_state_track(char c);

/**
  * marshalls a given buffer into a serial stream based upon the given identifier.
  *
  * @param identifier the actionTable identifier.
  *
  * @param data the buffer which can be mapped by the given actionTable identifier.
  *
  * @return STATUS_ERROR if the identifier doesn't exist, data is null. Or STATUS_SUCCESS if the send was successful.
  */
int jmx_send(const char* identifier, void* data);

/**
  * Configures the given identifiers' pointer to point to the given buffer (data)
  *
  * @param identifier the actionTable identifier.
  *
  * @param data the buffer to configure the actionTable to point to.
  *
  * @return STATUS_ERROR if the identifier doesn't exist, data is null, or there already is a buffer configured.
  *            or STATUS_SUCCESS
  */
int jmx_configure_buffer(const char* identifier, void* data);

#ifdef __cplusplus
}
#endif

#endif
