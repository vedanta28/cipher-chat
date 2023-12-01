#ifndef MIM
#define MIM
#include <bits/stdc++.h>
#include <string.h>
#include <sys/fcntl.h>
#include <signal.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include "rc4.hpp"
#include "util.hpp"


using namespace std;



long long  solve(long long  a, long long  b, long long  m)
{
    a %= m, b %= m;
    long long  n = sqrt(m) + 1;

    long long  an = 1;
    for (long long  i = 0; i < n; ++i)
        an = (an * 1ll * a) % m;

    unordered_map<long long , long long > vals;
    for (long long  q = 0, cur = b; q <= n; ++q)
    {
        vals[cur] = q;
        cur = (cur * 1ll * a) % m;
    }

    for (long long  p = 1, cur = 1; p <= n; ++p)
    {
        cur = (cur * 1ll * an) % m;
        if (vals.count(cur))
        {
            long long  ans = n * p - vals[cur];
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

long long  betterBruteForceUsingDiscreteLog(long long  ga, long long gb, long long  g,long long  p)
{
    // a^x mod m = b
    long long  a = solve(g, ga, p);
    long long  b = solve(g, gb, p);
    long long  key = binpow(ga, b, p);
    return key;
}

void decrypt_file(long long key,string serverFile)
{
    char buffer[65536];
    long long bytes_read=0,bytes_written=0,tot_bytes=0;
    int from=open(serverFile.c_str(),O_RDONLY,0744);
    long long size=get_file_size(serverFile);
    auto split=parseTheString(serverFile,'.');
    string newFileName= "."+split[1]+"_decrypted."+split[2];
    cout<<"Decrypted file can be found at "<<newFileName<<endl;
    // exit(1);
    int to=open(newFileName.c_str(),O_WRONLY|O_APPEND|O_CREAT|O_TRUNC,0744);
    array<int, 256> S, T;
    rc_setup(S, T, to_string(key));
    int i = 0, j = 0;
    auto pgra = [&S, &i, &j] () -> int {
        i = (i + 1) % 256;
        j = (j + S[i]) % 256;
        swap(S[i], S[j]);
        int k = (S[i] + S[j]) % 256;
        return S[k];
    };
    while (tot_bytes<size) {
        bytes_read = read(from,buffer, sizeof(buffer)-1);
        if (bytes_read == 0) // We're done reading from the file
            break;
        if (bytes_read < 0) {
            perror("could not read :");
            continue;
        }
        buffer[bytes_read]='\0';
        for(int i=0;i<bytes_read;i++){
            buffer[i]^=(pgra());
        }
        char *p = buffer;
        while (bytes_read > 0) {
            bytes_written = write(to,p,bytes_read);
            if (bytes_written < 0) {
                perror("write error:");
                continue;
            }
            bytes_read -= bytes_written;
            p += bytes_written;
            tot_bytes+=bytes_written;
        }
        progressbar((float)tot_bytes/size,50,0);
    }
    close(from);
    close(to);
}

void fix(string file)
{
    ifstream iF(file);
    int g,p,ga,gb;
    iF>>g>>p>>ga>>gb;
    int key=betterBruteForceUsingDiscreteLog(ga, gb, g, p); // O(sqrt(p))
    cerr<<"The shared secret key is "<<key<<endl;
    cerr<<"Lets proceed to break the encryption on messages and files"<<endl;
    string line;
    auto filename=parseTheString(file,'.');
    ofstream of("."+filename[1]+"_decrypted.txt",ios::app);
    
    array<int, 256> S, T;
    rc_setup(S, T, to_string(key));
    

    while(getline(iF,line)){
        if(line.size()==0) continue;
        // cerr<<line<<endl;
        if(line.substr(0,5)=="!file"){
            string filename=line.substr(6);
            cout<<"Decrypting File "<<filename<<endl;
            decrypt_file(key,filename);
            of<<line<<endl;
            continue;
        }
        int pos=line.find('$');
        string username=line.substr(0,pos-1);
        of<<username<<" : ";
        pos+=2;
        string msg=line.substr(pos);
        // cerr<<msg<<endl;
        auto characters=parseTheString(msg,' ');
        int i = 0, j = 0;
        rc_setup(S, T, to_string(key));
        auto pgra = [&S, &i, &j] () -> int {
            i = (i + 1) % 256;
            j = (j + S[i]) % 256;
            swap(S[i], S[j]);
            int k = (S[i] + S[j]) % 256;
            return S[k];
        };
        for(auto c:characters){
            char ch=hexStringToChar(c);  
            ch^=pgra();
            of<<ch;
        }
        of<<endl;   
    }
}
#endif