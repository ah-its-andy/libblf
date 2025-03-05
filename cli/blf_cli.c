#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <blf.h>  // Use the installed library header
#include "blf_cli.h"

static void print_usage(void) {
    printf("BLF CLI Tool - Binary Log Format utility\n\n");
    printf("Usage:\n");
    printf("  blf create <filename>                   Create a new BLF file\n");
    printf("  blf info <filename>                     Display information about a BLF file\n");
    printf("  blf put <filename> <key> <value>        Add/update a key-value pair\n");
    printf("  blf get <filename> <key>                Get a value by key\n");
    printf("  blf delete <filename> <key>             Delete a key-value pair\n");
    printf("  blf write-raw <filename> <input-file>   Write raw data from file\n");
    printf("  blf read-raw <filename> <output-file>   Read raw data to file\n");
    printf("  blf list <filename>                     List all key-value pairs\n");
    printf("  blf help                                Display this help message\n");
}

static bool cmd_create(int argc, char **argv) {
    if (argc < 1) {
        fprintf(stderr, "Error: Filename required\n");
        return false;
    }

    blf_file_t *file = blf_create(argv[0]);
    if (!file) {
        fprintf(stderr, "Error: Could not create BLF file '%s'\n", argv[0]);
        return false;
    }

    printf("Created BLF file: %s\n", argv[0]);
    blf_close(file);
    return true;
}

static bool cmd_info(int argc, char **argv) {
    if (argc < 1) {
        fprintf(stderr, "Error: Filename required\n");
        return false;
    }

    blf_file_t *file = blf_open(argv[0]);
    if (!file) {
        fprintf(stderr, "Error: Could not open BLF file '%s'\n", argv[0]);
        return false;
    }

    printf("BLF File: %s\n", argv[0]);
    printf("  Magic: 0x%08X\n", file->header.magic);
    printf("  Version: %u\n", file->header.version);
    printf("  KV Section Offset: %lu bytes\n", file->header.kv_offset);
    printf("  KV Section Size: %lu bytes\n", file->header.kv_size);
    printf("  Raw Data Offset: %lu bytes\n", file->header.raw_offset);
    printf("  Raw Data Size: %lu bytes\n", file->header.raw_size);

    blf_close(file);
    return true;
}

static bool cmd_put(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "Error: Filename, key, and value required\n");
        return false;
    }

    blf_file_t *file = blf_open(argv[0]);
    if (!file) {
        fprintf(stderr, "Error: Could not open BLF file '%s'\n", argv[0]);
        return false;
    }

    const char *key = argv[1];
    const char *value = argv[2];
    
    if (!blf_put_kv(file, key, value, strlen(value))) {
        fprintf(stderr, "Error: Could not store key-value pair\n");
        blf_close(file);
        return false;
    }

    printf("Added key-value pair: %s=%s\n", key, value);
    blf_close(file);
    return true;
}

static bool cmd_get(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Error: Filename and key required\n");
        return false;
    }

    blf_file_t *file = blf_open(argv[0]);
    if (!file) {
        fprintf(stderr, "Error: Could not open BLF file '%s'\n", argv[0]);
        return false;
    }

    const char *key = argv[1];
    char value[MAX_VALUE_SIZE] = {0};
    uint32_t value_len = MAX_VALUE_SIZE;

    if (!blf_get_kv(file, key, value, &value_len)) {
        if (value_len > MAX_VALUE_SIZE) {
            fprintf(stderr, "Error: Value too large (size: %u)\n", value_len);
        } else {
            fprintf(stderr, "Error: Key '%s' not found\n", key);
        }
        blf_close(file);
        return false;
    }

    // Ensure null-termination for safe printing
    value[value_len < MAX_VALUE_SIZE ? value_len : MAX_VALUE_SIZE-1] = '\0';
    printf("%s\n", value);
    
    blf_close(file);
    return true;
}

static bool cmd_delete(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Error: Filename and key required\n");
        return false;
    }

    blf_file_t *file = blf_open(argv[0]);
    if (!file) {
        fprintf(stderr, "Error: Could not open BLF file '%s'\n", argv[0]);
        return false;
    }

    const char *key = argv[1];
    
    if (!blf_delete_kv(file, key)) {
        fprintf(stderr, "Error: Could not delete key '%s'\n", key);
        blf_close(file);
        return false;
    }

    printf("Deleted key: %s\n", key);
    blf_close(file);
    return true;
}

static bool cmd_write_raw(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Error: BLF filename and input file required\n");
        return false;
    }

    blf_file_t *file = blf_open(argv[0]);
    if (!file) {
        fprintf(stderr, "Error: Could not open BLF file '%s'\n", argv[0]);
        return false;
    }

    FILE *input = fopen(argv[1], "rb");
    if (!input) {
        fprintf(stderr, "Error: Could not open input file '%s'\n", argv[1]);
        blf_close(file);
        return false;
    }

    // Get input file size
    fseek(input, 0, SEEK_END);
    long input_size = ftell(input);
    fseek(input, 0, SEEK_SET);

    // Allocate buffer and read data
    void *data = malloc(input_size);
    if (!data) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        fclose(input);
        blf_close(file);
        return false;
    }

    if (fread(data, 1, input_size, input) != (size_t)input_size) {
        fprintf(stderr, "Error: Failed to read input file\n");
        free(data);
        fclose(input);
        blf_close(file);
        return false;
    }

    fclose(input);

    // Write raw data
    if (!blf_write_raw(file, data, input_size)) {
        fprintf(stderr, "Error: Failed to write raw data\n");
        free(data);
        blf_close(file);
        return false;
    }

    free(data);
    printf("Wrote %ld bytes of raw data\n", input_size);
    blf_close(file);
    return true;
}

