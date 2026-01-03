#include "utils.h"
#include "globals.h"
#include "object.h"
#include <string.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <libcd.h>
#include <ctype.h>
#include <stdbool.h>

#define INT32_MIN   ((long)0x80000000)  // −2,147,483,648
#define INT32_MAX   ((long)0x7FFFFFFF)  // +2,147,483,647

char *FileRead(char *filename, u_long *length) {
	CdlFILE filepos;
	int numsectors;
	char *buffer;

	buffer = NULL;

	if (CdSearchFile(&filepos, filename)==NULL) {
		printf("%s file not found on the CD.\n", filename);
	} else {
		printf("Found %s on the CD.\n", filename);
		numsectors = (filepos.size + 2047) / 2048;
		buffer = (char*) malloc3(2048 * numsectors);
		if(!buffer) {
			printf("Error allocating %d sectors!\n", numsectors);
		}
		CdControl(CdlSetloc, (u_char*) &filepos.pos, 0);
		CdRead(numsectors, (u_long*) buffer, CdlModeSpeed);
		CdReadSync(0, 0);
	}
	*length = filepos.size;
	return buffer;
}

void LoadPly(char *filename, Object *obj) { 
	char *data;
	u_long length;

	data = FileRead(filename, &length);
	printf("Read %lu bytes from %s\n", length, filename);

	if(length < 100) { // too small for valid ply
		printf("Ply model file too short for a valid 3D model.\n");
		goto exit;
	} 

	// start readng the .ply file
	u_long byte_counter = 0; // byte counter;

	// read header
	char *header = "ply\n";
	char header_len = 4;
	if(strncmp(header, data, header_len)) {
		printf("Ply file header not recognized.\n");
		goto exit;
	}
	printf("Ply header ok! \n");
	byte_counter += header_len;
	
	// find vertex count
	char *vertex_str = "element vertex ";
	char vertex_str_len = 15;
	while(strncmp(vertex_str, data+byte_counter, vertex_str_len)) {
		byte_counter++;
		if(byte_counter + vertex_str_len > length) {
			printf("Reached end of file, vertex count not found.\n");
			goto exit;
		}
	}
	byte_counter = byte_counter + vertex_str_len;

	// read vertex count
	u_long vertex_count = (u_long)atoi(data+byte_counter);
	if(vertex_count == 0 || vertex_count > 10000) {
		printf("Invalid vertex count: %lu\n", vertex_count);
		goto exit;
	}
	printf("Found vertex count: %lu \n", vertex_count);
	obj->numverts = vertex_count;
	obj->vertices = malloc3(obj->numverts * sizeof(SVECTOR)); // SVECTOR contains 3 shorts vx,vy,vz and 1 padding short
	obj->uvs = malloc3(obj->numverts * sizeof(DVECTOR)); // DVECTOR contains 2 shorts vx, vy

	// find face count
	char *face_str = "element face ";
	char face_str_len = 13;
	while(strncmp(face_str, data+byte_counter, face_str_len)) {
		byte_counter++;
		if(byte_counter + face_str_len > length) {
			printf("Reached end of file, face count not found.\n");
			goto exit;
		}
	}
	byte_counter = byte_counter + face_str_len;

	// read face count
	u_long face_count = (u_long)atoi(data+byte_counter);
	if(face_count == 0 || face_count > 10000) {
		printf("Invalid face count: %lu\n", face_count);
		goto exit;
	}
	printf("Found face count: %lu\n", face_count);
	obj->numfaces = face_count;
	obj->faces = malloc3(obj->numfaces * sizeof(short) * 3); // each face has 3 indices

	// find end header
	char *endh_str = "end_header\n";
	char endh_str_len = 10;
	while(strncmp(endh_str, data+byte_counter, endh_str_len)) {
		byte_counter++;
		if(byte_counter + endh_str_len > length) {
			printf("Reached end of file, end header not found.\n");
			goto exit;
		}
	}
	byte_counter = byte_counter + endh_str_len;
	printf("Ply file end header found.\n");
	
	// parse vertices
	size_t num_len;
	char *end_ptr;
	u_long vertex_index = 0;
	while( vertex_index < vertex_count ) {
		// we are going to step throught all vertices
		short vector_values[3]; // x y z
		u_char i = 0;
		while(i<3) {
			if(byte_counter >= length) {
				printf("Reached end of file\n");
				goto exit;
			}
			short v = strtol(data+byte_counter, &end_ptr, 10);
			num_len = end_ptr - (data+byte_counter);
			if(num_len == 0) {
				if(*(data+byte_counter) == '.') {
					// advance the pointer past the decimal point
					byte_counter++;
					// advance the pointer past all subsequent digits (the decimal part)
					while ( isdigit( *(data+byte_counter) ) ) {
						byte_counter++;
					}
				} else {
					byte_counter++;
				}
				continue;
			}
			// we found a value
			byte_counter += num_len;	
			vector_values[i] = v;
			i++;
		}

		// parse U
		char *scan = data+byte_counter;
		while (*scan == ' ' || *scan == '\t' || *scan == '\n' || *scan == '\r') scan++;
		short u = ParseUVToByte(scan, false);
		while (*scan && !(*scan == ' ' || *scan == '\t' || *scan == '\n' || *scan == '\r')) scan++;
		while (*scan == ' ' || *scan == '\t' || *scan == '\n' || *scan == '\r') scan++;

		// parse V
		short v = ParseUVToByte(scan, true);
		// Advance byte_counter to after this line
		while (*(data + byte_counter) != '\n' && byte_counter < length) byte_counter++;
		byte_counter++;

		obj->vertices[vertex_index].vx = vector_values[0]; // <- flip with last vertex
		obj->vertices[vertex_index].vy = vector_values[1];
		obj->vertices[vertex_index].vz = vector_values[2]; // <- flip with first vertex
		obj->uvs[vertex_index].vx = u;
		obj->uvs[vertex_index].vy = v;
		printf("Vertex %d: x:%d, y:%d, z:%d, u:%d, v:%d\n", 
			vertex_index,
			obj->vertices[vertex_index].vx,
			obj->vertices[vertex_index].vy,
			obj->vertices[vertex_index].vz,
			obj->uvs[vertex_index].vx,
			obj->uvs[vertex_index].vy
		);
		vertex_index++;
	}

	// parse faces
	u_long face_index = 0;
	while( face_index < face_count * 3 ) { // multiply with 3 vertices per face!
		u_short face_values[4]; // first value should always be 3 (for a triangle)
		u_char i=0;
		while(i < 4) {
			if(byte_counter >= length) {
				printf("Reached end of file\n");
				goto exit;
			}
			char *scan = data+byte_counter;
			short f = strtol(scan, &end_ptr, 10);
			num_len = end_ptr - scan;
			if(num_len == 0) {
				if(*(data+byte_counter) == '.') {
					// advance the pointer past the decimal point
					byte_counter++;
					// advance the pointer past all subsequent digits (the decimal part)
					while ( isdigit( *(data+byte_counter) ) ) {
						byte_counter++;
					}
				} else {
					byte_counter++;
				}
				continue;
			}
			// we found a value
			byte_counter += num_len;
			face_values[i] = f;
			i++;
		}
		if(face_values[0]!=3) {
			printf("Face type unknown, is not 3 (triangle).\n");
			goto exit;
		}
		obj->faces[face_index++] = face_values[3]; //<- flip with last vertice
		obj->faces[face_index++] = face_values[2];
		obj->faces[face_index++] = face_values[1]; //<- flip with first vertice
	}

	for(int i = 0; i < face_count; i++) {
		printf("Face %d: %d %d %d\n",
			i+1,
			obj->faces[i*3],
			obj->faces[i*3+1],
			obj->faces[i*3+2]
		);
	}

exit:
	free3(data);
	return;
}

