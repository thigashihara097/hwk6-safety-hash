#include <limits.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "ts_hashmap.h"

/**
 * Creates a new thread-safe hashmap. 
 *
 * @param capacity initial capacity of the hashmap.
 * @return a pointer to a new thread-safe hashmap.
 */
ts_hashmap_t *initmap(int capacity) {
  ts_hashmap_t *map = malloc(sizeof(ts_hashmap_t));
  if(map == NULL) {
    return NULL; // return NULL if memory alloc fails
  }
  map->table = malloc(capacity * sizeof(ts_entry_t*));
  if(map->table == NULL){
    free(map);
    return NULL; // return NULL if memory alloc fails
  }

  // Initialize everything else
  map->capacity = capacity;
  map->size = 0;
  map->numOps = 0;
  pthread_mutex_init(&map->lock, NULL);

  return map;
}

/**
 * Obtains the value associated with the given key.
 * @param map a pointer to the map
 * @param key a key to search
 * @return the value associated with the given key, or INT_MAX if key not found
 */
int get(ts_hashmap_t *map, int key) {
  pthread_mutex_lock(&map->lock); // lock before accessing
  
  int index = (unsigned int) key % map->capacity;
  ts_entry_t *entry = map->table[index];

  while(entry != NULL){
    if(entry->key == key){
      pthread_mutex_unlock(&map->lock); // unlock before returning
    }
    entry = entry->next;
  }
  pthread_mutex_unlock(&map->lock); // unlock if key isn't found
  return INT_MAX;
}

/**
 * Associates a value associated with a given key.
 * @param map a pointer to the map
 * @param key a key
 * @param value a value
 * @return old associated value, or INT_MAX if the key was new
 */
int put(ts_hashmap_t *map, int key, int value) {
  pthread_mutex_lock(&map->lock); // lock before changing anything

  int index = (unsigned int) key % map->capacity;
  ts_entry_t *entry = map->table[index];
  ts_entry_t *prev = NULL;

  while(entry != NULL) { // key found
    if(entry->key == key) {
      int old_value = entry->value;
      entry->value = value;

      pthread_mutex_unlock(&map->lock); // unlock before exiting
      return old_value;                 // returns old value that was replaced
    }
    prev = entry;
    entry = entry->next;
  }
  // key not found
  ts_entry_t *new_entry = malloc(sizeof(ts_entry_t));
  if(new_entry == NULL) {
    pthread_mutex_unlock(&map->lock); // unlock before exiting
    return INT_MAX; // return INT_MAX if memory allocation fails
  }

  new_entry->key = key;
  new_entry->value = value;
  new_entry->next = NULL;

  // insert new entry
  if(prev == NULL) {
    map->table[index] = new_entry; // first entry if nothing's there
  } else {
    prev->next = new_entry;        // else it goes at the end
  }

  map->size++;
  map->numOps++;

  pthread_mutex_unlock(&map->lock); // Unlock when finished
  return INT_MAX; // return INT_MAX if key is new
}

/**
 * Removes an entry in the map
 * @param map a pointer to the map
 * @param key a key to search
 * @return the value associated with the given key, or INT_MAX if key not found
 */
int del(ts_hashmap_t *map, int key) {
  pthread_mutex_lock(&map->lock); // lock before changing anything

  int index = (unsigned int) key % map->capacity;
  ts_entry_t *entry = map->table[index];
  ts_entry_t *prev = NULL;

  while (entry != NULL) {
    if (entry->key == key) {
      int old_value = entry->value;
      if (prev == NULL) {
        map->table[index] = entry->next; // remove first in list
      } else {
        prev->next = entry->next;
      }
      free(entry); // free memory for the deleted entry
      map->size--;
      map->numOps--;

      pthread_mutex_unlock(&map->lock); // Unlock before exiting
      return old_value; // return value that was deleted
    }
    prev = entry;
    entry = entry->next;
  }

  pthread_mutex_unlock(&map->lock); // unlock after finishing
  return INT_MAX; // return INT_MAX if key wasn't found
}


/**
 * Prints the contents of the map (given)
 */
void printmap(ts_hashmap_t *map) {
  for (int i = 0; i < map->capacity; i++) {
    printf("[%d] -> ", i);
    ts_entry_t *entry = map->table[i];
    while (entry != NULL) {
      printf("(%d,%d)", entry->key, entry->value);
      if (entry->next != NULL)
        printf(" -> ");
      entry = entry->next;
    }
    printf("\n");
  }
}

/**
 * Free up the space allocated for hashmap
 * @param map a pointer to the map
 */
void freeMap(ts_hashmap_t *map) {
  for(int i = 0; i< map->capacity; i++){
    ts_entry_t *entry = map->table[i];
    while(entry != NULL){
      ts_entry_t *temp = entry;
      entry = entry->next;
      free(temp);
    }
  }
    free(map->table);
    pthread_mutex_destroy(&map->lock); // Destroy the lock
    free(map);
}