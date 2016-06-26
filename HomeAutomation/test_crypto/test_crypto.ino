#include "sha1.h"
#include "sha256.h"

#define AIN1 14
#define AIN2 17

volatile unsigned int hash_count;

void setup(){
  // Setup Serial Monitor
  Serial.begin(115200);
  for(int i=0; i<5; i++){
    genRandomNumber("No Print", 32);
  }
  int* serialNumber = genRandomNumber("Print", 8);
}

void loop(){
}

void genRandomNumber(String select, int length){
  // Create a hashing object
  SHA256 hash_sha256;

  // Seed the random number generator for the delay time
  randomSeed(analogRead(AIN1));

  // Wait a random amount of time, up to 512ms
  //delay(random(1,512));

  // Reseed the random number generator for generating the count
  //randomSeed(analogRead(AIN));

  // Grab how many bytes to generate to be hashed, must be at least 1
  hash_count = random(20);

  // Wait a random amount of time, up to 512ms
  delay(random(1,512));

  // Reseed the random number generator for generating "plaintext" bytes
  randomSeed(analogRead(AIN2));

  if(select == "Print"){
    Serial.print("Importing random bytes count of: ");
    Serial.println(hash_count, DEC);
    Serial.print("Input random value: 0x");
  }

  // Write in the value to be hashed
  for(int i=0; i<hash_count; i++){
    uint8_t intByte = random(256);
    if(select == "Print"){
      if(intByte < 16){
        Serial.print("0");
      }
      Serial.print(intByte,HEX);
    }
    hash_sha256.write(intByte);
  }
  if(select == "Print"){
    Serial.println("");
  }

  // Get the hash result
  uint8_t* digest_sha256 = hash_sha256.result();

  if(select == "Print"){
    Serial.print("Resulting Hash (SHA256) value is: 0x");
    for(int i=0; i<hash_sha256.resultLen(); i++){
      if(digest_sha256[i] < 16){
        Serial.print("0");
      }
      Serial.print(digest_sha256[i],HEX);
    }
    Serial.println("");
    Serial.println("");
  }

  // XOR the result back onto itself if the requested length is shorter than
  // the resulting digest
  if(length < 32){
    uint8_t digest_short[length];
    uint8_t splits = 32/length;
    if((32%length) > 0){
      splits++;
    }
    for(int i=0; i<splits; i++){
      if(i==0){
        for(int j=0; j<length; j++){
          digest_short[j] = digest_sha256[j];
        }
      }else{
        for(int j=0; j<length; j++){
          digest_short[j] = digest_short[j] ^ digest_sha256[(i*j + j)];
        }
      }
    }
  }
}
