#include "crypto.h"
#include <openssl/sha.h>
#include <openssl/pem.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <sstream>
#include <iomanip>
#include <fstream>

Wallet::Wallet() : rsa_key(nullptr) {}

Wallet::~Wallet()
{
    if (rsa_key)
    {
        RSA_free(rsa_key);
    }
}
    /*Generating a new 2048-bit RSA public/private key pair.*/
bool Wallet::generateKeys()
{
    BIGNUM * bne = BN_new(); // Creating a really big number

    if (!bne || !BN_set_word(bne, RSA_F4))
    {
        BN_free(bne);
        return false;
    }

    /*Creating a new RSA key structure*/
    rsa_key = RSA_new();

    if (!rsa_key || !RSA_generate_key_ex(rsa_key, 2048, bne, nullptr))
    { 
        RSA_free(rsa_key);
        return false;
    }

    BN_free(bne); // freeing the exponent

    /*Extracting a public key to a string*/
    BIO* bio_public = BIO_new(BIO_s_mem());
    PEM_write_bio_RSAPublicKey(bio_public, rsa_key);
    char* public_key_c;
    long public_key_len = BIO_get_mem_data(bio_public, &public_key_c);
    public_key_str.assign(public_key_c, public_key_len);
    BIO_free_all(bio_public);

    BIO* bio_private = BIO_new(BIO_s_mem());
    PEM_write_bio_RSAPrivateKey(bio_private, rsa_key, NULL, NULL, 0, NULL, NULL);
    char* private_key_c;
    long priv_key_len = BIO_get_mem_data(bio_private, &private_key_c);
    private_key_str.assign(private_key_c, priv_key_len);
    BIO_free_all(bio_private);

    address = Crypto::sha256(public_key_str);
    return true;
}

    /*Saving the Private Key to a local file*/
bool Wallet::saveToFile(const std::string& filename)
{
    ofstream file(filename);
    
    if (!file.is_open())
    {
        return false;
    }

    file << private_key_str;
    file.close();
    return true;
}

bool Wallet::loadFromFile(const string& filename)
{
    ifstream file(filename);
    if (!file.is_open())
    {
        return false;
    }

    stringstream buffer;
    buffer << file.rdbuf();
    private_key_str = buffer.str();
    file.close();

    BIO* bio = BIO_new_mem_buf(private_key_str.c_str(), -1);
    if (!bio)
    {
        return false;
    }

    if (rsa_key)
    {
        RSA_free(rsa_key);
    }
    rsa_key = PEM_read_bio_RSAPrivateKey(bio, nullptr, nullptr, nullptr);
    BIO_free(bio);

    if (!rsa_key)

    {
        return false;
    }

    BIO* bio_pub = BIO_new(BIO_s_mem());
    PEM_write_bio_RSAPublicKey(bio_pub, rsa_key);
    char* public_key_c;
    long public_key_len = BIO_get_mem_data(bio_pub, &public_key_c);
    public_key_str.assign(public_key_c, public_key_len);
    BIO_free_all(bio_pub);

    address = Crypto::sha256(public_key_str);
    return true;
}