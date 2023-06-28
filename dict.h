/// @file
/// @author github.com/SirJonthe
/// @date 2023
/// @copyright Public domain.
/// @license CC0 1.0

#ifndef CC0_DICT_H_INCLUDED__
#define CC0_DICT_H_INCLUDED__

#include <cstdint>

namespace cc0
{
	/// @brief Definitions used within the library itself.
	/// @note For internal use only. Everything inside this namespace could be subject to change without notice.
	namespace internal
	{
		/// @brief A class to wrap comparing two values in a default way.
		/// @tparam type_t The type to be compared.
		template < typename type_t >
		class bytewise_cmp
		{
		public:
			/// @brief Compares the bytes in two values.
			/// @param a A value.
			/// @param b Another value.
			/// @return TRUE if all bytes in the values are equal.
			bool operator()(const type_t &a, const type_t &b) const;
		};
	}

	/// @brief A dictionary/hash table/map type where an arbitary key type can be used as an index to find a particular value stored in the data structure.
	/// @tparam key_t The type of the key used to access values. Default behavior is to compare keys using a bytewise comparison.
	/// @tparam value_t The type of the value to be stored in the table.
	/// @tparam cmp_fn A functor used to determine equality between keys. Defaults to a bytewise comparison of the key structure (does not compare data that is pointed to).
	/// @note This means that keys containing pointers to data most likely will fail equality tests even though the data being pointed to is the same between two keys if they merely are copies. A common issue would be to use std::string as a key (use const char* as a key since constant strings are stored globally in the binary in C and C++). For the general purpose use a custom digest class as a key instead, or provide your own custom comparison function.
	/// @note Due to how this table is implemented, look up is O(n) in time complexity, where n is the number of bytes in the key type. However, for many cases, using a good key will result in a hit in just a few iterations. 
	template < typename key_t, typename value_t, typename cmp_fn = cc0::internal::bytewise_cmp<key_t> >
	class dict
	{
	private:
		/// @brief A basic array type for internal use.
		/// @tparam type_t The type of the array.
		template < typename type_t >
		class array
		{
		private:
			type_t   *m_vals;
			uint64_t  m_size;
			uint64_t  m_pool;
			uint64_t  m_growth;

		public:
			explicit array(uint64_t growth = 1);
			array(const array &a);
			~array( void );
			array &operator=(const array &a);
			void          destroy( void );
			void          create(uint64_t size);
			void          reserve(uint64_t size);
			void          resize(uint64_t size);
			void          resize_pool(uint64_t size);
			type_t       &add( void );
			uint64_t      size( void ) const;
			uint64_t      pool_size( void ) const;
			type_t       &operator[](uint64_t i);
			const type_t &operator[](uint64_t i) const;
			type_t       &first( void );
			const type_t &first( void ) const;
			type_t       &last( void );
			const type_t &last( void ) const;
		};

		/// @brief An index into an array.
		struct index
		{
			enum {
				NIL,  // Element has never been allocated before.
				FREE, // Element has previously been allocated, but is no longer in use.
				VAL,  // Element points to a value in the value array.
				TAB   // Element points to a table in the table array.
			} type;
			uint64_t index;
		};

		/// @brief A hash table entry (key-value pair).
		struct entry
		{
			key_t    k;    // The full key.
			value_t  v;    // The value.
			uint64_t refs; // The number of references to this entry from tables.
		};

		static const uint64_t NUM_ENTRIES_IN_TABLE = uint64_t(uint8_t(-1)) + 1;

		/// @brief A table of indices.
		struct table
		{
			index    idx[NUM_ENTRIES_IN_TABLE]; // Indices to either the value array or table array.
			uint64_t refs;                      // The number of in-use values in this table.
			uint64_t i;
		};

		static table &init_table(table &t);

	private:
		array<entry> m_vals;
		array<table> m_tabs;
		cmp_fn       m_cmp;
		uint64_t     m_size;

	private:
		template < typename type_t >
		static const uint8_t *bytes(const type_t &t);
		const value_t        *lookup(const table &t, const key_t &k, uint64_t level) const;
		value_t              *lookup(const table &t, const key_t &k, uint64_t level);
		value_t              &lookup_or_alloc(uint64_t t, const key_t &k, uint64_t level);
		value_t              &alloc(uint64_t t, const key_t &k, uint64_t level);
		void                  remove(table &t, const key_t &k, uint64_t level);
		uint64_t              prof_lookup(const table &t, const key_t &k, uint64_t level) const;

