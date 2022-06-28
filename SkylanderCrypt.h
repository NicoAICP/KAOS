#ifndef SKYLANDERCRYPT_H_
#define SKYLANDERCRYPT_H_

int computeEncryptionKey(char *keyOut, char *dataBuffer, unsigned int blockNumber); // keyOut Size = 32 character, dataBuffer Size = 1024 characters
void EncryptAES128ECB(unsigned char *key, unsigned char const *plainTextIn, unsigned char *cipherTextOut);
void DecryptAES128ECB(unsigned char *key, unsigned char const *cipherTextIn, unsigned char *plainTextOut);
int Decrypt(char *decBlockData, char *aesKey, char *encBlockData);
#endif