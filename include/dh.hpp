#ifndef DH
#define DH

#include <random>
using namespace std;
typedef long long ll;
ll MOD = 1e9 + 7;
ll SMOD = 1e3 + 9;

// ll MOD = 1009;
// ll SMOD = 103;

ll G, P;

void generatePublicKeys();
ll getPrivateKey();
ll getRandomPrime(ll mod);
ll modExp(ll a, ll b, ll mod);
bool isPrime(long long n);
ll createA(ll x);
void setPublicKeys(ll g, ll p);
ll createSecretKey(ll A, ll y);

// for server
void generatePublicKeys(){
    pair<ll, ll> ret;
    G = getRandomPrime(MOD);
    P = getRandomPrime(MOD);
}

// for clients
void setPublicKeys(ll g, ll p){
    G = g;
    P = p;
}

ll getPrivateKey(){
    return (getRandomPrime(SMOD));
}

ll getRandomPrime(ll mod){
    ll x;
    do{
        x = (rand() % mod);
    } while (!isPrime(x));
    return x;
}

ll modExp(ll a, ll b, ll mod){
    ll prod = 1;
    while (b > 0){
        if (b & 1)
            prod = (prod * a) % mod;
        a = (a * a) % mod;
        b >>= 1;
    }
    return prod;
}

bool isPrime(long long n){
    if (n <= 1)
        return false;
    if (n <= 3)
        return true;
    if (n % 2 == 0 || n % 3 == 0)
        return false;
    for (ll i=5;i*i<=n;i+=6)
        if (n%i==0||n%(i+2)==0)
            return false;
    return true;
}

ll createA(ll x){
    return modExp(G, x, P);
}

ll createSecretKey(ll A, ll y){
    return modExp(A, y, P);
}

#endif