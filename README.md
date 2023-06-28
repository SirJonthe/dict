# arr
## Copyright
Copyright Jonathan Karlsson, 2023

github.com/SirJonthe

## About
`dict` is a minimalist library that introduces a dictionary, hash table, or map type to C++11.

## Design
`dict` is intended to be minimal. It does not depend on STL, nor any other external library. It is a header-only library. It contains only the minimum amount of functionality to provide a useful data structure.

## Usage
`dict` exposes only a few functions of the dictionary itself, mainly a way to add a key-value pair, retrieve a key-value pair, and remove a key-value pair.

There are caveats to using `dict`; The library does not provide the user with proper ways to generate keys. This is mainly due to limitations in C++ itself. The default way to generate keys from data structures is simply to convert a data structure into a string of bytes and treating that string as the key. This does not always lead to satisfactory results. For instance, two pointers pointing to separate locations in memory that otherwise mirror each other in terms of data will be treated as two distinct keys. This is obvious when using, for instance, std::string as a key type since two instances of std::string will be treated as two distinct keys regardless of the contents of the strings (conversely, due to how C and C++ treat constant strings, these can still be used as valid keys, see examples below).

The library does, however, give the user room to implement their own key type that they can convert their data types into, and/or use a custom key comparison function (see the dictionary template arguments).

## Building
`dict` is a header-only template library. Just include it in your code via `#include "dict/dict.h"` (depending on how you structure your source tree) and compile with at least C++11 compatibility on.

## Examples
### Create a dictionary
Create an empty dictionary:
```
#include "dict/dict.h"

int main()
{
	cc0::dict<int,int> d;
	return 0;
}
```

Create an dictionary with 2 elements:
```
#include "dict/dict.h"

int main()
{
	cc0::dict<int,int> d;
	d(123) = 1;
	d(345) = 2;
	return 0;
}
```
Note that there is a difference between the [] and () operators. () will guarantee that a value is available to be returned. If it does not already exist, a new value is created and returned by reference. The [] operator, on the other hand, will return null if the key is not present in the dictionary.

### Strings as keys
Using global constant strings as keys:
```
#include "dict/dict.h"

int main()
{
	cc0::dict<const char*,int> d;
	d("Hello") = 1;
	d("World") = 2;
	return 0;
}
```
Note that this will mainly work for constant global strings. Dynamically allocated strings containing the same data as constant global strings will not work as the default behavior is to use the bit pattern in the pointer of the string as the key, rather than the contents of the string itself. C and C++ just happens to store constant strings globally in the binary, meaning repeated use of the same constant inline string will yield the same pointer.

### Checking for existence
Check if elements exist:
```
#include <iostream>
#include "dict/dict.h"

int main()
{
	cc0::dict<const char*,int> d;
	d("Hello") = 1;
	if (d["Hello"] != nullptr) {
		std::cout << "\"Hello\" exists and contains " << *d["Hello"] << std::endl;
	}
	if (d["World"] == nullptr) {
		std::cout << "\"World\" does not exist" << std::endl;
	}
	return 0;
}
```

There is a slight difference in behavior between the const version and non-const version of the () operator:
```
#include "dict/dict.h"

void nonconstant(cc0::dict<const char*,int> &d)
{
	d("Hello") = 12; // Creates a key-value pair in the table if it does not already exist.
}

int constant(const cc0::dict<const char*,int> &d)
{
	return d("World"); // Can not modify the dictionary, so instead it assumes that the value must already exist. If this is false the application will likely crash.
}

int main()
{
	cc0::dict<const char*,int> d;
	nonconstant(d);
	constant(d);
	return 0;
}
```
The non-constant version of the () operator will guarantee that a valid value reference is returned, i.e. it will allocate memory if the key-value pair does not already exist. The constant version of the () operator obviously can not allocate memory since that would violate it being constant. This means that using the constant () operator with a key that does not exist will yield an invalid dereference, and will likely crash the application. For testing if a value exists it is recommended to use the [] operator and checking for null.

### Erasing elements
Erase elements:
```
#include <iostream>
#include "dict/dict.h"

int main()
{
	cc0::dict<const char*,int> d;
	d("Hello") = 1;
	d.remove("Hello");
	if (d["Hello"] == nullptr) {
		std::cout << "\"Hello\" has been removed" << std::endl;
	}
	d.remove("World"); // Safe. Does nothing.
	return 0;
}
```
Note that is is always safe to remove elements even if the key-value pair marked for erasure is not located in the dictionary.

### Advanced key usage
The default behavior of the library is to treat the key data type as a string of bytes and using the bit patters in the bytes as keys. This has some drawbacks, namely that keys that are, or contain, pointers to data will not behave properly as they can be treated as distinct keys despite pointing to identical data in different memory locations. Because of this it may be necessary for the developer to create their own hash function to generate keys. Below is a highly simplified example of generating keys (which should not be used for production under any circumstances):
```
#include <iostream>
#include "dict/dict.h"

// Basic key type.
template <int size>
struct key
{
	uint8_t bytes[size];
};

// Some object that we want to be able to convert into a key.
class array
{
private:
	int *m_data;
	int  m_size;

public:
	array( void ) : m_data(nullptr), m_size(0) {}
	~array( void ) { delete [] m_data; }
	array(const array &a) : array() {
		create(a.m_size);
		for (int i = 0; i < m_size; ++i) {
			m_data[i] = a.m_data[i];
		}
	}

	void destroy( void ) {
		delete [] m_data;
		m_data = nullptr;
		m_size = 0;
	}

	void create(int size) {
		destroy();
		m_data = new int[size];
		m_size = size;
	}

	int size( void ) const { return m_size; }

	operator int*( void ) { return m_data };
	operator const int*( void ) { return m_data };

	operator key<4>( void ) const {
		// This is an extremely rudimentary key generator that should not be used in any practical circumstances.
		key<4> k = { 0, 0, 0, 0 };
		for (int i = 0; i < m_size; ++i) {
			for (int n = 0; n < sizeof(int); ++n) {
				k.bytes[i%4] ^= reinterpret_cast<const uint8_t*>(m_data + i)[n];
			}
		}
		return k;
	}
};

int main()
{
	array a1;
	a1.create(2);
	a1[0] = 1;
	a1[1] = 2;
	
	array a2(a1);

	cc0::dict<key<4>,int> d;
	d(a1) = 123;
	if (d[a2] != nullptr) {
		std::cout << "\"a1\" and a2 point to same element " << *d[a1] << " " << *d[a2] << std::endl;
	}
	return 0;
}
```

## Future work
The current implementation of the dictionary may reserve quite a bit of memory. As elements are removed from the dictionary, the dictionary memory usage does not reduce. This is a clear area of improvement moving forward.