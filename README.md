# Binary Layout Format (BLF)

BLF is a binary file format that provides two separate sections:
1. A key-value (KV) section for structured data
2. A raw data section for arbitrary binary data

The format allows independent modifications to either section without affecting the other.

## File Format Structure

```
+----------------+
| File Header    | (40 bytes)
+----------------+
| KV Section     | (variable size)
+----------------+
| Raw Section    | (variable size)
+----------------+
```

### File Header

The file header is 40 bytes:

| Field      | Type     | Size    | Description           |
|------------|----------|--------|-----------------------|
| magic      | uint32_t | 4 bytes | Magic number (0x42B1F000) |
| version    | uint32_t | 4 bytes | Format version       |
| kv_offset  | uint64_t | 8 bytes | KV section offset    |
| kv_size    | uint64_t | 8 bytes | KV section size      |
| raw_offset | uint64_t | 8 bytes | Raw section offset   |
| raw_size   | uint64_t | 8 bytes | Raw section size     |

### KV Section

The KV section contains multiple key-value entries:

```
+----------------+----------------+----------------+----------------+----------------+
| Key Length     | Value Length   | Key Data       | Value Data     | ...            |
| (4 bytes)      | (4 bytes)      | (Key Length)   | (Value Length) | ...            |
+----------------+----------------+----------------+----------------+----------------+
```

### Raw Section

The raw section is simply a contiguous block of binary data.

## API Usage

### Basic Operations

```c
// Create a new BLF file
blf_file_t *file = blf_create("data.blf");

// Store key-value pairs
blf_put_kv(file, "name", "test file", strlen("test file"));
blf_put_kv(file, "version", "1.0", strlen("1.0"));

// Store raw data
char raw_data[] = "This is raw binary data";
blf_write_raw(file, raw_data, strlen(raw_data));

// Close the file
blf_close(file);
```

### Reading Data

```c
// Open an existing BLF file
blf_file_t *file = blf_open("data.blf");

// Read a key-value pair
char value[256];
uint32_t value_len = sizeof(value);
if (blf_get_kv(file, "name", value, &value_len)) {
    value[value_len] = '\0';
    printf("Name: %s\n", value);
}

// Read raw data
char buffer[1024];
uint64_t bytes_read = blf_read_raw(file, buffer, sizeof(buffer));
printf("Read %lu bytes of raw data\n", bytes_read);

// Close the file
blf_close(file);
```

## Building

### Compiling the Library

To build the shared library and test program:

```sh
cd src
make all
```

This will produce:
- `libblf.so` - The shared library
- `test_blf` - Test executable

To clean up build artifacts:

```sh
make clean
```

### Using the Shared Library

To use the shared library in your projects:

1. Include the header file in your code:
   ```c
   #include "blf.h"
   ```

2. When compiling your program, link against the library:
   ```sh
   clang your_program.c -L/path/to/libblf -lblf -o your_program
   ```

3. When running your program, make sure the library is in the library path:
   ```sh
   export LD_LIBRARY_PATH=/path/to/libblf:$LD_LIBRARY_PATH
   ./your_program
   ```

Alternatively, you can install the library to a system directory or specify the runtime library path during compilation.