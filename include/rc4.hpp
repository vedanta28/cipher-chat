
#ifndef RC4
#define RC4

#include <iostream>
#include <string>
#include <array>

using namespace std;

void rc_setup (array<int, 256> &S, array<int, 256> &T, const string &key) {
    // Initialisation:
    for (int i = 0; i < 256; i++) S[i] = i;
    for (int i = 0; i < 256; i++) T[i] = (int)key[i % key.length()];

    // Key-scheduling algorithm (KSA)
    for (int i = 0, j = 0; i < 256; i++) {
        j = (j + S[i] + T[i]) % 256;
        swap(S[i], S[j]);
    }
}


void rc4_crypt(string &data, const string &key)
{
    if (key.length() > 256) {
        cout << "Key size shouldn't be more than 256" << endl;
        exit(1);
    }

    array<int, 256> S, T;
    rc_setup(S, T, key);

    int i = 0, j = 0;

    auto pgra = [&S, &i, &j] () -> int {
        i = (i + 1) % 256;
        j = (j + S[i]) % 256;
        swap(S[i], S[j]);
        int k = (S[i] + S[j]) % 256;
        return S[k];
    };

    for (char &c : data) {
        c ^= (pgra());
    }
}

#endif