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

#ifndef MANAGED_STRING_H
#define MANAGED_STRING_H

#include "MicroBitConfig.h"
#include "RefCounted.h"
#include "PacketBuffer.h"

struct StringData : RefCounted
{
    uint16_t len;
    char data[0];
};


/**
  * Class definition for a ManagedString.
  *
  * Uses basic reference counting to implement a copy-assignable, immutable string.
  *
  * This maps closely to the constructs found in many high level application languages,
  * such as Touch Develop.
  *
  * Written from first principles here, for several reasons:
  * 1) std::shared_ptr is not yet availiable on the ARMCC compiler
  *
  * 2) to reduce memory footprint - we don't need many of the other features in the std library
  *
  * 3) it makes an interesting case study for anyone interested in seeing how it works!
  *
  * 4) we need explicit reference counting to inter-op with low-level application langauge runtimes.
  *
  * 5) the reference counting needs to also work for read-only, flash-resident strings
  */
class ManagedString
{
    // StringData contains the reference count, the length, follwed by char[] data, all in one block.
    // When referece count is 0xffff, then it's read only and should not be counted.
    // Otherwise the block was malloc()ed.
    // We control access to this to proide immutability and reference counting.
    StringData *ptr;

    public:

    /**
      * Constructor.
      * Create a managed string from a specially prepared string literal.
      *
      * @param ptr The literal - first two bytes should be 0xff, then the length in little endian, then the literal. The literal has to be 4-byte aligned.
      *
      * @code
      * static const char hello[] __attribute__ ((aligned (4))) = "\xff\xff\x05\x00" "Hello";
      * ManagedString s((StringData*)(void*)hello);
      * @endcode
      */
    ManagedString(StringData *ptr);

    /**
      * Get current ptr, do not decr() it, and set the current instance to empty string.
      *
      * This is to be used by specialized runtimes which pass StringData around.
      */
    StringData *leakData();

    /**
      * Constructor.
      *
      * Create a managed string from a pointer to an 8-bit character buffer.
      *
      * The buffer is copied to ensure safe memory management (the supplied
      * character buffer may be declared on the stack for instance).
      *
      * @param str The character array on which to base the new ManagedString.
      *
      * @code
      * ManagedString s("abcdefg");
      * @endcode
      */
    ManagedString(const char *str);

    /**
      * Constructor.
      *
      * Create a managed string from a given integer.
      *
      * @param value The integer from which to create the ManagedString.
      *
      * @code
      * ManagedString s(20);
      * @endcode
      */
    ManagedString(const int value);


    /**
      * Constructor.
      * Create a managed string from a given char.
      *
      * @param value The character from which to create the ManagedString.
      *
      * @code
      * ManagedString s('a');
      * @endcode
      */
    ManagedString(const char value);

    /**
      * Constructor.
      * Create a ManagedString from a PacketBuffer. All bytes in the
      * PacketBuffer are added to the ManagedString.
      *
      * @param buffer The PacktBuffer from which to create the ManagedString.
      *
      * @code
      * ManagedString s = radio.datagram.recv();
      * @endcode
      */
    ManagedString(PacketBuffer buffer);

    /**
      * Constructor.
      * Create a ManagedString from a pointer to an 8-bit character buffer of a given length.
      *
      * The buffer is copied to ensure sane memory management (the supplied
      * character buffer may be declared on the stack for instance).
      *
      * @param str The character array on which to base the new ManagedString.
      *
      * @param length The length of the character array
      *
      * @code
      * ManagedString s("abcdefg",7);
      * @endcode
      */
    ManagedString(const char *str, const int16_t length);

    /**
      * Copy constructor.
      * Makes a new ManagedString identical to the one supplied.
      *
      * Shares the character buffer and reference count with the supplied ManagedString.
      *
      * @param s The ManagedString to copy.
      *
      * @code
      * ManagedString s("abcdefg");
      * ManagedString p(s);
      * @endcode
      */
    ManagedString(const ManagedString &s);

    /**
      * Default constructor.
      *
      * Create an empty ManagedString.
      *
      * @code
      * ManagedString s();
      * @endcode
      */
    ManagedString();

    /**
      * Destructor.
      *
      * Free this ManagedString, and decrement the reference count to the
      * internal character buffer.
      *
      * If we're holding the last reference, also free the character buffer.
      */
    ~ManagedString();

    /**
      * Copy assign operation.
      *
      * Called when one ManagedString is assigned the value of another.
      *
      * If the ManagedString being assigned is already referring to a character buffer,
      * decrement the reference count and free up the buffer as necessary.
      *
      * Then, update our character buffer to refer to that of the supplied ManagedString,
      * and increase its reference count.
      *
      * @param s The ManagedString to copy.
      *
      * @code
      * ManagedString s("abcd");
      * ManagedString p("efgh");
      * p = s   // p now points to s, s' ref is incremented
      * @endcode
      */
    ManagedString& operator = (const ManagedString& s);

