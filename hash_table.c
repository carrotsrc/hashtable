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

#include <stdlib.h>
#include <string.h>
#include "hash_table.h"

typedef struct ht_list_stc htitem_td;
struct ht_list_stc {
	void *value;		// the item that is hashed
	void *store;		// pointer to data that is indexed
	htitem_td *p;	// the previous list item
	htitem_td *n;	// the next list item
};


struct ht_bucket_stc {
	short occupied;
	htitem_td *content;
};

struct ht_iterator_stc {
	hashtable_td *table;
	unsigned int b;
	htitem_td *i;
	htitem_td *p;
};

static htitem_td *hashitem_get(htitem_td *, void *, size_t, int(*)(void*, void*, size_t));
static htitem_td *hashitem_append(htitem_td *list, void *value, size_t size);
static void *ht_add_val(unsigned int, void *, size_t, hashtable_td *, void*(*)(void*));

static void *ht_get_val(unsigned int, void *, size_t, hashtable_td *);
static void *ht_get_store(unsigned int, void *, size_t, hashtable_td *);
static void *ht_get_item(unsigned int, void *, size_t, hashtable_td *);

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
htitem_td *hashitem_get(htitem_td *list, void *value, size_t size, int(cmpf)(void*, void*, size_t))
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
htitem_td *hashitem_append(htitem_td *list, void *value, size_t size)
{
	while(list->n != NULL)
		list = list->n;

	htitem_td *item = malloc(sizeof(htitem_td));
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
hashtable_td *gen_hashtable(int size, unsigned int(*hashf)(void*), void*(*storef)(void*, void*), int(*cmpf)(void*, void*, size_t),void*(*freef)(void*,void*))
{
	hashtable_td *ht = malloc(sizeof(hashtable_td));
	ht->size = size;
	ht->total = 0;
	ht->buckets = calloc(size, sizeof(htbucket_td));
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
void free_hashtable(hashtable_td *table)
{
	int sz = table->size;

	for(int i = 0; i < sz; i++) {
		if(table->buckets[i].occupied == 0x0)
			continue;

		htitem_td *item = table->buckets[i].content;
		while(item->n != NULL) {
			if(table->freef != NULL)
				table->freef(item->value, item->store);
			else
				free(item->value);

			item = item->n;
			free(item->p);
		}

		if(table->freef != NULL)
			table->freef(item->value, item->store);
		else
			free(item->value);

		free(item);

	}
	table->size = 0;
	table->total = 0;
	table->hashf = NULL;
	table->storef = NULL;
	table->cmpf = NULL;
	table->freef = NULL;
	
	free(table->buckets);
	table->buckets = NULL;
	free(table);
	table = NULL;

}

/* Add a new value to the hashtable.
 * 
 * When an item is added to the hashtable, a callback function is run
 * to organise the data that the indexed item will be storing, which is
 * handed over to the *store field in the item's data structure
 */
void *hashtable_add(void *value, size_t size, hashtable_td *table)
{
	unsigned int hash = table->hashf(value);
	unsigned int index = hash%(table->size);
	htitem_td *item = NULL;

	if(table->buckets[index].occupied == 0x0) {
		/* the index is a clear bucket */
		item = malloc(sizeof(htitem_td));
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
void *hashtable_get_store(void *value, size_t size, hashtable_td *table)
{
	unsigned int hash = table->hashf(value);
	htitem_td *item = NULL;

	if((item = ht_get_item(hash, value, size, table)) == NULL)
		return NULL;

	return item->store;
}

/* get a value from the hash table. If it exists then return
 * a pointer to the value otherwise return NULL
 */
void *hashtable_get_value(void *value, size_t size, hashtable_td *table)
{
	unsigned int hash = table->hashf(value);
	htitem_td *item = NULL;

	if((item = ht_get_item(hash, value, size, table)) == NULL)
		return NULL;

	return item->value;
}

/* local scope function for retrieving an item */
void *ht_get_item(unsigned int hash, void *value, size_t size, hashtable_td *table)
{
	unsigned int index = hash%(table->size);
	htitem_td *item = NULL;
	if(table->buckets[index].occupied == 0x0)
		return NULL;
	
	if(( item = hashitem_get(table->buckets[index].content, value, size, table->cmpf)) == NULL)
		return NULL;
	
	return item;
}

/* create a new iterator */
htiterator_td *hashtable_iter(hashtable_td *table)
{
	htiterator_td *iter = malloc(sizeof(htiterator_td));
	iter->table = table;
	iter->b = 0;
	iter->i = NULL;
	iter->p = NULL;

	/* roll it forward */
	while(iter->table->buckets[iter->b].occupied == 0x0)
		iter->b++;
	
	/* roll it back one so the next() functions work
	 * with minimal checks
	 */
	iter->b--;
}

/* get next store from iterator
 *
 * this will set the current iterator item automatically
 * to the next one which is why ->p is used for reference
 */
void *hashiter_next_store(htiterator_td *iter)
{
	void *store = NULL;
	if(iter->i == NULL) {
		iter->b++;
		while(iter->table->buckets[iter->b].occupied == 0x0) {
			iter->b++;
			if(iter->b == iter->table->size)
				return NULL;
		}

		iter->i = iter->table->buckets[iter->b].content;
	}

	store = iter->i->store;
	iter->p = iter->i;	/* so we can just quickly roll back */
	iter->i = iter->i->n;

	return store;
}

void *hashiter_current_store(htiterator_td *iter)
{
	if(iter->p == NULL)
		return NULL;

	return iter->p->store;
}

/* get next value from iterator 
 *
 * Same as store function
 */
void *hashiter_next_value(htiterator_td *iter)
{
	void *value = NULL;
	if(iter->i == NULL) {
		iter->b++;
		while(iter->table->buckets[iter->b].occupied == 0x0) {
			iter->b++;
			if(iter->b == iter->table->size)
				return NULL;
		}

		iter->i = iter->table->buckets[iter->b].content;
	}

	value = iter->i->value;
	iter->p = iter->i;	/* so we can just quickly roll back */
	iter->i = iter->i->n;

	return value;
}

/* get the current value from the iterator */
void *hashiter_current_value(htiterator_td *iter)
{
	if(iter->p == NULL)
		return NULL;

	/* the next() function has already rolled onto the next
	 * item, so use ->p instead which is reference to previous
	 */
	return iter->p->value;
}

void hashiter_rewind(htiterator_td *iter)
{
	iter->b = 0;
	iter->p = NULL;
	iter->i = NULL;

	/* roll it forward */
	while(iter->table->buckets[iter->b].occupied == 0x0)
		iter->b++;
	
	/* roll it back one so the next() functions work
	 * with minimal checks
	 */
	iter->b--;
}

void hashiter_free(htiterator_td *iter)
{
	iter->b = 0;
	iter->p = NULL;
	iter->i = NULL;
	free(iter);
	iter = NULL;
}
