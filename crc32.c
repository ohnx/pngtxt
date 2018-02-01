/* uint32_t */
#include <stdint.h>

#define crc32_lookup_size 256

/* Code sourced from https://www.w3.org/TR/PNG-CRCAppendix.html */

 /* Table of CRCs of all 8-bit messages. */
   uint32_t crc_table[256];
   
   /* Flag: has the table been computed? Initially false. */
   int crc_table_computed = 0;
   
   /* Make the table for a fast CRC. */
   void make_crc_table(void)
   {
     uint32_t c;
     int n, k;
   
     for (n = 0; n < 256; n++) {
       c = (uint32_t) n;
       for (k = 0; k < 8; k++) {
         if (c & 1)
           c = 0xedb88320L ^ (c >> 1);
         else
           c = c >> 1;
       }
       crc_table[n] = c;
     }
     crc_table_computed = 1;
   }
   
   /* Update a running CRC with the bytes buf[0..len-1]--the CRC
      should be initialized to all 1's, and the transmitted value
      is the 1's complement of the final running CRC (see the
      crc() routine below)). */
   
   uint32_t update_crc(uint32_t crc, unsigned char *buf,
                            uint32_t len)
   {
     uint32_t c = crc;
     uint32_t n;
   
     if (!crc_table_computed)
       make_crc_table();
     for (n = 0; n < len; n++) {
       c = crc_table[(c ^ buf[n]) & 0xff] ^ (c >> 8);
     }
     return c;
   }
   
   /* Return the CRC of the bytes buf[0..len-1]. */
   uint32_t crc32(unsigned char *buf, uint32_t len)
   {
     return update_crc(0xffffffffL, buf, len) ^ 0xffffffffL;
   }
