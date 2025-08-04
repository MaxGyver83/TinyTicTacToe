#include "utils_x11.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utils.h"

bool
streq(char *str, char *other)
{
	return *str == *other && strcmp(str, other) == 0;
}

char *
read_file_to_buffer(const char *filename)
{
	// Open the file in binary mode
	FILE *file = fopen(filename, "rb");
	if (file == NULL) {
		error("Error opening file");
		return NULL;
	}

	// Seek to the end to determine the file size
	if (fseek(file, 0, SEEK_END) != 0) {
		error("Error seeking file");
		fclose(file);
		return NULL;
	}

	long file_size = ftell(file);
	if (file_size == -1) {
		error("Error telling file size");
		fclose(file);
		return NULL;
	}

	rewind(file);

	// Allocate memory for the buffer (+1 for null terminator)
	char *buffer = malloc(file_size + 1);
	if (buffer == NULL) {
		error("Error allocating memory");
		fclose(file);
		return NULL;
	}

	// Read the file into the buffer
	size_t bytes_read = fread(buffer, 1, file_size, file);
	if (bytes_read != (size_t)file_size) {
		error("Error reading file");
		free(buffer);
		fclose(file);
		return NULL;
	}

	// Null-terminate the buffer
	buffer[file_size] = '\0';

	// Close the file and return the buffer
	fclose(file);
	return buffer;
}
