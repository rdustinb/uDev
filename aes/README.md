The AES Encryption and Decryption functions have an external Key
Expansion function that can be called prior to the actual Encryption
or Decryption operations. This allows for streamlined cipher 
operations for maximum throughput as the future cipher implementation
may be pipelined. For instance, normally a session key is only 
generated once per connection which translates down to the key
expansion only needing to be performed once. What changes is the
feedback data block, but the originally expanded key will not change
until the actual cipher key changes.

First, setup the storage and pointers for the key and data:
```
    // Expected 256-bit Encryption
    byte original256Data[16] = {0x00,0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,0x99,0xaa,0xbb,0xcc,0xdd,0xee,0xff};
    byte* original256DataPtr = original256Data;
    byte original256KeyEnc[32] = {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f};
    byte* original256KeyEncPtr = original256KeyEnc;
    byte expanded256Key[256];
    byte* expanded256KeyPtr = expanded256Key;
```

Next, expand the key:
```
    // Expand the Key
    for(int j=0; j<32; j++){
        expanded256KeyPtr[j] = original256KeyEncPtr[j];
    }
    for(int i=1; i<8; i++){
        // Expand Key
        keyExpand(original256KeyEncPtr, 256, (i*8));
        for(int j=0; j<32; j++){
            expanded256KeyPtr[((i*32)+j)] = original256KeyEncPtr[j];
        }
    }
```

Finally, encrypt/decrypt passing in the expanded key:
```
    aesEncrypt(original256DataPtr, expanded256KeyPtr, 256);
    // -- OR --
    aesDecrypt(original256DataPtr, expanded256KeyPtr, 256);
```
