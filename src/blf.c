// Define _POSIX_C_SOURCE to make strdup available
#define _POSIX_C_SOURCE 200809L

#include "blf.h"
#include <stdlib.h>
#include <string.h>

// Create a new BLF file
blf_file_t* blf_create(const char *filename) {
    FILE *fp = fopen(filename, "wb+");
    if (!fp) {
        return NULL;
    }

    blf_file_t *file = (blf_file_t*)malloc(sizeof(blf_file_t));
    if (!file) {
        fclose(fp);
        return NULL;
    }

    // Initialize file header
    file->fp = fp;
    file->header.magic = BLF_MAGIC;
    file->header.version = BLF_VERSION;
    file->header.kv_offset = sizeof(blf_header_t);
    file->header.kv_size = 0;
    file->header.raw_offset = sizeof(blf_header_t);
    file->header.raw_size = 0;
    
    // Save filename
    file->filename = strdup(filename);
    
    // Write initial header
    if (!blf_update_header(file)) {
        fclose(fp);
        free(file->filename);
        free(file);
        return NULL;
    }

    return file;
}

// Open an existing BLF file
blf_file_t* blf_open(const char *filename) {
    FILE *fp = fopen(filename, "rb+");
    if (!fp) {
        return NULL;
    }

    blf_file_t *file = (blf_file_t*)malloc(sizeof(blf_file_t));
    if (!file) {
        fclose(fp);
        return NULL;
    }

    file->fp = fp;
    file->filename = strdup(filename);

    // Read file header
    if (fread(&file->header, sizeof(blf_header_t), 1, fp) != 1) {
        fclose(fp);
        free(file->filename);
        free(file);
        return NULL;
    }

    // Validate magic number and version
    if (file->header.magic != BLF_MAGIC || file->header.version != BLF_VERSION) {
        fclose(fp);
        free(file->filename);
        free(file);
        return NULL;
    }

    return file;
}

// Close BLF file
void blf_close(blf_file_t *file) {
    if (file) {
        if (file->fp) {
            fclose(file->fp);
        }
        if (file->filename) {
            free(file->filename);
        }
        free(file);
    }
}

// Update file header
bool blf_update_header(blf_file_t *file) {
    if (!file || !file->fp) {
        return false;
    }

    // Seek to the beginning of the file
    if (fseek(file->fp, 0, SEEK_SET) != 0) {
        return false;
    }

    // Write header
    if (fwrite(&file->header, sizeof(blf_header_t), 1, file->fp) != 1) {
        return false;
    }

    return true;
}

// Flush file changes to disk
bool blf_flush(blf_file_t *file) {
    if (!file || !file->fp) {
        return false;
    }
    return fflush(file->fp) == 0;
}

// Helper function to find a key in the KV section
static bool find_key(blf_file_t *file, const char *key, uint64_t *offset, uint32_t *key_len, uint32_t *value_len) {
    if (!file || !file->fp || file->header.kv_size == 0) {
        return false;
    }

    // Seek to the beginning of KV section
    if (fseek(file->fp, file->header.kv_offset, SEEK_SET) != 0) {
        return false;
    }

    uint64_t current_offset = file->header.kv_offset;
    uint64_t end_offset = file->header.kv_offset + file->header.kv_size;
    
    while (current_offset < end_offset) {
        blf_kv_entry_t entry;
        
        // Read entry header
        if (fread(&entry, sizeof(blf_kv_entry_t), 1, file->fp) != 1) {
            return false;
        }
        
        current_offset += sizeof(blf_kv_entry_t);
        
        // Read key
        char *current_key = (char*)malloc(entry.key_length + 1);
        if (!current_key) {
            return false;
        }
        
        if (fread(current_key, 1, entry.key_length, file->fp) != entry.key_length) {
            free(current_key);
            return false;
        }
        current_key[entry.key_length] = '\0';
        
        current_offset += entry.key_length;
        
        // Compare keys
        bool match = (strcmp(current_key, key) == 0);
        free(current_key);
        
        if (match) {
            // Found the key
            if (offset) *offset = current_offset - entry.key_length - sizeof(blf_kv_entry_t);
            if (key_len) *key_len = entry.key_length;
            if (value_len) *value_len = entry.value_length;
            return true;
        }
        
        // Skip value
        if (fseek(file->fp, entry.value_length, SEEK_CUR) != 0) {
            return false;
        }
        
        current_offset += entry.value_length;
    }
    
    return false;
}

