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

string Wallet::getAddress()
{
    return address;
}

string Wallet::getPublicKey()
{
    return public_key_str;
}


    /*Signing data using the wallet's RSA Private key*/
    // To ensure authenticity and integrity
string Wallet::sign(const string& data)
{
    if (!rsa_key)
    {
        return "";
    }

    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256((unsigned char*)data.c_str(), data.length(), hash);

    unsigned char* sig = new unsigned char[RSA_size(rsa_key)];
    unsigned int sig_len;

    if (RSA_sign(NID_sha256, hash, SHA256_DIGEST_LENGTH, sig, &sig_len, rsa_key))
    {
       string signature = Crypto::base64_encode(sig, sig_len);
       delete[] sig;
       return signature; 
    }

    delete[] sig;
    return "";
}

    /*Verifying whether the signature matches data using the
      RSA public key*/
bool Wallet::verify(const string& publicKey, const string& data, const string& signature)
{
    BIO* bio = BIO_new_mem_buf(publicKey.c_str(), -1);
    if (!bio)
    {
        return false;
    }

    RSA* rsa_pub_key = PEM_read_bio_RSAPublicKey(bio, nullptr, nullptr, nullptr);
    BIO_free(bio);
    
    if (!rsa_pub_key)
    {
        return false;
    }

    vector<unsigned char> decoded_sig = Crypto::base64_decode(signature);
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256((unsigned char*)data.c_str(), data.length(), hash);

    bool result = RSA_verify(NID_sha256, hash, SHA256_DIGEST_LENGTH, decoded_sig.data(), decoded_sig.size(), rsa_pub_key);
    RSA_free(rsa_pub_key);
    return result;
}

namespace Crypto
{
    /*Encrypting data using sha256*/
    string sha256(const string& data)
    {
        unsigned char hash[SHA256_DIGEST_LENGTH];
        SHA256_CTX sha256;
        SHA256_Init(&sha256);
        SHA256_Update(&sha256, data.c_str(), data.size());
        SHA256_Final(hash, &sha256);
        stringstream ss;
        for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
        {
            ss << hex << setw(2) << setfill('0') << (int)hash[i];
        }
        return ss.str();
    }

    /*Encoding the Bio buffer*/
    string base64_encode(const unsigned char* buffer, size_t length)
    {
        BIO *bio;
        BIO *b64;
        BUF_MEM *bufferPtr;

        b64 = BIO_new(BIO_f_base64());
        bio = BIO_new(BIO_s_mem());
        bio = BIO_push(b64, bio);

        BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
        BIO_write(bio, buffer, length);
        BIO_flush(bio);
        BIO_get_mem_ptr(bio, &bufferPtr);
        
        string result(bufferPtr->data, bufferPtr->length);
        BIO_free_all(bio);

        return result;
    }

    vector<unsigned char> base64_decode(const string& input)
    {
        BIO *bio;
        BIO *b64;
        vector<unsigned char> buffer(input.length());

        b64 = BIO_new(BIO_f_base64());
        bio = BIO_new_mem_buf(input.c_str(), input.length());
        bio = BIO_push(b64, bio);

        BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
        int decoded_size = BIO_read(bio, buffer.data(), input.length());
        buffer.resize(decoded_size);
        BIO_free_all(bio);

        return buffer;
    }
}