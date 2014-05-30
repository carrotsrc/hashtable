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

#include <stdlib.h>
#include <string.h>
#include "hash_table.h"

typedef struct ht_list_stc htitem_t;
struct ht_list_stc {
	char *value;		// the value that is hashed
	void *store;		// pointer to data that is indexed
	htitem_t *p;	// the previous list item
	htitem_t *n;	// the next list item
};

typedef struct ht_bucket_stc htbucket_t;
struct ht_bucket_stc {
	short occupied;
	htitem_t *content;
};

htitem_t *hashitem_get(htitem_t *, void *, size_t, int(*)(void*, void*, size_t));
htitem_t *hashitem_append(htitem_t *list, void *value, size_t size);
void *ht_add_val(unsigned int, void *, size_t, hashtable_t *, void*(*)(void*));

void *ht_get_val(unsigned int, void *, size_t, hashtable_t *);
void *ht_get_store(unsigned int, void *, size_t, hashtable_t *);
void *ht_get_item(unsigned int, void *, size_t, hashtable_t *);

/* simple hash generation function for string
 */
unsigned int hash_string(void *str)
{
	unsigned int key = 0;
	const char *s = (const char *)str;
	while(*s)
		key = key*31+*s++;

	return key;
}

/* simple native string comparison function */
int compare_string(void *value, void *str, size_t size)
{
	const char *s = (const char *)str;
	const char *v = (const char *)value;
	while(*s)
		if(*s++ != *v++)
			return 1;

	return 0;
}

/* retrieve an item from a bucket's list
 */
htitem_t *hashitem_get(htitem_t *list, void *value, size_t size, int(cmpf)(void*, void*, size_t))
{
	while(list->n != NULL) {
		if(cmpf(list->value, value, size) == 0)
			return list;

		list = list->n;
	}

	if(cmpf(list->value, value, size) == 0)
		return list;

	return NULL;
}

/* append an item to the end of a bucket's list
 */
htitem_t *hashitem_append(htitem_t *list, void *value, size_t size)
{
	while(list->n != NULL)
		list = list->n;

	htitem_t *item = malloc(sizeof(htitem_t));
	item->value = malloc(size);
	memcpy(item->value, value, size);
	item->store = NULL;
	item->n = NULL;
	item->p = list;
	list->n = item;
	return item;
}

/* generate a blank hash table of size specified. include the type functions
 */
hashtable_t *gen_hashtable(int size, unsigned int(*hashf)(void*), void*(*storef)(void*, void*), int(*cmpf)(void*, void*, size_t),void*(*freef)(void*))
{
	hashtable_t *ht = malloc(sizeof(hashtable_t));
	ht->size = size;
	ht->total = 0;
	ht->buckets = calloc(size, sizeof(htbucket_t));
	if(hashf == NULL)
		hashf = &hash_string;
	ht->hashf = hashf;
	ht->storef = storef;
	if(cmpf == NULL)
		cmpf = &compare_string;

	ht->cmpf = cmpf;
	ht->freef = freef;
	return ht;
}

/* properly free the memory allocated for the hash table
 * including all the bucket lists and their values
 */
void free_hashtable(hashtable_t *table)
{
	int sz = table->size;
	for(int i = 0; i < sz; i++) {
		if(table->buckets[i].occupied == 0x0)
			continue;

		htitem_t *item = table->buckets[i].content;
		while(item->n != NULL) {
			free(item->value);
			if(table->freef != NULL)
				table->freef(item->store);
			item = item->n;
			free(item->p);
		}

		free(item->value);
		free(item);
	}
	table->size = 0;
	table->total = 0;
	free(table->buckets);
	table->buckets = NULL;
	free(table);
}

/* Add a new value to the hashtable.
 * 
 * When an item is added to the hashtable, a callback function is run
 * to organise the data that the indexed item will be storing, which is
 * handed over to the *store field in the item's data structure
 */
void *hashtable_add(void *value, size_t size, hashtable_t *table)
{
	unsigned int hash = table->hashf(value);
	unsigned int index = hash%(table->size);
	htitem_t *item = NULL;

	if(table->buckets[index].occupied == 0x0) {
		/* the index is a clear bucket */
		item = malloc(sizeof(htitem_t));
		item->value = malloc(size);
		memcpy(item->value, value, size);
		if(table->storef != NULL)
			item->store = table->storef(NULL, value); /* run the callback for new store type */

		item->n = NULL;
		item->p = NULL;
		table->buckets[index].occupied = 0x1;
		table->buckets[index].content = item;
		table->total++;
	} else {
		/* the index is a bucket with content */
		if((item = hashitem_get(table->buckets[index].content, value, size, table->cmpf)) == NULL) {
			item = hashitem_append(table->buckets[index].content, value, size);
			table->total++;
		}

		item->store = table->storef(item->store, value); /* run the callback for store type */
	}

	return item->store;
}

/* get a value's store from the hash table. If it exists then return
 * a pointer to the item otherwise return NULL
 */
void *hashtable_get_store(void *value, size_t size, hashtable_t *table)
{
	unsigned int hash = table->hashf(value);
	htitem_t *item = NULL;

	if((item = ht_get_item(hash, value, size, table)) == NULL)
		return NULL;

	return item->store;
}

/* get a value from the hash table. If it exists then return
 * a pointer to the value otherwise return NULL
 */
void *hashtable_get_value(void *value, size_t size, hashtable_t *table)
{
	unsigned int hash = table->hashf(value);
	htitem_t *item = NULL;

	if((item = ht_get_item(hash, value, size, table)) == NULL)
		return NULL;

	return item->value;
}

/* local scope function for retrieving an item */
void *ht_get_item(unsigned int hash, void *value, size_t size, hashtable_t *table)
{
	unsigned int index = hash%(table->size);
	htitem_t *item = NULL;
	if(table->buckets[index].occupied == 0x0)
		return NULL;
	
	if(( item = hashitem_get(table->buckets[index].content, value, size, table->cmpf)) == NULL)
		return NULL;
	
	return item;
}
