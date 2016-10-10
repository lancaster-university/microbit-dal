#include "jmx_actions.h"
#include "jmx.h"
#include "mbed.h"
#include "MicroBitHeapAllocator.h"
#include "MicroBitCompat.h"

JMXInitPacket* jmx_init_p = NULL;
FSRequestPacket* jmx_fsr_p = NULL;
DIRRequestPacket* jmx_dir_p = NULL;
StatusPacket* jmx_status_p = NULL;
RedirectPacket* jmx_redirect_p = NULL;
UARTConfigPacket* jmx_uart_p = NULL;

static uint8_t* jmx_data_buffer = NULL;         // 4
static uint32_t jmx_data_size = 0;              // 4
static uint32_t jmx_data_offset = 0;            // 4

static uint8_t* jmx_key_buffer = NULL;          // 4
static uint8_t  jmx_key_offset = 0;             // 1

static int8_t    jmx_action_index = -1;         // 1

static uint8_t  jmx_parser_state = P_STATE_NONE;    // 1
static uint8_t  jmx_token_state = T_STATE_NONE;     // 1

static uint8_t    jmx_state = 0;
static uint8_t serial_state = STATE_IDLE;

static char previous;                           // 1

#ifdef __cplusplus
extern "C" {
#endif
extern void user_putc(char c);

extern void jmx_packet_received(char*);

void nop(void*) {};
#ifdef __cplusplus
}
#endif

__inline int jmx_get_action_index(JMXActionTable* t, char* key, int key_len)
{
    for (uint8_t i = 0; i < JMX_ACTION_COUNT; i++)
    {
        JMXActionItem* a = &t->actions[i];

        if (a->type == 0 || a->key == NULL)
            continue;

        //if our key matches, an item in the action table.
        if (strncmp(a->key, (char *)key, min(strlen(a->key), key_len)) == 0)
            return i;
    }

    return STATUS_ERROR;
}

__inline int jmx_get_table_entry_index(char* packet_identifier, int identifier_len)
{
    for (uint8_t i = 0; i < JMX_TABLE_COUNT; i++)
    {
        JMXActionTable* t = (JMXActionTable*)actionStore.actionTable[i];

        //if our key matches an item in the action table.
        if (strncmp(t->packet_identifier, (char *)packet_identifier, min(strlen(t->packet_identifier), identifier_len)) == 0)
            return i;
    }

    return STATUS_ERROR;
}

char jmx_previous(void)
{
    return previous;
}


void jmx_send_string(char* identifier)
{
    user_putc('"');
    for (uint8_t i = 0; i < strlen(identifier); i++)
        user_putc(identifier[i]);
    user_putc('"');
}

/**
  * marshalls a given buffer into a serial stream based upon the given identifier.
  *
  * @param identifier the actionTable identifier.
  *
  * @param data the buffer which can be mapped by the given actionTable identifier.
  *
  * @return STATUS_ERROR if the identifier doesn't exist, data is null. Or STATUS_SUCCESS if the send was successful.
  */
  int jmx_send(const char* identifier, void* data)
  {
      int table_index = jmx_get_table_entry_index((char *)identifier, strlen(identifier));

      if (table_index == STATUS_ERROR || data == NULL)
          return STATUS_ERROR;

      JMXActionTable* t = (JMXActionTable*)actionStore.actionTable[table_index];

      user_putc((char)JMX_ESCAPE_CHAR);

      user_putc((char)OBJECT_START);

      jmx_send_string((char *)t->packet_identifier);
      user_putc((char)FIELD_SEPARATOR_TOKEN);
      user_putc((char)OBJECT_START);

      for (uint8_t i = 0; i < JMX_ACTION_COUNT; i++)
      {
          JMXActionItem* a = &t->actions[i];
          if (a->type > 0 && a->key != NULL)
          {
              uint8_t* content = (uint8_t *)data + a->offset;

              //check if we have data...
              if (a->type == T_STATE_DYNAMIC_STRING && *((char **)content) == NULL)
                  continue;

              //add a comma if applicable!
              if (i > 0)
                  user_putc((char)PAIR_SEPARATOR_TOKEN);

              jmx_send_string((char *)a->key);

              user_putc((char)FIELD_SEPARATOR_TOKEN);

              if (a->type == T_STATE_STRING)
                  jmx_send_string((char*)content);
              else if(a->type == T_STATE_DYNAMIC_STRING)
                  jmx_send_string(*((char **)content));
              else if (a->type == T_STATE_NUMBER)
              {
                  char result[MAX_JSON_NUMBER];
                  int number;
                  memcpy(&number, content, sizeof(int));
                  itoa(number, result);

                  for (uint8_t j = 0; j < strlen(result); j++)
                      user_putc(result[j]);
              }
          }
      }

      user_putc((char)OBJECT_END);
      user_putc((char)OBJECT_END);

      user_putc((char)JMX_END_CHAR);

      return STATUS_SUCCESS;
}


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
int jmx_configure_buffer(const char* identifier, void* data)
{
    int table_index = jmx_get_table_entry_index((char *)identifier, strlen(identifier));

    if (table_index < 0 || jmx_state & J_STATE_USER_BUFFER || data == NULL)
        return STATUS_ERROR;

    JMXActionTable* t = (JMXActionTable*)actionStore.actionTable[table_index];

    *t->pointer_base = data;
    memset(*t->pointer_base, 0, t->struct_size);

    jmx_state |= J_STATE_USER_BUFFER;

    return STATUS_OK;
}

