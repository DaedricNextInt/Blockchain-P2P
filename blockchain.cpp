#include "blockchain.h"
#include <iostream>
#include <sstream>> 

using namespace std;

/*Transaction*/
string Transaction::calculate_Hash()
{
    return Crypto::sha256(sender_address + receiving_address +
         to_string(amount) + to_string(timestamp));
}