static bool cmd_read_raw(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Error: BLF filename and output file required\n");
        return false;
    }

    blf_file_t *file = blf_open(argv[0]);
    if (!file) {
        fprintf(stderr, "Error: Could not open BLF file '%s'\n", argv[0]);
        return false;
    }

    // Check raw data size
    uint64_t raw_size = file->header.raw_size;
    if (raw_size == 0) {
        fprintf(stderr, "No raw data in file\n");
        blf_close(file);
        return false;
    }

    // Allocate buffer
    void *data = malloc(raw_size);
    if (!data) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        blf_close(file);
        return false;
    }

    // Read raw data
    if (!blf_read_raw(file, data, &raw_size)) {
        fprintf(stderr, "Error: Failed to read raw data\n");
        free(data);
        blf_close(file);
        return false;
    }

    FILE *output = fopen(argv[1], "wb");
    if (!output) {
        fprintf(stderr, "Error: Could not open output file '%s'\n", argv[1]);
        free(data);
        blf_close(file);
        return false;
    }

    // Write to output file
    if (fwrite(data, 1, raw_size, output) != raw_size) {
        fprintf(stderr, "Error: Failed to write to output file\n");
        fclose(output);
        free(data);
        blf_close(file);
        return false;
    }

    fclose(output);
    free(data);
    printf("Read %lu bytes of raw data\n", raw_size);
    blf_close(file);
    return true;
}

static bool cmd_list(int argc, char **argv) {
    if (argc < 1) {
        fprintf(stderr, "Error: Filename required\n");
        return false;
    }

    blf_file_t *file = blf_open(argv[0]);
    if (!file) {
        fprintf(stderr, "Error: Could not open BLF file '%s'\n", argv[0]);
        return false;
    }

    // If no key-value pairs
    if (file->header.kv_size == 0) {
        printf("No key-value pairs in file\n");
        blf_close(file);
        return true;
    }

    // Seek to the beginning of KV section
    if (fseek(file->fp, file->header.kv_offset, SEEK_SET) != 0) {
        fprintf(stderr, "Error: Could not seek to KV section\n");
        blf_close(file);
        return false;
    }

    uint64_t current_offset = file->header.kv_offset;
    uint64_t end_offset = file->header.kv_offset + file->header.kv_size;
    int count = 0;
    
    printf("Keys in %s:\n", argv[0]);
    
    while (current_offset < end_offset) {
        blf_kv_entry_t entry;
        
        // Read entry header
        if (fread(&entry, sizeof(blf_kv_entry_t), 1, file->fp) != 1) {
            fprintf(stderr, "Error: Could not read entry header\n");
            blf_close(file);
            return false;
        }
        
        current_offset += sizeof(blf_kv_entry_t);
        
        // Read key
        char *key = (char*)malloc(entry.key_length + 1);
        if (!key) {
            fprintf(stderr, "Error: Memory allocation failed\n");
            blf_close(file);
            return false;
        }
        
        if (fread(key, 1, entry.key_length, file->fp) != entry.key_length) {
            fprintf(stderr, "Error: Could not read key\n");
            free(key);
            blf_close(file);
            return false;
        }
        key[entry.key_length] = '\0';
        
        current_offset += entry.key_length;
        
        printf("  %s (%u bytes value)\n", key, entry.value_length);
        free(key);
        count++;
        
        // Skip value
        if (fseek(file->fp, entry.value_length, SEEK_CUR) != 0) {
            fprintf(stderr, "Error: Could not seek past value\n");
            blf_close(file);
            return false;
        }
        
        current_offset += entry.value_length;
    }
    
    printf("Total: %d key-value pair(s)\n", count);
    blf_close(file);
    return true;
}

int main(int argc, char **argv) {
    // Check arguments
    if (argc < 2) {
        print_usage();
        return EXIT_FAILURE;
    }

    const char *command = argv[1];
    bool success = false;

    // Skip program name and command
    argc -= 2;
    argv += 2;

    if (strcmp(command, "create") == 0) {
        success = cmd_create(argc, argv);
    } else if (strcmp(command, "info") == 0) {
        success = cmd_info(argc, argv);
    } else if (strcmp(command, "put") == 0) {
        success = cmd_put(argc, argv);
    } else if (strcmp(command, "get") == 0) {
        success = cmd_get(argc, argv);
    } else if (strcmp(command, "delete") == 0) {
        success = cmd_delete(argc, argv);
    } else if (strcmp(command, "write-raw") == 0) {
        success = cmd_write_raw(argc, argv);
    } else if (strcmp(command, "read-raw") == 0) {
        success = cmd_read_raw(argc, argv);
    } else if (strcmp(command, "list") == 0) {
        success = cmd_list(argc, argv);
    } else if (strcmp(command, "help") == 0) {
        print_usage();
        success = true;
    } else {
        fprintf(stderr, "Unknown command: %s\n", command);
        print_usage();
    }

    return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
