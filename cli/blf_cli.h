#ifndef BLF_CLI_H
#define BLF_CLI_H

// Maximum size for values when reading
#define MAX_VALUE_SIZE 1024 * 1024  // 1MB

// Command functions
static void print_usage(void);
static bool cmd_create(int argc, char **argv);
static bool cmd_info(int argc, char **argv);
static bool cmd_put(int argc, char **argv);
static bool cmd_get(int argc, char **argv);
static bool cmd_delete(int argc, char **argv);
static bool cmd_write_raw(int argc, char **argv);
static bool cmd_read_raw(int argc, char **argv);
static bool cmd_list(int argc, char **argv);

#endif // BLF_CLI_H
