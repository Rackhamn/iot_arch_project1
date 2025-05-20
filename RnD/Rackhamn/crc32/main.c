#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "crc32.h"

// on my linux machine (x86_64, little endian) the crc32 of "test_data" is 2827671980
int main(int argc, char ** argv) {
	printf(" === START CRC32 ===\n");
	printf("* Endianess = %s\n", get_endian_string());
	printf("* Poly      = 0x%08X\n", CRC32_POLY);
	printf("\n");
	
	// this does not need to be called IF the compiler used the precompiled table 
	crc32_init(CRC32_POLY);
	 
	// print_crc32_table("crc32_256_table", 0xEDB88320);
	// print_crc32_table_def("CRC32_256_TABLE", 0xEDB88320);
	
	char * string;
	unsigned int hash;
	
	if(argc < 2) {
		printf(" === crc32 test:\n");
		
		string = "test_data";
		hash = crc32_hash(string);
		
		printf("%-11s = %u\n", string, hash);
	} else {
		printf(" === crc32 on argv:\n");
		for(int i = 0; i < argc; i++) {
			string = argv[i];
			hash = crc32_hash(string);
			
			printf("%-11s = %u\n", string, hash);
			// printf("%-12s = %-12u %u\n", string, hash, hash % 32);
		}
	}

	printf("\n");
	printf(" === END CRC32 ===\n");
	return 0;
}