// Convert a normalized UV string (e.g. "0.625" or "1" or "0.000") to 0..63 integer
// Assumes texture is 64x64 (so max coord = 63)
// Uses only integer math – safe on PS1
short ParseUVToByte(const char *str, bool flip) {
	long integer_part = 0;
	long fractional_part = 0;
	int frac_digits = 0;
	int sign = 1;
	int is_negative = 0;

	// Skip whitespace
	while (*str == ' ' || *str == '\t') str++;

	// Sign (shouldn't happen for UVs, but safe)
	if (*str == '-') { is_negative = 1; str++; }
	else if (*str == '+') str++;

	// Integer part (usually 0 or 1 for normalized UVs)
	while (*str >= '0' && *str <= '9') {
		integer_part = integer_part * 10 + (*str - '0');
		str++;
	}

	// Fractional part
	if (*str == '.') {
		str++;
		while (*str >= '0' && *str <= '9' && frac_digits < 6) {  // 6 digits is plenty
		fractional_part = fractional_part * 10 + (*str - '0');
		frac_digits++;
		str++;
		}
	}

	// Combine: value = integer_part + fractional_part / 10^frac_digits
	// Then scale to 0..63 range: uv_byte = (value * 64)  (clamped)
	long value = integer_part;

	if (frac_digits > 0) {
		// Shift fractional part up to full integer
		while (frac_digits < 6) {
		fractional_part *= 10;
		frac_digits++;
		}
		value = integer_part * 1000000L + fractional_part;  // now value = x.xxxxxx in "millionths"
	} else {
		value *= 1000000L;  // no decimal → treat as whole
	}

	// Apply sign (rare for UVs)
	if (is_negative) value = -value;

	// Scale to 0..64 (we want 0..63)
	long scaled = value * 64L / 1000000L;

	// Clamp to 0..63
	if (scaled < 0) scaled = 0;
	if (scaled > 63) scaled = 63;

    	if(flip) {
		scaled = 63-scaled;
	}

	return (short)scaled;
}

char GetChar(u_char *bytes, u_long *b) {
	return bytes[(*b)++];
}

short GetShortLE(u_char *bytes, u_long *b) {
	unsigned short value = 0;
	value = value | bytes[(*b)++] << 0;
	value = value | bytes[(*b)++] << 8;
	return (short)value;
}

short GetShortBE(u_char *bytes, u_long *b) {
	unsigned short value = 0;
	value = value | bytes[(*b)++] << 8;
	value = value | bytes[(*b)++] << 0;
	return (short)value;
}

long GetLongLE(u_char *bytes, u_long *b) {
	u_long value = 0;
	value |= bytes[(*b)++] <<  0;
	value |= bytes[(*b)++] <<  8;
	value |= bytes[(*b)++] << 16;
	value |= bytes[(*b)++] << 24;
	return (long) value;
}

long GetLongBE(u_char *bytes, u_long *b) {
	u_long value = 0;
	value |= bytes[(*b)++] << 24;
	value |= bytes[(*b)++] << 16;
	value |= bytes[(*b)++] <<  8;
	value |= bytes[(*b)++] <<  0;
	return (long) value;
}

