#ifndef AES_H
#define AES_H

void keyExpand(byte* prevKeyRound, int keyLength, int roundStart);
void aesEncrypt(byte* data, byte* key, int keyLength);
void aesDecrypt(byte* data, byte* key, int keyLength);

#endif