    /**
      * Equality operation.
      *
      * Called when one ManagedString is tested to be equal to another using the '==' operator.
      *
      * @param s The ManagedString to test ourselves against.
      *
      * @return true if this ManagedString is identical to the one supplied, false otherwise.
      *
      * @code
      * MicroBitDisplay display;
      * ManagedString s("abcd");
      * ManagedString p("efgh");
      *
      * if(p == s)
      *     display.scroll("We are the same!");
      * else
      *     display.scroll("We are different!"); //p is not equal to s - this will be called
      * @endcode
      */
    bool operator== (const ManagedString& s);

    /**
      * Inequality operation.
      *
      * Called when one ManagedString is tested to be less than another using the '<' operator.
      *
      * @param s The ManagedString to test ourselves against.
      *
      * @return true if this ManagedString is alphabetically less than to the one supplied, false otherwise.
      *
      * @code
      * MicroBitDisplay display;
      * ManagedString s("a");
      * ManagedString p("b");
      *
      * if(s < p)
      *     display.scroll("a is before b!"); //a is before b
      * else
      *     display.scroll("b is before a!");
      * @endcode
      */
    bool operator< (const ManagedString& s);

    /**
      * Inequality operation.
      *
      * Called when one ManagedString is tested to be greater than another using the '>' operator.
      *
      * @param s The ManagedString to test ourselves against.
      *
      * @return true if this ManagedString is alphabetically greater than to the one supplied, false otherwise.
      *
      * @code
      * MicroBitDisplay display;
      * ManagedString s("a");
      * ManagedString p("b");
      *
      * if(p>a)
      *     display.scroll("b is after a!"); //b is after a
      * else
      *     display.scroll("a is after b!");
      * @endcode
      */
    bool operator> (const ManagedString& s);

    /**
      * Extracts a ManagedString from this string, at the position provided.
      *
      * @param start The index of the first character to extract, indexed from zero.
      *
      * @param length The number of characters to extract from the start position
      *
      * @return a ManagedString representing the requested substring.
      *
      * @code
      * MicroBitDisplay display;
      * ManagedString s("abcdefg");
      *
      * display.scroll(s.substring(0,2)) // displays "ab"
      * @endcode
      */
    ManagedString substring(int16_t start, int16_t length);

    /**
      * Concatenates two strings.
      *
      * @param lhs The first ManagedString to concatenate.
      * @param rhs The second ManagedString to concatenate.
      *
      * @return a new ManagedString representing the joined strings.
      *
      * @code
      * MicroBitDisplay display;
      * ManagedString s("abcd");
      * ManagedString p("efgh")
      *
      * display.scroll(s + p) // scrolls "abcdefgh"
      * @endcode
      */
    friend ManagedString operator+ (const ManagedString& lhs, const ManagedString& rhs);

    /**
      * Provides a character value at a given position in the string, indexed from zero.
      *
      * @param index The position of the character to return.
      *
      * @return the character at posisiton index, zero if index is invalid.
      *
      * @code
      * MicroBitDisplay display;
      * ManagedString s("abcd");
      *
      * display.scroll(s.charAt(1)) // scrolls "b"
      * @endcode
      */
    char charAt(int16_t index);


    /**
      * Provides an immutable 8 bit wide character buffer representing this string.
      *
      * @return a pointer to the character buffer.
      */
    const char *toCharArray() const
    {
        return ptr->data;
    }

    /**
      * Determines the length of this ManagedString in characters.
      *
      * @return the length of the string in characters.
      *
      * @code
      * MicroBitDisplay display;
      * ManagedString s("abcd");
      *
      * display.scroll(s.length()) // scrolls "4"
      * @endcode
      */
    int16_t length() const
    {
        return ptr->len;
    }

    /**
      * Empty String constant
      */
    static ManagedString EmptyString;

    private:

    /**
      * Internal constructor helper.
      *
      * Configures this ManagedString to refer to the static EmptyString
      */
    void initEmpty();

    /**
      * Internal constructor helper.
      *
      * Creates this ManagedString based on a given null terminated char array.
      */
    void initString(const char *str);

    /**
      * Private Constructor.
      *
      * Create a managed string based on a concat of two strings.
      * The buffer is copied to ensure sane memory management (the supplied
      * character buffer may be declared on the stack for instance).
      *
      * @param str1 The first string on which to base the new ManagedString.
      *
      * @param str2 The second string on which to base the new ManagedString.
      */
    ManagedString(const ManagedString &s1, const ManagedString &s2);

};

#endif
