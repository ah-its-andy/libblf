#ifndef BLF_H
#define BLF_H

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

#define BLF_MAGIC 0x42B1F000  // 'BLF\0'
#define BLF_VERSION 1

// File header structure
typedef struct {
    uint32_t magic;      // Magic number for file identification
    uint32_t version;    // File format version
    uint64_t kv_offset;  // KV section offset
    uint64_t kv_size;    // KV section size
    uint64_t raw_offset; // Raw data section offset
    uint64_t raw_size;   // Raw data section size
} blf_header_t;

// KV entry header
typedef struct {
    uint32_t key_length;    // Length of key
    uint32_t value_length;  // Length of value
} blf_kv_entry_t;

// BLF file handle
typedef struct {
    FILE *fp;
    blf_header_t header;
    char *filename;
} blf_file_t;

// File operations
blf_file_t* blf_create(const char *filename);
blf_file_t* blf_open(const char *filename);
void blf_close(blf_file_t *file);

// KV operations
bool blf_put_kv(blf_file_t *file, const char *key, const void *value, uint32_t value_length);
bool blf_get_kv(blf_file_t *file, const char *key, void *value, uint32_t *value_length);
bool blf_delete_kv(blf_file_t *file, const char *key);

// Raw data operations
bool blf_write_raw(blf_file_t *file, const void *data, uint64_t size);
bool blf_read_raw(blf_file_t *file, void *data, uint64_t *size);

// Utility functions
bool blf_flush(blf_file_t *file);
bool blf_update_header(blf_file_t *file);

#endif // BLF_H