	public:
		/// @brief Initializes the data structure.
		dict( void );

		/// @brief Copies a dictionary.
		/// @param d The dictionary to copy.
		dict(const dict &d);

		/// @brief Moves data from one dictionary to another.
		/// @param d The dictionary to move data from.
		//dict(dict &&d);

		/// @brief Copies a dictionary.
		/// @param d The dictionary to copy.
		/// @return A reference to self.
		dict &operator=(const dict &d);

		/// @brief Moves data from one dictionary to another.
		/// @param d The dictionary to move data from.
		/// @return A reference to self.
		//dict &operator=(dict &&d);

		/// @brief Returns the pointer to the value pointed to by the key. Null is returned if the key does not exist.
		/// @param key The key.
		/// @return The value pointed to by the key.
		const value_t *operator[](const key_t &key) const;
		
		/// @brief Returns the pointer to the value pointed to by the key. Null is returned if the key does not exist.
		/// @param key The key.
		/// @return The value pointed to by the key.
		value_t *operator[](const key_t &key);

		/// @brief Returns a reference to the value pointed to by the key. If the value does not exist, there will be a null pointer dereference, and possibly a crash.
		/// @param key The key.
		/// @return The reference to the value pointed to by the key.
		/// @warning May crash the application if the value does not exist. Only use this function if you are sure the value exists.
		const value_t &operator()(const key_t &key) const;

		/// @brief Returns a reference to the value pointed to by the key. If the value does not exist, a new value will be created making this function always safe to call (the value returned may, however, not be initialized).
		/// @param key The key.
		/// @return The reference to the value pointed to by the key.
		value_t &operator()(const key_t &key);

		/// @brief Returns a reference to the value pointed to by the key. If the value does not exist, a new value will be created making this function always safe to call (the value returned may, however, not be initialized).
		/// @param key The key.
		/// @return The reference to the value pointed to by the key.
		value_t &insert(const key_t &key);

		/// @brief Removes a value with the specified key. If the value does not exist nothing will happen.
		/// @param key The key.
		void remove(const key_t &key);

		/// @brief Returns the total space, in bytes, allocated by the data structure.
		/// @return  The total space, in bytes, allocated by the data structure.
		uint64_t allocated_bytes( void ) const;

		/// @brief Returns the total space, in bytes, used by the data structure.
		/// @return The total space, in bytes, used by the data structure.
		/// @note This calculation does not completely give an accurate picture as tables containing at least one value is still marked as in use, and not partially in use.
		uint64_t used_bytes( void ) const;

		/// @brief Returns the number of values stored in the data structure.
		/// @return The number of values stored in the data structure.
		uint64_t size( void ) const;

		/// @brief Counts the number of look-ups made to find the requested value at the key.
		/// @param key The key.
		/// @return The number of look-ups made to find the value at the key.
		uint64_t prof_lookup(const key_t &key) const;

		/// @brief Returns the number of tables currently allocated for the dictionary.
		/// @return The number of tables currently allocated for the dictionary.
		uint64_t table_count( void ) const;
	};
}

template < typename type_t >
bool cc0::internal::bytewise_cmp<type_t>::operator()(const type_t &a, const type_t &b) const
{
	const uint8_t *A = reinterpret_cast<const uint8_t*>(&a);
	const uint8_t *B = reinterpret_cast<const uint8_t*>(&b);
	for (uint64_t i = 0; i < sizeof(type_t); ++i) {
		if (A[i] != B[i]) { return false; }
	}
	return true;
}

template < typename key_t, typename value_t, typename cmp_fn >
template < typename type_t >
cc0::dict<key_t, value_t, cmp_fn>::array<type_t>::array(uint64_t growth) : m_vals(nullptr), m_size(0), m_pool(0), m_growth(growth > 0 ? growth : 1)
{}

template < typename key_t, typename value_t, typename cmp_fn >
template < typename type_t >
cc0::dict<key_t, value_t, cmp_fn>::array<type_t>::array(const cc0::dict<key_t, value_t, cmp_fn>::array<type_t> &a) : array()
{
	resize_pool(a.m_pool);
	resize(a.m_size);
	for (uint64_t i = 0; i < m_size; ++i) {
		m_vals[i] = a.m_vals[i];
	}
}

template < typename key_t, typename value_t, typename cmp_fn >
template < typename type_t >
cc0::dict<key_t, value_t, cmp_fn>::array<type_t>::~array( void )
{
	delete [] m_vals;
}

