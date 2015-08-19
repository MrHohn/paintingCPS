/* In order for this code to work, when compiling in linux, the -lcrypto needs to be added to the command. Say for example i wanted to compile this file. I would need 
to type in "gcc AESciph.c -lcrypto" in order for all the main encryption and decryption functions to work.*/

#include <openssl/conf.h> 
#include <openssl/evp.h>
#include <openssl/err.h>
#include <string.h>

int main (void)
{
  // A 256 bit key and 128 bit initialization vector. These are needed in order for the encryption to work.
  unsigned char *key = (unsigned char *)"31415926535897932384626433832795"; 
  unsigned char *iv = (unsigned char *)"01234567890123456";

  unsigned char *plaintext = (unsigned char *)"I do not like chocolate ice cream.";/* A hardcoded message that was used for testing. Have this character array set to 
  equal the array of the message being sent. */

  unsigned char ciphertext[128];//this is where the encrypted descriptors would be stored.
  unsigned char decryptedtext[128];//this is where the decrypted descriptors would be stored.

  int decryptedtext_len, ciphertext_len;//This needs to be part of the code
  
  //Initialization of libraries. Needed in code. 
  ERR_load_crypto_strings();
  OpenSSL_add_all_algorithms();
  OPENSSL_config(NULL);

  //Function call to encrypt plaintext. You will probably need this line.
  ciphertext_len = encrypt (plaintext, strlen ((char *)plaintext), key, iv,
                            ciphertext);

  //The following four lines of code are just to display the plaintext and ciphertext. They are not needed for our code to work.
  printf("Original message is:\n");
  printf("%s\n", plaintext);
  printf("Ciphertext is:\n");
  BIO_dump_fp (stdout, (const char *)ciphertext, ciphertext_len);

  // Function call to decrypt the ciphertext. You will probably need this line.
  decryptedtext_len = decrypt(ciphertext, ciphertext_len, key, iv,decryptedtext);

  // Adds a NULL terminator.
  decryptedtext[decryptedtext_len] = '\0';

  // Shows the decrypted text. This is not needed in our code. 
  printf("Decrypted text is:\n");
  printf("%s\n", decryptedtext);

  // Clean up
  EVP_cleanup();
  ERR_free_strings();

  return 0;
}

void handleErrors(void) //A function to display any error messages that openSSL might give :) Useful for troubleshooting.
{
  ERR_print_errors_fp(stderr);
  abort();
}

int encrypt(unsigned char *plaintext, int plaintext_len, unsigned char *key,unsigned char *iv, unsigned char *ciphertext) //This is the encryption function.
{
  EVP_CIPHER_CTX *ctx;

  int len;
  int ciphertext_len;

  if(!(ctx = EVP_CIPHER_CTX_new())) handleErrors();

  /* Initialise the encryption operation.
   * In this example I used 256 bit AES (i.e. a 256 bit key). The
   * IV size for *most* modes is the same as the block size. For AES this
   * is 128 bits */
  if(1 != EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv))
    handleErrors();

  // Provide the message to be encrypted, and obtain the encrypted output.
  if(1 != EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len))
    handleErrors();
  ciphertext_len = len;

  if(1 != EVP_EncryptFinal_ex(ctx, ciphertext + len, &len)) handleErrors();
  ciphertext_len += len;

  // Clean up 
  EVP_CIPHER_CTX_free(ctx);

  return ciphertext_len;
}

int decrypt(unsigned char *ciphertext, int ciphertext_len, unsigned char *key,unsigned char *iv, unsigned char *plaintext)//Definition of Decryption Function.
{
  EVP_CIPHER_CTX *ctx;

  int len;
  int plaintext_len;

  if(!(ctx = EVP_CIPHER_CTX_new())) handleErrors();

  /* Initialise the decryption operation.
   * In this example I used 256 bit AES (i.e. a 256 bit key). The
   * IV size for *most* modes is the same as the block size. For AES this
   * is 128 bits */
  if(1 != EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv))
    handleErrors();

  // Provide the message to be decrypted, and obtain the plaintext output.
  if(1 != EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len))
    handleErrors();
  plaintext_len = len;

  if(1 != EVP_DecryptFinal_ex(ctx, plaintext + len, &len)) handleErrors();
  plaintext_len += len;

  // Clean up
  EVP_CIPHER_CTX_free(ctx);

  return plaintext_len;
}

