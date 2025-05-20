#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/*
Note(@Rackhamn): 
    This code has not been tested.
    Also! It does not care about the Endianess at all...
        The Pico W is Little Endian (LE)
        And all data sent to pico will be in BE (Internet Endianess)
        Use ntohl() or be32toh() as needed
*/

// rms_block_32b_s 
typedef struct {
    uint8_t     id;
    uint16_t    size;
    uint8_t *   data;
    uint32_t    crc32;
} block_t;

// rms_payload_32b_s 
typedef struct {
    uint32_t    hash_id;
    uint32_t    size; // payload_size_bytes
    uint8_t     num_blocks;
    block_t *   blocks;
    uint8_t *   zero_padding;
} payload_t;

// Function to extract payload_t from a buffer
payload_t *parse_payload(const uint8_t *buffer, size_t buffer_size) {
    if (buffer == NULL || buffer_size < sizeof(payload_t)) return NULL;

    const uint8_t *ptr = buffer;
    
    // Allocate payload struct 
    // On the PicoW we should already have a scratch buffer to RW with.
    // The memory manager would just return a ptr or NULL :)
    payload_t *payload = (payload_t *)malloc(sizeof(payload_t));
    if (!payload) return NULL;

    // Copy fixed-size fields
    memcpy(&payload->hash_id, ptr, sizeof(uint32_t)); ptr += sizeof(uint32_t);
    memcpy(&payload->size, ptr, sizeof(uint32_t)); ptr += sizeof(uint32_t);
    memcpy(&payload->num_blocks, ptr, sizeof(uint8_t)); ptr += sizeof(uint8_t);

    // Allocate memory for block array
    // This data would also use the memory manager / scratch arena.
    payload->blocks = (block_t *)malloc(payload->num_blocks * sizeof(block_t));
    if (!payload->blocks) {
        free(payload);
        return NULL;
    }

    // Parse each block
    for (uint8_t i = 0; i < payload->num_blocks; i++) {
        block_t *block = &payload->blocks[i];

        memcpy(&block->id, ptr, sizeof(uint8_t)); ptr += sizeof(uint8_t);
        memcpy(&block->size, ptr, sizeof(uint16_t)); ptr += sizeof(uint16_t);

        // Allocate memory for block data
        block->data = (uint8_t *)malloc(block->size);
        if (!block->data) {
            // Cleanup on failure
            for (uint8_t j = 0; j < i; j++) free(payload->blocks[j].data);
            free(payload->blocks);
            free(payload);
            return NULL;
        }

        memcpy(block->data, ptr, block->size); ptr += block->size;
        memcpy(&block->crc32, ptr, sizeof(uint32_t)); ptr += sizeof(uint32_t);
    }

    // Zero-padding (optional, based on buffer size)
    size_t padding_size = buffer_size - (ptr - buffer);
    if (padding_size > 0) {
        payload->zero_padding = (uint8_t *)malloc(padding_size);
        if (payload->zero_padding) {
            memcpy(payload->zero_padding, ptr, padding_size);
        }
    } else {
        payload->zero_padding = NULL;
    }

    return payload;
}

// Free allocated memory
// When using a scratch arena, this code would NEVER be called.
// we would just zero the region and update the freelist or just move scratch ptr to offset 0.
void free_payload(payload_t *payload) {
    if (!payload) return;

    for (uint8_t i = 0; i < payload->num_blocks; i++) {
        free(payload->blocks[i].data);
    }
    free(payload->blocks);
    free(payload->zero_padding);
    free(payload);
}