template < typename key_t, typename value_t, typename cmp_fn >
template < typename type_t >
cc0::dict<key_t, value_t, cmp_fn>::array<type_t> &cc0::dict<key_t, value_t, cmp_fn>::array<type_t>::operator=(const cc0::dict<key_t, value_t, cmp_fn>::array<type_t> &a)
{
	if (&a != this) {
		resize_pool(a.m_pool);
		resize(a.m_size);
		for (uint64_t i = 0; i < m_size; ++i) {
			m_vals[i] = a.m_vals[i];
		}
	}
	return *this;
}

template < typename key_t, typename value_t, typename cmp_fn >
template < typename type_t >
void cc0::dict<key_t, value_t, cmp_fn>::array<type_t>::destroy( void )
{
	delete [] m_vals;
	m_size = 0;
	m_pool = 0;
}

template < typename key_t, typename value_t, typename cmp_fn >
template < typename type_t >
void cc0::dict<key_t, value_t, cmp_fn>::array<type_t>::create(uint64_t size)
{
	reserve(size);
	m_size = size;
}

template < typename key_t, typename value_t, typename cmp_fn >
template < typename type_t >
void cc0::dict<key_t, value_t, cmp_fn>::array<type_t>::reserve(uint64_t size)
{
	if (size > m_pool) {
		destroy();
		m_vals = new type_t[size];
		m_pool = size;
	}
	m_size = 0;
}

template < typename key_t, typename value_t, typename cmp_fn >
template < typename type_t >
void cc0::dict<key_t, value_t, cmp_fn>::array<type_t>::resize(uint64_t size)
{
	if (size > m_pool) {
		type_t *vals = new type_t[size];
		const uint64_t min = m_size < size ? m_size : size;
		for (uint64_t i = 0; i < min; ++i) {
			vals[i] = m_vals[i];
		}
		delete [] m_vals;
		m_vals = vals;
		m_pool = size;
	}
	m_size = size;
}

template < typename key_t, typename value_t, typename cmp_fn >
template < typename type_t >
void cc0::dict<key_t, value_t, cmp_fn>::array<type_t>::resize_pool(uint64_t size)
{
	const uint64_t min = m_size < size ? m_size : size;
	if (size > m_pool) {
		type_t *vals = new type_t[size];
		for (uint64_t i = 0; i < min; ++i) {
			vals[i] = m_vals[i];
		}
		delete [] m_vals;
		m_vals = vals;
		m_pool = size;
	}
	m_size = min;
}

template < typename key_t, typename value_t, typename cmp_fn >
template < typename type_t >
type_t &cc0::dict<key_t, value_t, cmp_fn>::array<type_t>::add( void )
{
	if (m_size >= m_pool) {
		resize_pool(m_size + m_growth);
	}
	return m_vals[m_size++];
}

template < typename key_t, typename value_t, typename cmp_fn >
template < typename type_t >
uint64_t cc0::dict<key_t, value_t, cmp_fn>::array<type_t>::size( void ) const
{
	return m_size;
}

template < typename key_t, typename value_t, typename cmp_fn >
template < typename type_t >
uint64_t cc0::dict<key_t, value_t, cmp_fn>::array<type_t>::pool_size( void ) const
{
	return m_pool;
}

template < typename key_t, typename value_t, typename cmp_fn >
template < typename type_t >
type_t &cc0::dict<key_t, value_t, cmp_fn>::array<type_t>::operator[](uint64_t i)
{
	return m_vals[i];
}

template < typename key_t, typename value_t, typename cmp_fn >
template < typename type_t >
const type_t &cc0::dict<key_t, value_t, cmp_fn>::array<type_t>::operator[](uint64_t i) const
{
	return m_vals[i];
}

template < typename key_t, typename value_t, typename cmp_fn >
template < typename type_t >
type_t &cc0::dict<key_t, value_t, cmp_fn>::array<type_t>::first( void )
{
	return m_vals[0];
}

template < typename key_t, typename value_t, typename cmp_fn >
template < typename type_t >
const type_t &cc0::dict<key_t, value_t, cmp_fn>::array<type_t>::first( void ) const
{
	return m_vals[0];
}

template < typename key_t, typename value_t, typename cmp_fn >
template < typename type_t >
type_t &cc0::dict<key_t, value_t, cmp_fn>::array<type_t>::last( void )
{
	return m_vals[m_size - 1];
}

