#include <string.h>
#include <stdlib.h>
#include "MicroBitCompat.h"
#include "ManagedString.h"


/**
  * Internal constructor helper.
  * Configures this ManagedString to refer to the static EmptyString
  */
void ManagedString::initEmpty()
{
    data = ManagedString::EmptyString.data;
    ref = ManagedString::EmptyString.ref;
    len = ManagedString::EmptyString.len;

    (*ref)++;
}

/**
  * Internal constructor helper.
  * creates this ManagedString based on a given null terminated char array.
  */
void ManagedString::initString(const char *str)
{   
    // Initialise this ManagedString as a new string, using the data provided.
    // We assume the string is sane, and null terminated.
    len = strlen(str);
    data = (char *) malloc(len+1);
    memcpy(data, str, len+1);
    ref = (int16_t *) malloc(sizeof(int16_t));
    *ref = 1;
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
    if ((str == NULL || *str == 0) && this != &ManagedString::EmptyString)
    {
        initEmpty();
        return;
    }
    
    initString(str);
}

ManagedString::ManagedString(const ManagedString &s1, const ManagedString &s2)
{
    // Calculate length of new string.
    len = s1.len + s2.len;

    // Create a new buffer for holding the new string data.
    data = (char *) malloc(len+1);

    // Enter the data, and terminate the string.
    memcpy(data, s1.data, s1.len);
    memcpy(data + s1.len, s2.data, s2.len);
    data[len] = 0;

    // Initialise the ref count and we're done.
    ref = (int16_t *) malloc(sizeof(int16_t));
    *ref = 1;
}


/**
  * Constructor. 
  * Create a managed string from a pointer to an 8-bit character buffer of a given length.
  * The buffer is copied to ensure sane memory management (the supplied
  * character buffer may be declared on the stack for instance).
  *
  * @param str The character array on which to base the new ManagedString.
  * @param length The length of the character array
  *
  * Example:
  * @code 
  * ManagedString s("abcdefg",7); // this is generally used for substring... why not use a normal char * constructor?
  * @endcode
  */  
ManagedString::ManagedString(const char *str, const int16_t length)
{
    // Sanity check. Return EmptyString for anything distasteful
    if (str == NULL || *str == 0 || length > strlen(str))
    {
        initEmpty();
        return;
    }

    // Store the length of the new string
    len = length;
    
    // Allocate a new buffer, and create a NULL terminated string.
    data = (char *) malloc(len+1);
    memcpy(data, str, len);
    data[len] = 0;

    // Initialize a refcount and we're done.
    ref = (int16_t *) malloc(sizeof(int16_t));
    *ref = 1;
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
    data = s.data;
    ref = s.ref;
    len = s.len;

    (*ref)++;
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
    if(--(*ref) == 0)
    {
        free(data);
        free(ref);
    }
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
    if(this == &s)
        return *this; 

    if(--(*ref) == 0)
    {
        free(data);
        free(ref);
    }
    
    data = s.data;
    ref = s.ref;
    len = s.len;
    (*ref)++;

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
    return ((len == s.len) && (memcmp(data,s.data,len)==0));    
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
    return (memcmp(data, s.data,min(len,s.len))<0);
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
    return (memcmp(data, s.data,min(len,s.len))>0); 
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
    if (start >= len)
        return ManagedString(ManagedString::EmptyString);

    // Compute a safe copy length;
    length = min(len-start, length);

    // Build a ManagedString from this.
    return ManagedString(data+start, length);
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
    if(s.len == 0)
        return *this;

    if (len == 0)
        return s;
        
    if(s == ManagedString::EmptyString)
        return *this;
        
    if(*this == ManagedString::EmptyString)
        return s;

    return ManagedString(data, s.data);
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
    return (index >=0 && index < len) ? data[index] : 0;
}

/**
  * Provides an immutable 8 bit wide haracter buffer representing this string.
  *
  * @return a pointer to the charcter buffer.
  */    
const char *ManagedString::toCharArray()
{
    return data;
}

/**
  * Determines the length of this ManagedString in characters.
  *
  * @return the length of the string in characters.
  *
  * Example:
  * @code 
  * ManagedString s("abcd");
  *
  * print(s.length()) // prints "4"
  * @endcode
  */ 
int16_t ManagedString::length()
{
    return len;
}

/**
  * Empty string constant literal
  */
ManagedString ManagedString::EmptyString("\0");


