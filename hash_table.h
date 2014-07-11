/*
* Copyright 2014, Charlie Fyvie-Gauld (Carrotsrc.org)
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*   http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/
#ifndef HASH_TABLE_H_
#define HASH_TABLE_H_

typedef struct ht_iterator_stc htiterator_td; /* an interator type for the hash table */
/* structure for describing the hash table */
typedef struct ht_bucket_stc htbucket_td; /* this is required for structure */
typedef struct hashtable_stc hashtable_td;

struct hashtable_stc {
	int size;
	int total;
	htbucket_td *buckets;

	/* these are the callback functions for different
	 * data handling.
	 * hash and compare need to be defined
	 * the rest can be NULL
	 */
	unsigned int(*hashf)(void*); /* the hashing function for the data type */
	void*(*storef)(void*,void*); /* the function to call when storing data */
	int(*cmpf)(void*, void*, size_t); /* comparison between given value and hashed value */
	void*(*freef)(void*,void*); /* freeing the data type */
};

unsigned int hash_string(void *str);

/* generate a new hash table */
hashtable_td *gen_hashtable(	int size, 
				unsigned int(*)(void*),
				void*(*)(void*, void*),
				int(*)(void*, void*, size_t),
				void*(*)(void*,void*));

void free_hashtable(hashtable_td *table);

void *hashtable_add(void *value, size_t size, hashtable_td *table); /* add an item to the hash table */

void *hashtable_get_value(void *, size_t, hashtable_td *); /* get an value store */
void *hashtable_get_store(void *, size_t, hashtable_td *); /* get a value */

htiterator_td *hashtable_iter(hashtable_td*);

void *hashiter_next_store(htiterator_td*);
void *hashiter_current_store(htiterator_td*);

void *hashiter_next_value(htiterator_td*);
void *hashiter_current_value(htiterator_td*);

void hashiter_rewind(htiterator_td*);
void hashiter_free(htiterator_td*);

#endif
