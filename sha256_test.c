#include <stdio.h>
#include <stdint.h>
#include <memory.h>
#include <math.h>

const char fileName[] = "lorem_ipsum.txt";

enum PrintMode {None, Binary, Verbose};
const enum PrintMode PRINTMODE = None;


uint32_t rrot(int x, int n) {
    uint32_t y = (x >> n) & ~(-1 << (32 - n));
    uint32_t z = x << (32 - n);
    return y | z;
}

void printbinchar(char c) {
    for (int i = 7; i >= 0; --i)
    {
        putchar( (c & (1 << i)) ? '1' : '0' );
    }
}

void printbin32int(uint32_t n) {
    for (int i = 31; i >= 0; --i)
    {
        putchar( (n & (1 << i)) ? '1' : '0' );
    }

}

int main(void) {

    // initialize some values
    const int PRIMES[] =    {2, 3, 5, 7, 11, 13, 17, 19,
                            23, 29, 31, 37, 41, 43, 47, 53,
                            59, 61, 67, 71, 73, 79, 83, 89,
                            97, 101, 103, 107, 109, 113, 127, 131,
                            137, 139, 149, 151, 157, 163, 167, 173,
                            179, 181, 191, 193, 197, 199, 211, 223,
                            227, 229, 233, 239, 241, 251, 257, 263,
                            269, 271, 277, 281, 283, 293, 307, 311};

    // generating the k constants
    uint32_t kConstants[64];
    for (int i = 0; i < 64; i++) {
        double cubeRoot = cbrt(PRIMES[i]);
        double fracPart = (cubeRoot - floor(cubeRoot)) * pow(2,32);
        kConstants[i] = (uint32_t)fracPart;
        if (PRINTMODE >= Verbose) {
            printf("k%d:\t", i);
            printbin32int(kConstants[i]);
            printf("\n");
        }
    }

    // initial hash values
    uint32_t hashParts[8];
    uint32_t vars[8];  // working variables a..h stored in vars[]
    for (int i = 0; i < 8; i++) {
        double squareRoot = sqrt(PRIMES[i]);
        double fracPart = (squareRoot - floor(squareRoot)) * pow(2,32);
        hashParts[i] = (uint32_t) fracPart;
        if (PRINTMODE >= Verbose) {
            printf("h%d:\t", i);
            printbin32int(hashParts[i]);
            printf("\n");
        }
    }



    // open the file
    FILE* fptr;
    fptr = fopen(fileName, "rb");
    if (fptr == NULL) {
        printf("Could not find %s\n", fileName);
        return 1;
    }
    printf("Reading %s\n\n", fileName);

    // get the file size
    fseek(fptr, 0L, SEEK_END);
    long int fileSizeBytes = ftell(fptr);
    if (PRINTMODE >= Verbose) printf("The file is %d bytes long.\n", fileSizeBytes);
    fseek(fptr, 0L, SEEK_SET);  // "put that thing back where it came from or so help me"

    // create the message block array
    uint64_t messageBlockSizeBytes = ((fileSizeBytes / 64) + 1) * 64;
    if ((messageBlockSizeBytes - fileSizeBytes) <= 8) messageBlockSizeBytes += 64;
    if (PRINTMODE >= Verbose) printf("The message block will be %d bytes long.\n", messageBlockSizeBytes);
    uint8_t messageBlock[messageBlockSizeBytes];
    memset(messageBlock, 0, sizeof(messageBlock));  // remove any garbage

    // read the file into the message block
    int nextByte = 0;
    uint8_t nextChar;
    while (fread(&nextChar, 1, 1, fptr) > 0) {
        messageBlock[nextByte] = nextChar;
        nextByte++;
    }
    fclose(fptr);  // we're done with the file now

    // append 10000000 to the message block
    messageBlock[nextByte] = 128;

    // the last 8 bytes will be the message length as a 64-bit big-endian integer
    uint64_t fileSizeBits = fileSizeBytes * 8;
    nextByte = sizeof(messageBlock) - 1;
    for (int i = 0; i < 8; ++i) {
        messageBlock[nextByte] = (uint8_t) (fileSizeBits & 0xFF);
        fileSizeBits >>= 8;
        nextByte--;
    }

    // print the message block for my sanity
    if (PRINTMODE > None) {
        nextByte = 0;
        printf("\n");
        while (nextByte < messageBlockSizeBytes) {
            printbinchar(messageBlock[nextByte]);
            printf("%c", (++nextByte % 4) == 0 ? '\n' : ' ');
        }
        printf("\n");
    }

    // MAIN LOOP
    for (int b = 0; b < messageBlockSizeBytes / 64; b++) {

        // STEP 0
        uint32_t messageSchedule[64] = {0};
        for (int i = 0; i < 16; i++) {  // copy the next message block into the first 16 words
            uint32_t nextWord = 0;
            for (int j = 0; j < 4; j++) {  // each word is built byte by byte
                nextWord <<= 8;
                nextWord = nextWord | messageBlock[((i*4)+j) + (b*64)];
            }
            messageSchedule[i] = nextWord;
            if (PRINTMODE > None) {
                printbin32int(nextWord);
                printf("\n");
            }
        }

        // STEP 1
        for (int i = 16; i < 64; i++) {
            uint32_t sigma0 = rrot(messageSchedule[i-15], 7) ^ rrot(messageSchedule[i-15], 18) ^ (messageSchedule[i-15] >> 3);
            uint32_t sigma1 = rrot(messageSchedule[i-2], 17) ^ rrot(messageSchedule[i-2], 19) ^ (messageSchedule[i-2] >> 10);
            messageSchedule[i] = messageSchedule[i-16] + sigma0 + messageSchedule[i-7] + sigma1;

            switch (PRINTMODE) {
                case Verbose:
                    printf("W%d:\t", i-16);
                    printbin32int(messageSchedule[i-16]);
                    printf("\n");

                    printf("s0:\t");
                    printbin32int(sigma0);
                    printf(" +\n");

                    printf("W%d:\t", i-7);
                    printbin32int(messageSchedule[i-7]);
                    printf(" +\n");

                    printf("s1:\t");
                    printbin32int(sigma1);
                    printf(" +\n");

                    printf("W%d:\t", i);
                    printbin32int(messageSchedule[i]);
                    printf(" =\n\n");
                    break;

                case Binary:
                    printbin32int(messageSchedule[i]);
                    printf("\n");
                    break;
            }
        }

        // STEP 2
        for (int i = 0; i < 8; i++) {
            vars[i] = hashParts[i];
        }

        for (int i = 0; i < 64; i++) {
            uint32_t sigma0 = rrot(vars[0], 2) ^ rrot(vars[0], 13) ^ rrot(vars[0], 22);
            uint32_t sigma1 = rrot(vars[4], 6) ^ rrot(vars[4], 11) ^ rrot(vars[4], 25);
            uint32_t majority = (vars[0] & vars[1]) | (vars[1] & vars[2]) | (vars[2] & vars[0]);
            uint32_t choice = (vars[4] & vars[5]) | (~vars[4] & vars[6]);
            uint32_t temp1 = vars[7] + sigma1 + choice + kConstants[i] + messageSchedule[i];
            uint32_t temp2 = majority + sigma0;
            vars[7] = vars[6];
            vars[6] = vars[5];
            vars[5] = vars[4];
            vars[4] = vars[3] + temp1;
            vars[3] = vars[2];
            vars[2] = vars[1];
            vars[1] = vars[0];
            vars[0] = temp1 + temp2;
        }

        for (int i = 0; i < 8; i++) {
            hashParts[i] = hashParts[i] + vars[i];
        }
    }

    printf("SHA-256 hash:\n");
    for (int i = 0; i < 8; i++) {
        printf("%x", hashParts[i]);
    }
    printf("\n");
    return 0;
}
