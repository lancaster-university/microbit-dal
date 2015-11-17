#include <string.h>
#include <stdlib.h>
#include "mbed.h"
#include "MicroBit.h"

static const char empty[] __attribute__ ((aligned (4))) = "\xff\xff\0\0\0";

/**
  * Internal constructor helper.
  * Configures this ManagedString to refer to the static EmptyString
  */
void ManagedString::initEmpty()
{
    ptr = (StringData*)(void*)empty;
}

/**
  * Internal constructor helper.
  * creates this ManagedString based on a given null terminated char array.
  */
void ManagedString::initString(const char *str)
{   
    // Initialise this ManagedString as a new string, using the data provided.
    // We assume the string is sane, and null terminated.
    int len = strlen(str);
    ptr = (StringData *) malloc(4+len+1);
    ptr->init();
    ptr->len = len;
    memcpy(ptr->data, str, len+1);
}

/**
  * Constructor. 
  * Create a managed string from a specially prepared string literal. It will ptr->incr().
  *
  * @param ptr The literal - first two bytes should be 0xff, then the length in little endian, then the literal. The literal has to be 4-byte aligned.
  * 
  * Example:
  * @code 
  * static const char hello[] __attribute__ ((aligned (4))) = "\xff\xff\x05\x00" "Hello";
  * ManagedString s((StringData*)(void*)hello);
  * @endcode
  */    
ManagedString::ManagedString(StringData *p)
{
    ptr = p;
    ptr->incr();
}

/**
  * Get current ptr, do not decr() it, and set the current instance to empty string.
  * This is to be used by specialized runtimes which pass StringData around.
  */
StringData* ManagedString::leakData()
{
    StringData *res = ptr;
    initEmpty();
    return res;
}

/**
  * Constructor. 
  * Create a managed string from a given integer.
  *
  * @param value The integer from which to create the ManagedString
  * 
  * Example:
  * @code 
  * ManagedString s(20);
  * @endcode
  */      
ManagedString::ManagedString(const int value)
{
    char str[12];
        
    itoa(value, str);
    initString(str);    
}

/**
  * Constructor. 
  * Create a managed string from a given char.
  *
  * @param value The char from which to create the ManagedString
  * 
  * Example:
  * @code 
  * ManagedString s('a');
  * @endcode
  */      
ManagedString::ManagedString(const char value)
{
    char str[2] = {value, 0};
    initString(str); 
}


/**
  * Constructor. 
  * Create a managed string from a pointer to an 8-bit character buffer.
  * The buffer is copied to ensure sane memory management (the supplied
  * character buffer may be decalred on the stack for instance).
  *
  * @param str The character array on which to base the new ManagedString.
  */    
ManagedString::ManagedString(const char *str)
{
    // Sanity check. Return EmptyString for anything distasteful
    if (str == NULL || *str == 0)
    {
        initEmpty();
        return;
    }
    
    initString(str);
}

ManagedString::ManagedString(const ManagedString &s1, const ManagedString &s2)
{
    // Calculate length of new string.
    int len = s1.length() + s2.length();

    // Create a new buffer for holding the new string data.
    ptr = (StringData*) malloc(4+len+1);
    ptr->init();
    ptr->len = len;

    // Enter the data, and terminate the string.
    memcpy(ptr->data, s1.toCharArray(), s1.length());
    memcpy(ptr->data + s1.length(), s2.toCharArray(), s2.length());
    ptr->data[len] = 0;
}


/**
  * Constructor. 
  * Create a managed string from a pointer to an 8-bit character buffer of a given length.
  * The buffer is copied to ensure sane memory management (the supplied
  * character buffer may be declared on the stack for instance).
  *
  * @param str The character array on which to base the new ManagedString.
  * @param length The number of characters to use.
  *
  * Example:
  * @code 
  * ManagedString s("abcdefg",7); 
  * @endcode
  */  
ManagedString::ManagedString(const char *str, const int16_t length)
{
    // Sanity check. Return EmptyString for anything distasteful
    if (str == NULL || *str == 0 || (uint16_t)length > strlen(str)) // XXX length should be unsigned on the interface
    {
        initEmpty();
        return;
    }

    
    // Allocate a new buffer, and create a NULL terminated string.
    ptr = (StringData*) malloc(4+length+1);
    ptr->init();
    // Store the length of the new string
    ptr->len = length;
    memcpy(ptr->data, str, length);
    ptr->data[length] = 0;
}

/**
  * Copy constructor. 
  * Makes a new ManagedString identical to the one supplied. 
  * Shares the character buffer and reference count with the supplied ManagedString.
  *
  * @param s The ManagedString to copy.
  * 
  * Example:
  * @code 
  * ManagedString s("abcdefg");
  * ManagedString p(s);
  * @endcode
  */
ManagedString::ManagedString(const ManagedString &s)
{
    ptr = s.ptr;
    ptr->incr();
}


/**
  * Default constructor. 
  *
  * Create an empty ManagedString. 
  *
  * Example:
  * @code 
  * ManagedString s();
  * @endcode
  */
