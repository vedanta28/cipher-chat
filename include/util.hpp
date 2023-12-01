#ifndef UTIL
#define UTIL

#include <bits/stdc++.h>
#include <string.h>
#include <sys/fcntl.h>
#include <signal.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include<fcntl.h>
#include "const.hpp"
#include <unistd.h>
#include <fcntl.h>

using namespace std;

vector<string> split (const string &s, char delim) {
    vector<string> result;
    stringstream ss (s);
    string item;
    while (getline (ss, item, delim)) {
        result.push_back(item);
    }
    return result;
}

void err(string s){
    perror(s.c_str());
    exit(1);
}
long long get_file_size(string filename) {
    FILE *p_file = NULL;
    p_file = fopen(filename.c_str(),"rb");
    fseek(p_file,0,SEEK_END);
    long long size = ftell(p_file);
    fclose(p_file);
    return size;
}

string formatBytes(long long bytes) {
    static const char* suffixes[] = {"B", "KB", "MB", "GB", "TB", "PB", "EB", "ZB", "YB"};
    int suffixIndex = 0;
    double size = static_cast<double>(bytes);
    while (size >= 1000 && suffixIndex < sizeof(suffixes) / sizeof(suffixes[0]) - 1) {
        size /= 1000;
        ++suffixIndex;
    }
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << size << " " << suffixes[suffixIndex];
    return oss.str();
}

void progressbar(float progress, int barwidth,int offset){
    if(progress>=1.0){
        progress=1.0;
    }
    cout<<string(offset,' ');
    cout << "[";
    int pos = barwidth * progress;
    for (int i = 0; i < barwidth; ++i) {
        if (i < pos) cout << "=";
        else if (i == pos) cout << ">";
        else cout << " ";
    }
    cout << "] " << int(progress * 100.0) << " %\r";
    cout.flush();
    if(abs(progress-1.0)<=1e-9)
        cout<<endl;
}

vector<string> parseTheString(string &s, char delimiter){
    vector<string> res;
    string curr;
    for (auto x : s){
        if (x == delimiter){
            res.push_back(curr);
            curr = "";
        }
        else
            curr += x;
    }
    res.push_back(curr);
    return res;
}

void welcomeMessage(){
    cout<<GRN<<"\t______________________________________________________________"<<endl;
    cout<<RED<<"\t                    Read This                                 "<<endl;
    cout<<RED<<"\t                Messages are encrypted                        "<<endl;
    cout<<NRM<<"\t1.commands need to be specified by a preceeding !"<<endl;
    cout<<NRM<<"\t2.Commands are as follows:\n";
    cout<<"\t>"<<GRN<<"!clear"<<NRM<<" : "<<"This clears the client-side screen"<<endl;
    cout<<"\t>"<<MAG<<"!test"<<NRM<<" : "<<"This shows the available users and their status"<<endl;
    cout<<"\t>"<<CYN<<"!connect <username>"<<NRM<<" : "<<"This is used to connect to another user"<<endl;
    cout<<"\t>"<<GRN<<"!file <filePath>" <<NRM<<" : "<<"This is used to send files to another user"<<endl;
    cout<<"\t>"<<RED<<"!goodbye"<<NRM<<" : "<<"This exits from an existing chat session"<<endl;
    cout<<"\t>"<<RED<<"!close"<<NRM<<" : "<<"This disconnects the user from the server"<<endl;
    cout<<GRN<<"\t^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^"<<endl;
}
string genRandName(){
    string s = "";
    for (int i = 0; i < 16; i++)
        s.push_back((char)'0' + (rand() % 10));
    return s;
}

std::string charToHexString(char input) {
    std::stringstream stream;
    stream << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(static_cast<unsigned char>(input));
    return stream.str();
}

void transfile(int from,int to,long long size,string serverFile)
{
    char buffer[65536];
    long long bytes_read=0,bytes_written=0,tot_bytes=0;
    int file=open(serverFile.c_str(),O_WRONLY|O_APPEND|O_CREAT|O_TRUNC,0744);
    while (tot_bytes<size) 
    {
        bytes_read = recv(from,buffer, sizeof(buffer)-1,0);
        if (bytes_read == 0) // We're done reading from the file
            break;
        if (bytes_read < 0) 
        {
            perror("could not read :");
            continue;
        }
        buffer[bytes_read]='\0';
        char *p = buffer;

        int copy_bytes_read=bytes_read;
        int copy_bytes_written=bytes_written;
        while(copy_bytes_read>0){
            copy_bytes_written = write(file,p,copy_bytes_read);
            if (copy_bytes_written < 0) 
            {
                perror("write error:");
                continue;
            }
            copy_bytes_read -= copy_bytes_written;
            p += copy_bytes_written;
        }

        p=buffer;
        while (bytes_read > 0) 
        {
            bytes_written = write(to,p,bytes_read);
            if (bytes_written < 0) 
            {
                perror("write error:");
                continue;
            }
            bytes_read -= bytes_written;
            p += bytes_written;
            tot_bytes+=bytes_written;
        }
        progressbar((float)tot_bytes/size,50,0);
    }
    close(file);
}


char hexStringToChar(const std::string& hexString) {
    int value;
    std::stringstream stream(hexString);
    stream >> std::hex >> value;
    return static_cast<char>(value);
}
#endif