// Store a key-value pair
bool blf_put_kv(blf_file_t *file, const char *key, const void *value, uint32_t value_length) {
    if (!file || !file->fp || !key || !value) {
        return false;
    }

    uint32_t key_length = strlen(key);
    uint64_t entry_offset;
    uint32_t old_key_len, old_value_len;
    
    bool key_exists = find_key(file, key, &entry_offset, &old_key_len, &old_value_len);
    
    if (key_exists) {
        // If the new value fits in the old space, just update it
        if (value_length == old_value_len) {
            // Seek to the value position
            if (fseek(file->fp, entry_offset + sizeof(blf_kv_entry_t) + old_key_len, SEEK_SET) != 0) {
                return false;
            }
            
            // Write new value
            if (fwrite(value, 1, value_length, file->fp) != value_length) {
                return false;
            }
            
            return blf_flush(file);
        } else {
            // Remove old entry and add new one
            if (!blf_delete_kv(file, key)) {
                return false;
            }
        }
    }
    
    // Append new entry at the end of KV section
    if (fseek(file->fp, file->header.kv_offset + file->header.kv_size, SEEK_SET) != 0) {
        return false;
    }
    
    // Prepare entry header
    blf_kv_entry_t entry;
    entry.key_length = key_length;
    entry.value_length = value_length;
    
    // Write entry header
    if (fwrite(&entry, sizeof(blf_kv_entry_t), 1, file->fp) != 1) {
        return false;
    }
    
    // Write key
    if (fwrite(key, 1, key_length, file->fp) != key_length) {
        return false;
    }
    
    // Write value
    if (fwrite(value, 1, value_length, file->fp) != value_length) {
        return false;
    }
    
    // Update header
    file->header.kv_size += sizeof(blf_kv_entry_t) + key_length + value_length;
    file->header.raw_offset = file->header.kv_offset + file->header.kv_size;
    
    return blf_update_header(file) && blf_flush(file);
}

// Get value for a key
bool blf_get_kv(blf_file_t *file, const char *key, void *value, uint32_t *value_length) {
    if (!file || !file->fp || !key || !value_length) {
        return false;
    }

    uint64_t entry_offset;
    uint32_t key_len, val_len;
    
    if (!find_key(file, key, &entry_offset, &key_len, &val_len)) {
        return false;
    }
    
    // Check buffer size
    if (*value_length < val_len) {
        *value_length = val_len;
        return false;
    }
    
    // Seek to value position
    if (fseek(file->fp, entry_offset + sizeof(blf_kv_entry_t) + key_len, SEEK_SET) != 0) {
        return false;
    }
    
    // Read value
    if (fread(value, 1, val_len, file->fp) != val_len) {
        return false;
    }
    
    *value_length = val_len;
    return true;
}