template < typename key_t, typename value_t, typename cmp_fn >
template < typename type_t >
const type_t &cc0::dict<key_t, value_t, cmp_fn>::array<type_t>::last( void ) const
{
	return m_vals[m_size - 1];
}

template < typename key_t, typename value_t, typename cmp_fn >
typename cc0::dict<key_t, value_t, cmp_fn>::table &cc0::dict<key_t, value_t, cmp_fn>::init_table(cc0::dict<key_t, value_t, cmp_fn>::table &t)
{
	for (uint64_t i = 0; i < NUM_ENTRIES_IN_TABLE; ++i) {
		t.idx[i] = { index::NIL, 0 };
	}
	t.refs = 0;
	t.i = 0;
	return t;
}

template < typename key_t, typename value_t, typename cmp_fn >
template < typename type_t >
const uint8_t *cc0::dict<key_t, value_t, cmp_fn>::bytes(const type_t &t)
{
	return reinterpret_cast<const uint8_t*>(&t);
}

template < typename key_t, typename value_t, typename cmp_fn >
const value_t *cc0::dict<key_t, value_t, cmp_fn>::lookup(const cc0::dict<key_t, value_t, cmp_fn>::table &t, const key_t &k, uint64_t level) const
{
	const index i = t.idx[bytes(k)[level]];
	switch (i.type) {
		case index::TAB: return lookup(m_tabs[i.index], k, level + 1);
		case index::VAL: return m_cmp(k, m_vals[i.index].k) ? &m_vals[i.index].v : nullptr;
	}
	return nullptr;
}

template < typename key_t, typename value_t, typename cmp_fn >
value_t *cc0::dict<key_t, value_t, cmp_fn>::lookup(const cc0::dict<key_t, value_t, cmp_fn>::table &t, const key_t &k, uint64_t level)
{
	const index i = t.idx[bytes(k)[level]];
	switch (i.type) {
		case index::TAB: return lookup(m_tabs[i.index], k, level + 1);
		case index::VAL: return m_cmp(k, m_vals[i.index].k) ? &m_vals[i.index].v : nullptr;
	}
	return nullptr;
}

template < typename key_t, typename value_t, typename cmp_fn >
value_t &cc0::dict<key_t, value_t, cmp_fn>::lookup_or_alloc(uint64_t t, const key_t &k, uint64_t level)
{
	const index i = m_tabs[t].idx[bytes(k)[level]];
	switch (i.type) {
	case index::TAB: return lookup_or_alloc(i.index, k, level + 1);
	case index::VAL: return m_cmp(k, m_vals[i.index].k) ? m_vals[i.index].v : alloc(t, k, level);
	}
	return alloc(t, k, level);
}

template < typename key_t, typename value_t, typename cmp_fn >
value_t &cc0::dict<key_t, value_t, cmp_fn>::alloc(uint64_t t, const key_t &k, uint64_t level)
{
	// NOTE: We can be quite wasteful with resources here in the worst case. If the keys only differ in the last byte, we allocate a ton of tables that are never in proper use.
	index i = m_tabs[t].idx[bytes(k)[level]];
	if (i.type == index::VAL) { // Collision!
		init_table(m_tabs.add()).idx[bytes(m_vals[i.index].k)[level + 1]] = i;
		m_tabs.last().i = m_tabs.size() - 1;
		i.type = index::TAB;
		i.index = m_tabs.size() - 1;
		m_tabs[t].idx[bytes(k)[level]] = i;
		m_tabs.last().refs = 1;
		return alloc(i.index, k, level + 1);
	}
	if (i.type == index::NIL) { // NOTE: If the index is FREE we can re-use the index since we know it is unused in the value array. (We already know type is not TAB since alloc() is only called for values or empty entries).
		i.index = m_vals.size();
		m_vals.add();
		m_vals.last().k = k;
	}
	i.type = index::VAL;
	m_vals[i.index].refs = 1;
	++m_tabs[t].refs;
	++m_size;
	m_tabs[t].idx[bytes(k)[level]] = i;
	return m_vals[i.index].v;
}

