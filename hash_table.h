/*
* Copyright 2014, carrotsrc.org
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

/* structure for describing the hash table */
typedef struct hashtable_stc hashtable_t;

struct hashtable_stc {
	int size;
	int total;
	htbucket_t *buckets;

	/* these are the callback functions for different
	 * data handling.
	 * hash and compare need to be defined
	 * the rest can be NULL
	 */
	unsigned int(*hashf)(void*); /* the hashing function for the data type */
	void*(*storef)(void*,void*); /* the function to call when storing data */
	int(*cmpf)(void*, void*, size_t); /* comparison between given value and hashed value */
	void*(*freef)(void*); /* freeing the data type */
};

unsigned int hash_string(void *str);

/* generate a new hash table */
hashtable_t *gen_hashtable(	int size, 
				unsigned int(*)(void*),
				void*(*)(void*, void*),
				int(*)(void*, void*, size_t),
				void*(*)(void*));

void free_hashtable(hashtable_t *table);

void *hashtable_add(void *value, size_t size, hashtable_t *table); /* add an item to the hash table */

void *hashtable_get_value(void *, size_t, hashtable_t *); /* get an value store */
void *hashtable_get_store(void *, size_t, hashtable_t *); /* get a value */

#endif
