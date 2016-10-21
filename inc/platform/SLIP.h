#define SLIP_END            '\xc0'
#define SLIP_ESC            '\xdb'
#define SLIP_ESC_END        '\xdc'
#define SLIP_ESC_ESC        '\xdd'

enum SLIPCharacters {
    END = 1,
    ESC,
    ESC_END,
    ESC_ESC
};

inline int is_slip_character(char c)
{
    switch (c)
    {
        case SLIP_END:
            return END;
        case SLIP_ESC:
            return ESC;
        case SLIP_ESC_END:
            return ESC_END;
        case SLIP_ESC_ESC:
            return ESC_ESC;
    }

    return 0;
}