/**
  * An internal function used to configure buffers after a packet has been detected, and has been matched to an
  * appropriate JMXActionTable.
  */
int jmx_detect_key()
{
    if (jmx_action_index < 0)
        return STATUS_ERROR;

    JMXActionTable* t = (JMXActionTable*)actionStore.actionTable[jmx_action_index];

    int ret = jmx_get_action_index(t, (char *)jmx_key_buffer, jmx_key_offset);

    //if it's not in our action table, ignore.
    if (ret < 0)
        return STATUS_OK;

    JMXActionItem* a = &t->actions[ret];

    void* base = *t->pointer_base;

    // if we have already got a user call
    if (base == NULL)
    {
        *t->pointer_base = (void *)malloc(t->struct_size);
        base = *t->pointer_base;
        memset(*t->pointer_base, 0, t->struct_size);
    }

    //determine the type of this action item
    if (a->type == T_STATE_STRING)
    {
        jmx_data_buffer = (uint8_t *)base + a->offset;
        jmx_data_size = a->buffer_size;
    }
    else if (a->type == T_STATE_NUMBER)
    {
        //malloc a buffer max 12 character for a 'c-style' integer.
        jmx_data_buffer = (uint8_t *)malloc(MAX_JSON_NUMBER);
        jmx_data_size = MAX_JSON_NUMBER;
    }
    else if (a->type == T_STATE_DYNAMIC_STRING)
    {
        int16_t fieldLen = 0;

        // if the state is T_STATE_DYNAMIC_STRING, the action's buffer size is actually an index into the action table.
        if (a->buffer_size > JMX_ACTION_COUNT)
            return STATUS_ERROR;

        JMXActionItem* dependency = &t->actions[a->buffer_size];

        // if the data pointed to by this action is not an int, we can't accept that dependency.
        if (dependency->type != T_STATE_NUMBER)
            return STATUS_ERROR;

        //get the value pointed to by our action
        memcpy(&fieldLen, (uint8_t *)base + dependency->offset, sizeof(fieldLen));

        if (!fieldLen)
            return STATUS_ERROR;

        //increment to account for a NULL terminator.
        fieldLen += 1;

        char* dynamicData = (char *)malloc(fieldLen);

        char** bufferPointer = (char **)((char *)base + a->offset);

        *bufferPointer = dynamicData;

        jmx_data_buffer = (uint8_t *)dynamicData;
        jmx_data_size = fieldLen;
    }

    jmx_data_offset = 0;
    memset(jmx_data_buffer, 0, jmx_data_size);

    return STATUS_OK;
}

/**
  * Resets the currently used buffers dependent on the booleans passed to this member function.
  *
  * @param invoke used to determine if the equivalent JMXActionTable function should be invoked.
  *
  * @param resetAll A more vigirous reset process, where all buffers are freed.
  */