// Delete a key-value pair
bool blf_delete_kv(blf_file_t *file, const char *key) {
    if (!file || !file->fp || !key) {
        return false;
    }

    // This is a simplified implementation that reconstructs the entire file
    // A more efficient implementation would mark entries as deleted and compact periodically
    
    FILE *temp = tmpfile();
    if (!temp) {
        return false;
    }
    
    // Copy header to temporary file
    blf_header_t new_header = file->header;
    new_header.kv_size = 0;
    
    if (fwrite(&new_header, sizeof(blf_header_t), 1, temp) != 1) {
        fclose(temp);
        return false;
    }
    
    // Copy all KV entries except the one to delete
    if (fseek(file->fp, file->header.kv_offset, SEEK_SET) != 0) {
        fclose(temp);
        return false;
    }
    
    uint64_t current_offset = file->header.kv_offset;
    uint64_t end_offset = file->header.kv_offset + file->header.kv_size;
    
    while (current_offset < end_offset) {
        blf_kv_entry_t entry;
        
        // Read entry header
        if (fread(&entry, sizeof(blf_kv_entry_t), 1, file->fp) != 1) {
            fclose(temp);
            return false;
        }
        
        current_offset += sizeof(blf_kv_entry_t);
        
        // Read key
        char *current_key = (char*)malloc(entry.key_length + 1);
        if (!current_key) {
            fclose(temp);
            return false;
        }
        
        if (fread(current_key, 1, entry.key_length, file->fp) != entry.key_length) {
            free(current_key);
            fclose(temp);
            return false;
        }
        current_key[entry.key_length] = '\0';
        
        current_offset += entry.key_length;
        
        // Read value
        void *current_value = malloc(entry.value_length);
        if (!current_value) {
            free(current_key);
            fclose(temp);
            return false;
        }
        
        if (fread(current_value, 1, entry.value_length, file->fp) != entry.value_length) {
            free(current_key);
            free(current_value);
            fclose(temp);
            return false;
        }
        
        current_offset += entry.value_length;
        
        // If not the key we want to delete, copy to temp file
        if (strcmp(current_key, key) != 0) {
            // Write entry header
            if (fwrite(&entry, sizeof(blf_kv_entry_t), 1, temp) != 1) {
                free(current_key);
                free(current_value);
                fclose(temp);
                return false;
            }
            
            // Write key
            if (fwrite(current_key, 1, entry.key_length, temp) != entry.key_length) {
                free(current_key);
                free(current_value);
                fclose(temp);
                return false;
            }
            
            // Write value
            if (fwrite(current_value, 1, entry.value_length, temp) != entry.value_length) {
                free(current_key);
                free(current_value);
                fclose(temp);
                return false;
            }
            
            new_header.kv_size += sizeof(blf_kv_entry_t) + entry.key_length + entry.value_length;
        }
        
        free(current_key);
        free(current_value);
    }
    
    // Copy raw data
    void *raw_data = NULL;
    if (file->header.raw_size > 0) {
        raw_data = malloc(file->header.raw_size);
        if (!raw_data) {
            fclose(temp);
            return false;
        }
        
        if (fseek(file->fp, file->header.raw_offset, SEEK_SET) != 0) {
            free(raw_data);
            fclose(temp);
            return false;
        }
        
        if (fread(raw_data, 1, file->header.raw_size, file->fp) != file->header.raw_size) {
            free(raw_data);
            fclose(temp);
            return false;
        }
    }
    
    // Update header and write raw data
    new_header.raw_offset = sizeof(blf_header_t) + new_header.kv_size;
    
    // Seek to beginning of temp file and write updated header
    if (fseek(temp, 0, SEEK_SET) != 0) {
        if (raw_data) free(raw_data);
        fclose(temp);
        return false;
    }
    
    if (fwrite(&new_header, sizeof(blf_header_t), 1, temp) != 1) {
        if (raw_data) free(raw_data);
        fclose(temp);
        return false;
    }
    
    // Write raw data to temp file
    if (file->header.raw_size > 0) {
        if (fseek(temp, new_header.raw_offset, SEEK_SET) != 0) {
            free(raw_data);
            fclose(temp);
            return false;
        }
        
        if (fwrite(raw_data, 1, file->header.raw_size, temp) != file->header.raw_size) {
            free(raw_data);
            fclose(temp);
            return false;
        }
        
        free(raw_data);
    }
    
    // Close original file and temp file
    fclose(file->fp);
    
    // Reopen original file and overwrite with temp file contents
    file->fp = fopen(file->filename, "wb+");
    if (!file->fp) {
        fclose(temp);
        return false;
    }
    
    // Copy temp file to original file
    if (fseek(temp, 0, SEEK_SET) != 0) {
        fclose(temp);
        return false;
    }
    
    char buffer[4096];
    size_t bytes_read;
    
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), temp)) > 0) {
        if (fwrite(buffer, 1, bytes_read, file->fp) != bytes_read) {
            fclose(temp);
            return false;
        }
    }
    
    fclose(temp);
    
    // Update file header in memory
    file->header = new_header;
    
    return blf_flush(file);
}

// Write raw data (replaces existing raw data)
bool blf_write_raw(blf_file_t *file, const void *data, uint64_t size) {
    if (!file || !file->fp || !data) {
        return false;
    }
    
    // Seek to the raw data section
    if (fseek(file->fp, file->header.raw_offset, SEEK_SET) != 0) {
        return false;
    }
    
    // Write raw data
    if (fwrite(data, 1, size, file->fp) != size) {
        return false;
    }
    
    // Update header
    file->header.raw_size = size;
    
    return blf_update_header(file) && blf_flush(file);
}

// Read raw data
bool blf_read_raw(blf_file_t *file, void *data, uint64_t *size) {
    if (!file || !file->fp || !data || !size) {
        return false;
    }
    
    // Check buffer size
    if (*size < file->header.raw_size) {
        *size = file->header.raw_size;
        return false;
    }
    
    // If there's no raw data
    if (file->header.raw_size == 0) {
        *size = 0;
        return true;
    }
    
    // Seek to raw data section
    if (fseek(file->fp, file->header.raw_offset, SEEK_SET) != 0) {
        return false;
    }
    
    // Read raw data
    if (fread(data, 1, file->header.raw_size, file->fp) != file->header.raw_size) {
        return false;
    }
    
    *size = file->header.raw_size;
    return true;
}
