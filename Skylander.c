#include "Skylander.h"
#include "SkylanderCrypt.h"
#include <string.h>

void GetName(char* dataBuffer, char* name){
    //Name 1 Part
    char aesKeyName1[] = calloc(16, 1);
    computeEncryptionKey(aesKeyName1,dataBuffer,0x0A);
    char name1Block[] = calloc(16,1);
    memcpy(name1Block, dataBuffer+0x0A, 16);
    char name1[] = calloc (16,1);
    Decrypt(name1, aesKeyName1, name1Block);
    //Name 2 Part
   char aesKeyName2[] = calloc(16, 1);
    computeEncryptionKey(aesKeyName1,dataBuffer,0x0C);
    char name2Block[] = calloc(16,1);
    memcpy(name2Block, dataBuffer+0x0C, 16);
    char name2[] = calloc (16,1);
    Decrypt(name2, aesKeyName2, name2Block);

    //Setname
    memcpy(name, name1, sizeof(name1));
    memcpy(name+sizeof(name1), name2, sizeof(name2));

    free(name2);
    free(name1);
    free(name2Block);
    free(name1Block);
    free(aesKeyName1);
    free(aesKeyName2);
    
}