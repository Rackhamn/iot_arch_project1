#ifndef REVERSE_BITS_H
#define REVERSE_BITS_H

unsigned int reverse_u32(unsigned int x)
{
    x = ((x >>  1) & 0x55555555) | ((x & 0x55555555) << 1);
    x = ((x >>  2) & 0x33333333) | ((x & 0x33333333) << 2);
    x = ((x >>  4) & 0x0F0F0F0F) | ((x & 0x0F0F0F0F) << 4);
    x = ((x >>  8) & 0x00FF00FF) | ((x & 0x00FF00FF) << 8);
    x = ((x >> 16) & 0x0000FFFF) | ((x & 0x0000FFFF) << 16);
    return x;
}

#endif /* REVERSE_BITS_H */