void jmx_reset_buffers(bool invoke, bool resetAll)
{
    if (jmx_action_index >= 0)
    {
        JMXActionTable* t = (JMXActionTable*)actionStore.actionTable[jmx_action_index];

        if (invoke)
        {
            if (!(jmx_state & J_STATE_USER_BUFFER))
            {
                if (t->fp && *t->pointer_base)
                    t->fp(*t->pointer_base);
                else
                    t->fp(NULL);
            }
            else
                jmx_packet_received((char *)t->packet_identifier);
        }

        if (*t->pointer_base && resetAll)
        {
            if (!(jmx_state & J_STATE_USER_BUFFER))
            {
                //free any dynamic buffers
                for (uint8_t i = 0; i < JMX_ACTION_COUNT; i++)
                {
                    JMXActionItem* a = &t->actions[i];

                    if (a->type != T_STATE_DYNAMIC_STRING || a->key == NULL)
                        continue;

                    char** bufferPointer = (char **)((char *)*t->pointer_base + a->offset);

                    if (*bufferPointer != NULL)
                    {
                        free(*bufferPointer);
                        *bufferPointer = NULL;
                    }
                }
                free(*t->pointer_base);
                *t->pointer_base = NULL;
            }

            if(invoke)
                jmx_state &= ~J_STATE_USER_BUFFER;
        }
    }

    memset(jmx_key_buffer, 0, jmx_key_offset);

    jmx_data_offset = 0;
    jmx_key_offset = 0;

    jmx_data_size = 0;
}

/**
  * Resets the state of the parser, and resets the buffers.
  *
  * @param error If the parser was successful or not. If successful, it will call jmx_reset_buffers with the invoke boolean set.
  *
  * @return PARSER_ERROR or PARSER_SUCCESS based on error.
  */
int jmx_reset(bool error)
{
    jmx_parser_state = P_STATE_NONE;
    jmx_token_state = T_STATE_NONE;

    jmx_reset_buffers(!error, true);

    jmx_action_index = -1;

    previous = 0;

    return (error) ? STATUS_ERROR : STATUS_SUCCESS;
}

/**
  * A simple initialisation call that sets our pointer values to well known c constants.
  */
void jmx_init(void)
{
    if (jmx_state & J_STATE_INIT)
        return;

    jmx_state |= J_STATE_INIT;

    jmx_data_buffer = NULL;
    jmx_key_buffer = NULL;

    jmx_reset(true);
}

/**
  * Accepts a single character and updates the state of this instance of JMX parser.
  *
  * @param c the character to process
  *
  * @return PARSER_OK if the parser has moved to a valid state, PARSER_ERROR if the parser has moved to an invalid state
  *            and PARSER_SUCCESS if the JSON packet has been processed successfully.
  *
  * @note On PARSER_ERROR, all state for the parser will be reset, thus you will have to parse from the beginning of the next packet.
  */
