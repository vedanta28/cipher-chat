#ifndef COMMS
#define COMMS

#include <bits/stdc++.h>
#include <string.h>
#include <sys/fcntl.h>
#include <signal.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include "rc4.hpp"
#include "util.hpp"
#include "const.hpp"
#include <unistd.h>
#include <fcntl.h>


using namespace std;

class comms {
    int fd;
public:
    comms(int f) : fd(f) {}
    bool sendMsg(string s) {
        string sizeString = to_string(s.size());
        sizeString = string(MESSAGE_SIZE_LENGTH - sizeString.length(), '0') + sizeString;
        string paddedMessage = sizeString + s + string(MESSAGE_CONTENT_LENGTH - s.size(), 'X');
        char buf[MAX_SIZE];
        memcpy(buf, paddedMessage.c_str(), paddedMessage.size());
        int byte_total = 0, byte_now = 0, len = MAX_SIZE;
        while (byte_total < len) {
            if (!((byte_now = send(fd, &buf[byte_total], len - byte_total, 0)) != -1)) {
                perror("cannot send");
                return false;
            }
            byte_total += byte_now;
        }
        return true;
    }
    string receive() {
        char buf[MAX_SIZE] = {0};
        int byte_total = 0, byte_now = 0, len = MAX_SIZE;
        while (byte_total < len) {
            if (!((byte_now = read(fd, &buf[byte_total], len - byte_total)) != -1)) {
                perror("cannot receive");
                return "";
            }
            byte_total += byte_now;
        }
        string s(buf + MESSAGE_SIZE_LENGTH, MESSAGE_CONTENT_LENGTH);
        int sizeofMessage=stoi(string(buf,20));
        s=s.substr(0,sizeofMessage);
        return s;
    }

    void sendfile(string path,long long secretKey){
        long long size=get_file_size(path);
        cout<<YEL<<"Sending a File of size "<<formatBytes(size)<<" ("<<size<<" Bytes ) "<<NRM<<endl;
        char buffer[65536];
        long long bytes_read=0,bytes_written=0,tot_bytes=0;
        int f=open(path.c_str(),O_RDONLY);
        array<int, 256> S, T;
        rc_setup(S, T, to_string(secretKey));
        int i = 0, j = 0;
        auto pgra = [&S, &i, &j] () -> int {
            i = (i + 1) % 256;
            j = (j + S[i]) % 256;
            swap(S[i], S[j]);
            int k = (S[i] + S[j]) % 256;
            return S[k];
        };
        while (1) {
            bytes_read = read(f, buffer, sizeof(buffer));
            if (bytes_read == 0) // We're done reading from the file
                break;
            if (bytes_read < 0) {
                // handle errors
            }
            for(int i=0;i<bytes_read;i++){
                buffer[i]^=(pgra());
            }
            char *p = buffer;
            while (bytes_read > 0) 
            {
                bytes_written = send(fd,p,bytes_read,0);
                if (bytes_written <= 0) {
                    // handle errors
                }
                bytes_read -= bytes_written;
                p += bytes_written;
                tot_bytes+=bytes_written;
            }
            progressbar((float)tot_bytes/size,50,0);
        }
        close(f);
    }
    void recvfile(string path,long long size,long long secretKey)
    {
        cout<<string(65,' ')<<YEL<<"Receiving a File of size "<<formatBytes(size)<<" ("<<size<<" Bytes ) "<<NRM<<endl;
        char buffer[65536];
        long long bytes_read=0,bytes_written=0,tot_bytes=0;
        int f=open(path.c_str(),O_WRONLY|O_APPEND|O_CREAT|O_TRUNC,0744);
        if(f<0){
            perror("something is wrong:");
            exit(1);
        }
        array<int, 256> S, T;
        rc_setup(S, T, to_string(secretKey));
        int i = 0, j = 0;
        auto pgra = [&S, &i, &j] () -> int {
            i = (i + 1) % 256;
            j = (j + S[i]) % 256;
            swap(S[i], S[j]);
            int k = (S[i] + S[j]) % 256;
            return S[k];
        };
        while (tot_bytes<size) {
            bytes_read = recv(fd,buffer, sizeof(buffer)-1,0);
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
                bytes_written = write(f,p,bytes_read);
                if (bytes_written < 0) {
                    perror("write error:");
                    continue;
                }
                bytes_read -= bytes_written;
                p += bytes_written;
                tot_bytes+=bytes_written;
            }
            progressbar((float)tot_bytes/size,50,65);
        }
        close(f);
    }
    void disconnect() {
        close(fd);
    }
};


#endif
