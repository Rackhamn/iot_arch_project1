# crc32
A small CRC32 hash library in C

## usage
If compiling with gcc, then the crc32_table is alredy initialised to CRC32_POLY at compile time.  
Otherwise, the table is 0 and needs to be computed if you want it to work.  

Initialize the crc32_table from a polynomial with:  
```c
// Builds the poly table and reverses the bits for big endian systems at runtime
void crc32_init(unsigned int poly);
```  

It can be used directly:  
```c
// === singlet ===
// takes a regular cstring (char *)
unsigned int crc32_hash(void * s);

// start and size are in bytes
unsigned int crc32_hash_s(void * s, size_t size);
unsigned int crc32_hash_ss(void * s, size_t start, size_t size);

// === rolling (appending) ===
unsigned int crc32_hash_a(unsigned int hash, void * s);
unsigned int crc32_hash_sa(unsigned int hash, void * s, size_t size);
unsigned int crc32_hash_ssa(unsigned int hash, void * s, size_t start, size_t size);
```


If you use another polynomial, you can dump it with:
```c
// dump static table
void print_crc32_table(char * name, unsigned int poly);

// dump define
void print_crc32_table_def(char * name, unsigned int poly);
```  


## main.c output
This test program takes either nothing or a list of strings to hash as arguments.  
(I used xargs -a strings.txt ./crc32)  

```
 === START CRC32 ===
* Endianess = little_endian
* Poly      = 0xEDB88320

 === crc32 on argv:
./crc32     = 2644315679
Abjure      = 2957274936
Future      = 3477550270
Picnic      = 573629091
Agonistic   = 1679475360
Garland     = 3577671965
Protect     = 1355336158
Airline     = 598288228
Gigantic    = 24304644
Publish     = 1999238621
Bandit      = 544264766
Goofy       = 373366197
Quadrangle  = 3969504663
Banquet     = 1695148408
Government  = 3827921264
Recount     = 853805686
Binoculars  = 1646839428
Redoubtable = 3216527620
Biologist   = 1928214283
Handbook    = 108430058
Reflection  = 2140281442
Blackboard  = 2580158661
Himself     = 4276023153
Reporter    = 2409661092
Board       = 2576811075
Indulge     = 419125806
Ring        = 804149832
Bookworm    = 4283289487
Inflatable  = 3165992227
Salesclerk  = 265979466
Butterscotch = 4157164914
Inimical    = 4068304412
Grandnieces = 2801471879

 === END CRC32 ===
 ```
