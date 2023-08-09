# dict
## Copyright
Public domain, 2023

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

### Keys and comparisons
Trivial fundamental types like `int`, `char`, and `float` can be used directly as keys without too much trouble.
```
cc0::dict<unsigned int, const char*> d; // Use unsigned integers as keys to access C strings.
```

When key types become too large, too complex, or reference data indirectly, the user may want to consider either constructing a dedicated key type which contains a hash of the data, or constructing a custom comparison function.
```
struct array
{
	const int *values;
	uint32_t size;
};

cc0::dict<array, float> d; // Use an array as key to access floating-point values. This may NOT work as expected.
```
Keys are compared using bytewise equality. This means that two keys referencing data indirectly may reference two different memory locations which both contain the same values. By default, the keys will be treated as inequal. If the user wishes that two keys referencing different memory locations but the same values to be treated as equal the user needs to hash the intended key, and provide the hash as key instead. The user may choose to use template specialization of the provided `cc0::key` type, or roll their own. In the example below the `cc0::key` is specialized:
```
struct array
{
	const int *values;
	uint32_t size;
};

template<>
class cc0::key<array>
{
	uint64_t k;

	key(const array &a) {
		cc0::fnv1a64 hasher;
		for (uint32_t i = 0; i < a.size; ++i) {
			hasher(a[i]);
		}
		hasher(a.size);
		k = hasher;
	}
};

cc0::dict<key<array>, float> d;
```
Note the use of `cc0::fnv1a64` which performs a bytewise ingestion of the input data. This hashing method, which is not cryptographically secure, comes with `dict`, but the user may use another hashing method or even implement their own key type altogether as long as valid key comparisons can be done in a bytewise fashion.

The `cc0::key` type already has a specialized 64-bit version to deal with plain C strings so the user will not need to take action if 64 bit FNV1a hashing is acceptable.

### Strings as keys
Oftentimes, global constant strings may be used as keys without much issue:
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
However, due to how the compiler decides to lay out memory, constant strings containing the same sequence of characters may not always be located on the same memory address. It is therefore recommended that dedicated key types are used instead of raw C strings. `dict` ships with a specialized version of `cc0::key` to deal with raw C strings:

```
#include "dict/dict.h"

int main()
{
	cc0::dict<cc0::key<const char*>,int> d;
	d("Hello") = 1;
	d("World") = 2;
	return 0;
}
```
This changes the behavior of the dictionary so that strings containing the same sequence of characters will be treated as equal regardless of if the two keys reference different memory locations.

### Checking for existence
Check if elements exist:
```
#include <iostream>
#include "dict/dict.h"

int main()
{
	cc0::dict<cc0::key<const char*>,int> d;
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

void nonconstant(cc0::dict<cc0::key<const char*>,int> &d)
{
	d("Hello") = 12; // Creates a key-value pair in the table if it does not already exist.
}

int constant(const cc0::dict<cc0::key<const char*>,int> &d)
{
	return d("World"); // Can not modify the dictionary, so instead it assumes that the value must already exist. If this is false the application will likely crash.
}

int main()
{
	cc0::dict<cc0::key<const char*>,int> d;
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
	cc0::dict<cc0::key<const char*>,int> d;
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

Remove FNV1a64 as an explicit package included in `dict` as this will clash with the `sum` library.
