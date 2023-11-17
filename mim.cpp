#include <iostream>
#include <string>
#include <fstream>
#include "rc4.hpp"
#include <cctype>
#include <vector>
#include <unordered_map>
#include <cmath>
#include <set>

using namespace std;

bool containsNonKeyboardChars(const string &input)
{
    for (char c : input)
    {
        if (!isalnum(c) && !ispunct(c) && !isspace(c))
        {
            // c is not an alphanumeric character, punctuation, or a space
            return true;
        }
    }
    return false;
}

void bruteForceOnG(vector<string> &authors, vector<string> &msgs, int p)
{
    vector<int> probKeys;
    for (int i = 0; i < p; i++)
    {
        bool valid = true;
        for (const string &msg : msgs)
        {
            string c = msg;
            rc4_crypt(c, to_string(i));
            if (containsNonKeyboardChars(c))
            {
                valid = false;
                break;
            }
        }
        if (valid == true)
        {
            probKeys.push_back(i);
            cout << "For Key " << i << " messages are:" << endl;
            for (int j = 0; j < authors.size(); j++)
            {
                string c = msgs[j];
                rc4_crypt(c, to_string(i));
                cout << authors[j] << " " << c << endl;
            }
        }
    }
}

int solve(int a, int b, int m)
{
    a %= m, b %= m;
    int n = sqrt(m) + 1;

    int an = 1;
    for (int i = 0; i < n; ++i)
        an = (an * 1ll * a) % m;

    unordered_map<int, int> vals;
    for (int q = 0, cur = b; q <= n; ++q)
    {
        vals[cur] = q;
        cur = (cur * 1ll * a) % m;
    }

    for (int p = 1, cur = 1; p <= n; ++p)
    {
        cur = (cur * 1ll * an) % m;
        if (vals.count(cur))
        {
            int ans = n * p - vals[cur];
            return ans;
        }
    }
    return -1;
}

long long binpow(long long a, long long b, long long m)
{
    a %= m;
    long long res = 1;
    while (b > 0)
    {
        if (b & 1)
            res = res * a % m;
        a = a * a % m;
        b >>= 1;
    }
    return res;
}

void betterBruteForceUsingDiscreteLog(vector<string> &authors, vector<string> &msgs, int ga, int gb, int g, int p)
{
    // a^x mod m = b
    int a = solve(g, ga, p);
    int b = solve(g, gb, p);
    int key = binpow(ga, b, p);

    cout << "For Key " << key << " messages are:" << endl;
    for (int j = 0; j < authors.size(); j++)
    {
        string c = msgs[j];
        rc4_crypt(c, to_string(key));
        cout << authors[j] << " " << c << endl;
    }
}

void bruteForceUsingSmallSecretKey(vector<string> &authors, vector<string> &msgs, int ga, int gb, int g, int p)
{
    vector<int> probA;
    set<int> probKeys;
    for (int a = 0; a <= 1300; a++)
    {
        if (binpow(g, a, p) == ga)
            probA.push_back(a);
    }
    for (const int &a : probA)
    {
        int key = binpow(gb, a, p);
        bool valid = true;
        for (const string &msg : msgs)
        {
            string c = msg;
            rc4_crypt(c, to_string(key));
            if (containsNonKeyboardChars(c))
            {
                valid = false;
                break;
            }
        }
        if (valid)
            probKeys.insert(key);
    }
    for (const int &key : probKeys)
    {
        cout << "For Key " << key << " messages are:" << endl;
        for (int j = 0; j < authors.size(); j++)
        {
            string c = msgs[j];
            rc4_crypt(c, to_string(key));
            cout << authors[j] << " " << c << endl;
        }
    }
}

bool is_digits(const string &str)
{
    return str.find_first_not_of("0123456789") == string::npos;
}

int main()
{
    int ga = -1, gb = -1, g = -1, p = -1;
    ifstream f("./logs/publicKeys.txt");
    if (!f.is_open())
    {
        cerr << "Error opening the file!" << endl;
        return 1;
    }
    string l;
    while (getline(f, l))
    {
        if (g == -1)
            g = stoi(l);
        else if (p == -1)
            p = stoi(l);
    }
    f.close();
    cout << "Enter your desired filename from logs: ";
    string name;
    getline(cin >> ws, name);

    ifstream file("./logs/" + name + ".txt");

    vector<string> authors, msgs;

    if (!file.is_open())
    {
        cerr << "Error opening the file!" << endl;
        return 1;
    }

    string line;

    while (getline(file, line))
    {
        if (is_digits(line))
        {
            if (ga == -1)
                ga = stoi(line);
            else if (gb == -1)
                gb = stoi(line);
        }
        if (line.find("[") != string::npos)
        {
            size_t f = line.find("$");
            if (f != string::npos)
            {
                string msg = line.substr(f + 1);
                string author = line.substr(0, f);
                authors.push_back(author);
                msgs.push_back(msg);
            }
        }
    }

    file.close();

    // bruteForceOnG(authors, msgs, p); // O(p)
    betterBruteForceUsingDiscreteLog(authors, msgs, ga, gb, g, p); // O(sqrt(p))
    bruteForceUsingSmallSecretKey(authors, msgs, ga, gb, g, p);    // O(a)

    return 0;
}
