/**
  * Compatibility / portability funcitons and constants for the MicroBit DAL.
  */
#include "mbed.h"


/**
  * Performs an in buffer reverse of a given char array
  * @param s the char* to reverse.
  * @return the reversed char*
  */
void string_reverse(char *s) 
{
    //sanity check...
    if(s == NULL)
        return;
    
    char *j;
    int c;
 
    j = s + strlen(s) - 1;
    
    while(s < j) 
    {
        c = *s;
        *s++ = *j;
        *j-- = c;
    }
}

/**
  * Converts a given integer into a base 10 ASCII equivalent.
  *
  * @param n The number to convert.
  * @param s Pointer to a buffer in which to store the resulting string.
  */
void itoa(int n, char *s)
{
    int i = 0;
    int positive = (n >= 0);

    // Record the sign of the number,
    // Ensure our working value is positive.
    if (positive)
        n = -n;          

    // Calculate each character, starting with the LSB.    
    do {       
         s[i++] = abs(n % 10) + '0';
    } while (abs(n /= 10) > 0);
    
    // Add a negative sign as needed
    if (!positive)
        s[i++] = '-';
    
    // Terminate the string.
    s[i] = '\0';
    
    // Flip the order.
    string_reverse(s);
}