template < typename key_t, typename value_t, typename cmp_fn >
void cc0::dict<key_t, value_t, cmp_fn>::remove(cc0::dict<key_t, value_t, cmp_fn>::table &t, const key_t &k, uint64_t level)
{
	index &i = t.idx[bytes(k)[level]];
	switch (i.type) {
	case index::VAL:
		if (m_cmp(k, m_vals[i.index].k)) {
			m_vals[i.index].refs = 0;
			i.type = index::FREE; // NOTE: If we do not delete the index, we can reuse it if another entry hits this index. FREE denotes just that.
			--t.refs; // TODO: We could collapse the table if refs is 1. That will require some additional work however as we either need to deallocate the table or be able to reuse it later without needing to scan the entire table array for an empty table.
			--m_size;
		}
		break;
	case index::TAB:
		remove(m_tabs[i.index], k, level + 1);
	}
}

template < typename key_t, typename value_t, typename cmp_fn >
uint64_t cc0::dict<key_t, value_t, cmp_fn>::prof_lookup(const table &t, const key_t &k, uint64_t level) const
{
	const index i = t.idx[bytes(k)[level]];
	switch (i.type) {
	case index::TAB: return prof_lookup(m_tabs[i.index], k, level + 1);
	}
	return level + 1;
}

template < typename key_t, typename value_t, typename cmp_fn >
cc0::dict<key_t, value_t, cmp_fn>::dict( void ) : m_vals(NUM_ENTRIES_IN_TABLE), m_tabs(16), m_size(0)
{
	init_table(m_tabs.add());
}

template < typename key_t, typename value_t, typename cmp_fn >
cc0::dict<key_t, value_t, cmp_fn>::dict(const dict<key_t, value_t, cmp_fn> &d) : m_vals(d.m_vals), m_tabs(d.m_tabs), m_size(d.m_size)
{}

template < typename key_t, typename value_t, typename cmp_fn >
cc0::dict<key_t, value_t, cmp_fn> &cc0::dict<key_t, value_t, cmp_fn>::operator=(const dict<key_t, value_t, cmp_fn> &d)
{
	if (&d != this) {
		m_vals = d.m_vals;
		m_tabs = d.m_tabs;
		m_size = d.m_size;
	}
	return *this;
}

template < typename key_t, typename value_t, typename cmp_fn >
const value_t *cc0::dict<key_t, value_t, cmp_fn>::operator[](const key_t &key) const
{
	return lookup(m_tabs.first(), key, 0);
}

template < typename key_t, typename value_t, typename cmp_fn >
value_t *cc0::dict<key_t, value_t, cmp_fn>::operator[](const key_t &key)
{
	return lookup(m_tabs.first(), key, 0);
}

template < typename key_t, typename value_t, typename cmp_fn >
const value_t &cc0::dict<key_t, value_t, cmp_fn>::operator()(const key_t &key) const
{
	return *lookup(key);
}

template < typename key_t, typename value_t, typename cmp_fn >
value_t &cc0::dict<key_t, value_t, cmp_fn>::operator()(const key_t &key)
{
	return insert(key);
}

template < typename key_t, typename value_t, typename cmp_fn >
value_t &cc0::dict<key_t, value_t, cmp_fn>::insert(const key_t &key)
{
	return lookup_or_alloc(0, key, 0);
}

template < typename key_t, typename value_t, typename cmp_fn >
void cc0::dict<key_t, value_t, cmp_fn>::remove(const key_t &key)
{
	remove(m_tabs.first(), key, 0);
}

template < typename key_t, typename value_t, typename cmp_fn >
uint64_t cc0::dict<key_t, value_t, cmp_fn>::allocated_bytes( void ) const
{
	return m_vals.pool_size() * sizeof(entry) + m_tabs.pool_size() * sizeof(table);
}

template < typename key_t, typename value_t, typename cmp_fn >
uint64_t cc0::dict<key_t, value_t, cmp_fn>::used_bytes( void ) const
{
	uint64_t v = size();
	uint64_t t = 0;
	for (uint64_t i = 0; i < m_tabs.size(); ++i) {
		if (m_tabs[i].refs) {
			++t;
		}
	}
	return v * sizeof(entry) + t * sizeof(table);
}

template < typename key_t, typename value_t, typename cmp_fn >
uint64_t cc0::dict<key_t, value_t, cmp_fn>::size( void ) const
{
	return m_size;
}

template < typename key_t, typename value_t, typename cmp_fn >
uint64_t cc0::dict<key_t, value_t, cmp_fn>::prof_lookup(const key_t &key) const
{
	return prof_lookup(m_tabs[0], key, 0);
}

template < typename key_t, typename value_t, typename cmp_fn >
uint64_t cc0::dict<key_t, value_t, cmp_fn>::table_count( void ) const
{
	return m_tabs.size();
}

#endif
