/*
The MIT License (MIT)

Copyright (c) 2017 Lancaster University.

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

/**
  * This file contains functions used to maintain compatability and portability.
  * It also contains constants that are used elsewhere in the DAL.
  */
#include "MicroBitUtil.h"

KeyValueTableEntry* KeyValueTable::find(const uint32_t key) const
{
	// Now find the nearest sample range to that specified.
	KeyValueTableEntry *p = (KeyValueTableEntry *)data + (length - 1);
	KeyValueTableEntry *result = p;

	while (p >= (KeyValueTableEntry *)data)
	{
		if (p->key < key)
			break;

		result = p;
		p--;
	}

	return result;
}


uint32_t KeyValueTable::get(const uint32_t key) const
{
	return find(key)->value;
}

uint32_t KeyValueTable::getKey(const uint32_t key) const
{
	return find(key)->key;
}

bool KeyValueTable::hasKey(const uint32_t key) const
{
	return (find(key)->key == key);
}


