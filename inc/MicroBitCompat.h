/**
  * Compatibility / portability funcitons and constants for the MicroBit DAL.
  */
  
#ifndef MICROBIT_COMPAT_H
#define MICROBIT_COMPAT_H

#define PI 3.14159265359

 /*! \
    \brief Utility functions. 
    
    Included here often to reduce the need to import a whole library for simple opertations.
    This helps to minimize our SRAM footprint.
  */
/*!
  \brief returns the smallest of the two numbers
  \param a the first number
  \param b the number to compare against a
  \return whichever value is the smallest
  */
inline int min(int a, int b)
{
    return (a < b ? a : b);
}

/*!
  \brief returns the biggest of the two numbers
  \param a the first number
  \param b the number to compare against a
  \return whichever value is the biggest
  */
inline int max(int a, int b)
{
    return (a > b ? a : b);
}

/*!
  \brief clears b number of bytes given the pointer a
  \param a the pointer to the beginning of the memory to clear
  \param b the number of bytes to clear.
  */
inline void *memclr(void *a, size_t b)
{
    return memset(a,0,b);
}

/*!
  \brief returns true if the character c lies between ascii values 48 and 57 inclusive.
  \param c the character to check
  \return a bool, true if it is a digit, false otherwise
  */
inline bool isdigit(char c)
{
    return (c > 47 && c < 58);
}

/*!
  \brief Performs an in buffer reverse of a given char array
  \param s the char* to reverse.
  \return the reversed char*
  */
void string_reverse(char *s);

/*!
  \briefConverts a given integer into a base 10 ASCII equivalent.
  \param n The number to convert.
  \param s Pointer to a buffer in which to store the resulting string.
  */
void itoa(int n, char *s);


#endif
