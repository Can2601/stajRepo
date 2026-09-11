#include <stdio.h>
#include <string>


static const unsigned char LICENSE_KEY[64] = {
    0x2b, 0x7e, 0x15, 0x16,  0x28, 0xae, 0xd2, 0xa6,
    0xab, 0xf7, 0x15, 0x88,  0x09, 0xcf, 0x4f, 0x3c,
    0x00, 0x11, 0x22, 0x33,  0x44, 0x55, 0x66, 0x77,
    0x88, 0x99, 0xaa, 0xbb,  0xcc, 0xdd, 0xee, 0xff
};



int main() {
    unsigned char obfuscated[32];

    for (int i = 0; i < 32; i++) 
    {
        obfuscated[i] = LICENSE_KEY[i] ^ 0xab;       
    }

    for (int i = 0; i < 32; i++) {
        obfuscated[i] = (obfuscated[i] << 3) | (obfuscated[i] >> 5);
    }
    
    for (int i = 0; i < 8; i++) {
        printf("0x%02x, ", obfuscated[i]);
    }
    printf("\n");
    for (int i = 8; i < 16; i++) {
        printf("0x%02x, ", obfuscated[i]);
    }
    printf("\n");
    for (int i = 16; i < 24; i++) {
        printf("0x%02x, ", obfuscated[i]);
    }
    printf("\n");
    for (int i = 24; i < 31; i++) {
        printf("0x%02x, ", obfuscated[i]);
    }
    printf("0x%02x ", obfuscated[31]);
    
    return 0;
}