int jmx_parse(char c)
{
    // a special case for the scenario where we have a streamed buffer after a JMX sequence
    if (jmx_token_state & T_STATE_STREAM_BUFFER)
    {
        jmx_data_buffer[jmx_data_offset++] = c;

        //success!
        if (jmx_data_offset >= jmx_data_size)
            return jmx_reset(false);

        return STATUS_OK;
    }

    switch (c)
    {
        case JMX_END_CHAR:
            if (previous == OBJECT_END && (jmx_token_state == T_STATE_NONE || (jmx_token_state & T_STATE_NUMBER)))
            {
                JMXActionTable* t = (JMXActionTable*)actionStore.actionTable[jmx_action_index];

                //determine if this packet anticpates a stream after it...
                if (t->packet_type == BUFFERED_ACTION)
                {
                    JMXActionItem* a = NULL;
                    for (int i = 0; i < JMX_ACTION_COUNT; i++)
                        if (t->actions[i].type == T_STATE_STREAM_BUFFER)
                        {
                            a = &t->actions[i];
                            break;
                        }

                    // if we have found a valid stream_buffer entry, determine it's size based on its dependency.
                    if (a)
                    {
                        int fieldLen = 0;

                        // if the state is T_STATE_DYNAMIC_STRING, the action's buffer size is actually an index into the action table.
                        if (a->buffer_size > JMX_ACTION_COUNT)
                            return jmx_reset(true);

                        JMXActionItem* dependency = &t->actions[a->buffer_size];

                        // if the data pointed to by this action is not an int, we can't accept that dependency.
                        if (dependency->type != T_STATE_NUMBER)
                            return jmx_reset(true);

                        uint8_t* base = (uint8_t *)*t->pointer_base;

                        //get the value pointed to by our action
                        memcpy(&fieldLen, base + dependency->offset, sizeof(fieldLen));

                        uint8_t** buffer_pointer = (uint8_t**)(base + a->offset);

                        // we should get fieldLen bytes of data after this packet, set our parser state accordingly
                        if (fieldLen && *buffer_pointer)
                        {
                            jmx_token_state |= T_STATE_STREAM_BUFFER;
                            jmx_data_buffer = *buffer_pointer;
                            jmx_data_size = fieldLen;
                            jmx_data_offset = 0;
                            return STATUS_OK;
                        }
                    }
                }

                return jmx_reset(false);
            }
            else
                return jmx_reset(true);

        case JMX_ESCAPE_CHAR:

            //if we are here, and processing a packet, we do not expect an escape character...
            if (jmx_token_state == T_STATE_NONE && jmx_parser_state == P_STATE_NONE)
                previous = c;
            else
                return jmx_reset(true);

            return STATUS_OK;
        case OBJECT_START:
            if (previous == JMX_ESCAPE_CHAR && jmx_token_state == T_STATE_NONE && jmx_parser_state == P_STATE_NONE)
            {
                if (!jmx_key_buffer)
                {
                    jmx_key_buffer = (uint8_t *)malloc(KEY_BUFFER_SIZE);

                    if (!jmx_key_buffer)
                        return jmx_reset(true);

                    memset(jmx_key_buffer, 0, KEY_BUFFER_SIZE);
                }
            }
            else if (previous == FIELD_SEPARATOR_TOKEN  && (jmx_parser_state & P_STATE_VALUE) && !(jmx_parser_state & P_STATE_MAX_OBJECT))
            {
                // look up our key in our action table.
                // set our packet state if there's a match
                int ret = jmx_get_table_entry_index((char *)jmx_key_buffer, jmx_key_offset);

                if (ret >= 0)
                {
                    jmx_action_index = ret;

                    JMXActionTable* t = (JMXActionTable*)actionStore.actionTable[ret];

                    if (*t->pointer_base != NULL)
                        jmx_state |= J_STATE_USER_PACKET;
                }

                jmx_parser_state |= P_STATE_MAX_OBJECT;

                // we have entered a new detection phase, jmx_reset.
                jmx_reset_buffers(false, false);
            }
            else
                return jmx_reset(true);

            jmx_parser_state &= ~P_STATE_VALUE;
            jmx_parser_state |= P_STATE_KEY;

            previous = c;
            return STATUS_OK;

        case FIELD_SEPARATOR_TOKEN:
            if (jmx_parser_state & P_STATE_KEY)
            {
                jmx_parser_state &= ~P_STATE_KEY;
                jmx_parser_state |= P_STATE_VALUE;

                jmx_detect_key();
            }
            else if (jmx_parser_state & P_STATE_VALUE)
            {
                jmx_parser_state &= ~P_STATE_VALUE;
                jmx_parser_state |= P_STATE_KEY;
            }
            else
                return jmx_reset(true);

            //if we are still in the string state, an error has occurred...
            if (jmx_token_state & T_STATE_STRING)
                return jmx_reset(true);

            previous = c;
            return STATUS_OK;
        case STRING_TOKEN:

            if (jmx_token_state == T_STATE_NONE && (jmx_parser_state & P_STATE_KEY || jmx_parser_state & P_STATE_VALUE))
                jmx_token_state |= T_STATE_STRING;
            else if (jmx_token_state & T_STATE_STRING)
                jmx_token_state = T_STATE_NONE;
            else
                return jmx_reset(true);

            previous = c;
            return STATUS_OK;

        case OBJECT_END:
            jmx_parser_state &= ~P_STATE_MAX_OBJECT;
        case PAIR_SEPARATOR_TOKEN:
            //if we are still in the string state an error has occurred.
            if (jmx_token_state & T_STATE_STRING)
                return jmx_reset(true);

            // we need to convert our char* data to an int and store it, then free the buffer.
            if (jmx_token_state & T_STATE_NUMBER && jmx_data_buffer)
            {
                JMXActionTable* t = (JMXActionTable*)actionStore.actionTable[jmx_action_index];
                int actionIndex = jmx_get_action_index(t, (char *)jmx_key_buffer, jmx_key_offset);
                JMXActionItem* a = &t->actions[actionIndex];

                int res = atoi((char *)jmx_data_buffer);

                char* dest = (char *)(*t->pointer_base) + a->offset;

                memcpy(dest, &res, sizeof(int));

                free(jmx_data_buffer);
            }

            jmx_reset_buffers(false, false);
            jmx_data_buffer = NULL;

            jmx_token_state = T_STATE_NONE;

            jmx_parser_state &= ~P_STATE_VALUE;
            jmx_parser_state |= P_STATE_KEY;
            previous = c;

            return STATUS_OK;

        case WHITE_SPACE:
            if (jmx_token_state == T_STATE_NONE || jmx_token_state & T_STATE_NUMBER)
                return STATUS_OK;

        default:
            if (previous == FIELD_SEPARATOR_TOKEN )
            {
                if ((!IS_DIGIT(c) && c != '-') || jmx_token_state != T_STATE_NONE)
                    return jmx_reset(true);

                jmx_token_state = T_STATE_NUMBER;
            }

            // we should never receive escaped slip characters in our json stream,
            // however this is a byte-by-byte parser, and as such, users may use
            // this class as a mechanism for detecting escaped slip characters
            // in streams.
            if (previous == JMX_ESCAPE_CHAR && is_slip_character(c))
            {
                jmx_reset(true);
                return STATUS_SLIP_ESC;
            }

            previous = c;
    }

    if (jmx_token_state & T_STATE_STRING)
    {
        if (jmx_parser_state & P_STATE_KEY && jmx_key_offset < KEY_BUFFER_SIZE)
            jmx_key_buffer[jmx_key_offset++] = c;
        // -1 for null terminator...
        else if (jmx_parser_state & P_STATE_VALUE && jmx_data_offset < (jmx_data_size - 1))
                jmx_data_buffer[jmx_data_offset++] = c;
    }
    else if (jmx_token_state & T_STATE_NUMBER && (IS_DIGIT(c) || (c == '-' && jmx_data_offset == 0)) && jmx_parser_state & P_STATE_VALUE)
    {
        // -1 for null terminator...
        if (jmx_data_offset < (jmx_data_size - 1))
            jmx_data_buffer[jmx_data_offset++] = c;
    }
    else
        return jmx_reset(true);

    return STATUS_OK;
}

/**
  * Accepts a single character and updates the state of this instance of JMX parser, and also maintains a "serial_state" varaible
  *
  * @param c the character to process
  *
  * @return -1 if the parser consumed the character, 0 if the parser has validated a packet, 1 indicates that the given character should be forwarded
  *            2 indicates that the previous character, and the given character should be forwarded.
  */
int jmx_state_track(char c)
{
    if (!(jmx_state & J_STATE_INIT))
        return 1;

    if (c == JMX_ESCAPE_CHAR || serial_state == STATE_PROCESSING)
    {
        int ret = jmx_parse(c);

        // if our parser is ok, and we are not processing, enter a parsing state.
        if (ret == STATUS_OK && serial_state != STATE_PROCESSING)
            serial_state = STATE_PROCESSING;

        // we have detected a slip escape sequence, or an error has occurred, tell
        // the client app send the previous char as well as the given one
        if (ret == STATUS_SLIP_ESC || ret == STATUS_ERROR)
        {
            serial_state = STATE_IDLE;
            return 2;
        }

        // the parser has signified the end of a packet, or an error has occurred.
        if (ret == STATUS_SUCCESS)
        {
            serial_state = STATE_IDLE;
            return 0;
        }

        return -1;
    }

    return 1;
}
