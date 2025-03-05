#include "blf.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

void test_basic_operations() {
    // Create a new file
    blf_file_t *file = blf_create("/tmp/test.blf");
    assert(file != NULL);
    
    // Add key-value pairs
    const char *key1 = "name";
    const char *value1 = "Test File";
    assert(blf_put_kv(file, key1, value1, strlen(value1)));
    
    const char *key2 = "version";
    const char *value2 = "1.0";
    assert(blf_put_kv(file, key2, value2, strlen(value2)));
    
    // Add raw data
    const char *raw_data = "This is raw data section";
    assert(blf_write_raw(file, raw_data, strlen(raw_data)));
    
    blf_close(file);
    
    // Reopen the file
    file = blf_open("/tmp/test.blf");
    assert(file != NULL);
    
    // Read key-value pairs
    char value_buffer[256];
    uint32_t value_len = sizeof(value_buffer);
    
    assert(blf_get_kv(file, key1, value_buffer, &value_len));
    value_buffer[value_len] = '\0';
    printf("Key: %s, Value: %s\n", key1, value_buffer);
    assert(strcmp(value_buffer, value1) == 0);
    
    value_len = sizeof(value_buffer);
    assert(blf_get_kv(file, key2, value_buffer, &value_len));
    value_buffer[value_len] = '\0';
    printf("Key: %s, Value: %s\n", key2, value_buffer);
    assert(strcmp(value_buffer, value2) == 0);
    
    // Read raw data
    char raw_buffer[256];
    uint64_t raw_len = sizeof(raw_buffer);
    
    assert(blf_read_raw(file, raw_buffer, &raw_len));
    raw_buffer[raw_len] = '\0';
    printf("Raw data: %s\n", raw_buffer);
    assert(strcmp(raw_buffer, raw_data) == 0);
    
    // Modify key-value
    const char *new_value1 = "Updated Test File";
    assert(blf_put_kv(file, key1, new_value1, strlen(new_value1)));
    
    value_len = sizeof(value_buffer);
    assert(blf_get_kv(file, key1, value_buffer, &value_len));
    value_buffer[value_len] = '\0';
    printf("Updated Key: %s, Value: %s\n", key1, value_buffer);
    assert(strcmp(value_buffer, new_value1) == 0);
    
    // Delete a key
    assert(blf_delete_kv(file, key2));
    
    value_len = sizeof(value_buffer);
    assert(blf_get_kv(file, key2, value_buffer, &value_len) == false);
    
    // Modify raw data
    const char *new_raw_data = "Updated raw data section";
    assert(blf_write_raw(file, new_raw_data, strlen(new_raw_data)));
    
    raw_len = sizeof(raw_buffer);
    assert(blf_read_raw(file, raw_buffer, &raw_len));
    raw_buffer[raw_len] = '\0';
    printf("Updated raw data: %s\n", raw_buffer);
    assert(strcmp(raw_buffer, new_raw_data) == 0);
    
    blf_close(file);
}

int main() {
    printf("Testing BLF file format...\n");
    test_basic_operations();
    printf("All tests passed!\n");
    return 0;
}
