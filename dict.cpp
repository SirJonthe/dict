/// @file
/// @author github.com/SirJonthe
/// @date 2023
/// @copyright Public domain.
/// @license CC0 1.0

#include "dict.h"

//
// global
//

uint64_t cc0::internal::str_count(const char *s)
{
	uint64_t i = 0;
	while (s[i] != 0) {
		++i;
	}
	return i;
}

//
// fnv1a64
//

void cc0::fnv1a64::ingest(const void *in, uint64_t num_bytes)
{
	const uint8_t *ptr = reinterpret_cast<const uint8_t*>(in);
	for (uint64_t i = 0; i < num_bytes; ++i) {
		h ^= uint64_t(ptr[i]);
		h *= 0x100000001b3ULL;
	}
}

cc0::fnv1a64::fnv1a64( void ) : h(0xcbf29ce484222325ULL)
{}

cc0::fnv1a64::fnv1a64(const void *in, uint64_t num_bytes) : fnv1a64()
{
	ingest(in, num_bytes);
}

cc0::fnv1a64 &cc0::fnv1a64::operator()(const void *in, uint64_t num_bytes)
{
	ingest(in, num_bytes);
	return *this;
}

cc0::fnv1a64 cc0::fnv1a64::operator()(const void *in, uint64_t num_bytes) const
{
	return fnv1a64(*this)(in, num_bytes);
}

cc0::fnv1a64::operator uint64_t( void ) const
{
	return h;
}

//
// key
//

cc0::key<const char*>::key(const char *v) : key(v, cc0::internal::str_count(v))
{}

cc0::key<const char*>::key(const char *v, uint64_t num_chars) : k(cc0::fnv1a64(v, num_chars))
{}