ManagedString::ManagedString()
{
    initEmpty();
}

/**
  * Destructor. 
  *
  * Free this ManagedString, and decrement the reference count to the
  * internal character buffer. If we're holding the last reference,
  * also free the character buffer and reference counter.
  */
ManagedString::~ManagedString()
{
    ptr->decr();
}

/**
  * Copy assign operation. 
  *
  * Called when one ManagedString is assigned the value of another.
  * If the ManagedString being assigned is already refering to a character buffer,
  * decrement the reference count and free up the buffer as necessary.
  * Then, update our character buffer to refer to that of the supplied ManagedString,
  * and increase its reference count.
  *
  * @param s The ManagedString to copy.
  *
  * Example:
  * @code 
  * ManagedString s("abcd");
  * ManagedString p("efgh");
  * p = s   // p now points to s, s' ref is incremented 
  * @endcode
  */
ManagedString& ManagedString::operator = (const ManagedString& s)
{
    if (this->ptr == s.ptr)
        return *this; 

    ptr->decr();
    ptr = s.ptr;
    ptr->incr();

    return *this;
}

/**
  * Equality operation.
  *
  * Called when one ManagedString is tested to be equal to another using the '==' operator.
  *
  * @param s The ManagedString to test ourselves against.
  * @return true if this ManagedString is identical to the one supplied, false otherwise.
  *
  * Example:
  * @code 
  * ManagedString s("abcd");
  * ManagedString p("efgh");
  * 
  * if(p==s)
  *     print("We are the same!");
  * else
  *     print("We are different!"); //p is not equal to s - this will be called
  * @endcode
  */
bool ManagedString::operator== (const ManagedString& s)
{
    return ((length() == s.length()) && (strcmp(toCharArray(),s.toCharArray())==0));
}

/**
  * Inequality operation.
  *
  * Called when one ManagedString is tested to be less than another using the '<' operator.
  *
  * @param s The ManagedString to test ourselves against.
  * @return true if this ManagedString is alphabetically less than to the one supplied, false otherwise.
  *
  * Example:
  * @code 
  * ManagedString s("a");
  * ManagedString p("b");
  * 
  * if(s<p)
  *     print("a is before b!"); //a is before b
  * else
  *     print("b is before a!"); 
  * @endcode
  */
bool ManagedString::operator< (const ManagedString& s)
{
    return (strcmp(toCharArray(), s.toCharArray())<0);
}

/**
  * Inequality operation.
  *
  * Called when one ManagedString is tested to be greater than another using the '>' operator.
  *
  * @param s The ManagedString to test ourselves against.
  * @return true if this ManagedString is alphabetically greater than to the one supplied, false otherwise.
  *
  * Example:
  * @code 
  * ManagedString s("a");
  * ManagedString p("b");
  * 
  * if(p>a)
  *     print("b is after a!"); //b is after a
  * else
  *     print("a is after b!"); 
  * @endcode
  */
bool ManagedString::operator> (const ManagedString& s)
{
    return (strcmp(toCharArray(), s.toCharArray())>0);
}

/**
  * Extracts a ManagedString from this string, at the position provided.
  *
  * @param start The index of the first character to extract, indexed from zero.
  * @param length The number of characters to extract from the start position
  * @return a ManagedString representing the requested substring.
  *
  * Example:
  * @code 
  * ManagedString s("abcdefg");
  *
  * print(s.substring(0,2)) // prints "ab"
  * @endcode
  */ 
ManagedString ManagedString::substring(int16_t start, int16_t length)
{
    // If the parameters are illegal, just return a reference to the empty string.
    if (start >= this->length())
        return ManagedString(ManagedString::EmptyString);

    // Compute a safe copy length;
    length = min(this->length()-start, length);

    // Build a ManagedString from this.
    return ManagedString(toCharArray()+start, length);
}

/**
  * Concatenates this string with the one provided.
  *
  * @param s The ManagedString to concatenate.
  * @return a new ManagedString representing the joined strings.
  *
  * Example:
  * @code 
  * ManagedString s("abcd");
  * ManagedString p("efgh")
  *
  * print(s + p) // prints "abcdefgh"
  * @endcode
  */     
ManagedString ManagedString::operator+ (ManagedString& s)
{
    // If the other string is empty, nothing to do!
    if(s.length() == 0)
        return *this;

    if (length() == 0)
        return s;

    return ManagedString(*this, s);
}


/**
  * Provides a character value at a given position in the string, indexed from zero.
  *
  * @param index The position of the character to return.
  * @return the character at posisiton index, zero if index is invalid.
  *
  * Example:
  * @code 
  * ManagedString s("abcd");
  *
  * print(s.charAt(1)) // prints "b"
  * @endcode
  */     
char ManagedString::charAt(int16_t index)
{
    return (index >=0 && index < length()) ? ptr->data[index] : 0;
}

/**
  * Empty string constant literal
  */
ManagedString ManagedString::EmptyString((StringData*)(void*)empty);
