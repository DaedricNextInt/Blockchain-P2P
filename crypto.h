#ifndef CRYPTO_H
#define CRYPTO_H

#include <string>
#include <vector>
#include <openssl/rsa.h>

using namespace std;

/* Creating a wallet with RSA keys and an address*/
class Wallet {

public:
    Wallet();
    ~Wallet();

    bool generateKeys();
    bool saveToFile(const string& filename);
    bool loadFromFile(const string& filename);

    string getAddress();
    string getPublicKey();

    string sign(const string& data);
    static bool verify(const string& publicKey, const string& data, const string& signature);

private: 
    RSA* rsa_key_;
    string public_key_str_;
    string private_key_str_;
    string address_;
};


//Encryption 
namespace Crypto 
{
    string sha256(const string& data);
    string base64_encode(const unsigned char* buffer, size_t length);
    vector<unsigned char> base64_decode(const string& input);
}

#endif