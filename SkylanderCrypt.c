#include "SkylanderCrypt.h"
#include <openssl/md5.h>
#include "rijndael.h"
#include <string.h>

const char hashConst[] = "Copyright (C) 2010 Activision. All Rights Reserved.";

int computeEncryptionKey(char *keyOut, char *dataBuffer, unsigned int blockNumber) // keyOut Size = 16 character, dataBuffer Size = 1024 characters
{
    char premd5[86];
    // Copy the first 32 from dataBuffer to keyOut
    memcpy(premd5, dataBuffer, 32);
    // Copy blockNumber to keyOut
    memset(premd5 + 32, blockNumber, 1);
    // Copy hashConst to keyOut
    memcpy(premd5 + 33, hashConst, sizeof(hashConst));

    // calculate md5 = keyOut
    MD5_CTX md5;
    MD5_Init(&md5);
    MD5_Update(&md5, premd5, sizeof(premd5));
    MD5_Final(keyOut, &md5);
}

int Decrypt(char *decBlockData, char *aesKey, char *encBlockData) // Block = 0x10 = 16 | AES = 0x10 = 32
{
    DecryptAES128ECB(aesKey, encBlockData, decBlockData);
}

#define KEYBITS 128
void EncryptAES128ECB(unsigned char *key, unsigned char const *plainTextIn, unsigned char *cipherTextOut)
{
    unsigned long rk[RKLENGTH(KEYBITS)];
    int nrounds;
    nrounds = rijndaelSetupEncrypt(rk, key, KEYBITS);
    rijndaelEncrypt(rk, nrounds, plainTextIn, cipherTextOut);
}

void DecryptAES128ECB(unsigned char *key, unsigned char const *cipherTextIn, unsigned char *plainTextOut)
{
    unsigned long rk[RKLENGTH(KEYBITS)];
    int nrounds;
    nrounds = rijndaelSetupDecrypt(rk, key, KEYBITS);
    rijndaelDecrypt(rk, nrounds, cipherTextIn, plainTextOut);
}
