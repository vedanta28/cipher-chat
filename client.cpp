#include <bits/stdc++.h>
#include <signal.h>
#include <sys/file.h>
#include "include/const.hpp"
#include "include/util.hpp"
#include "include/rc4.hpp"
#include "include/comms.hpp"
#include "include/connector.hpp"
#include "include/dh.hpp"
#include <unistd.h>
#include <filesystem>

using namespace std;

namespace fs = std::filesystem;

string downloadDirectory;

int sockfd;
long long privateKey;
long long secretKey;
string username;
bool secure;
pair<string, string> removeUsername(string msg)
{
    string name;
    int i = 0;
    while (i < msg.length() && msg[i] != '$')
    {
        name.push_back(msg[i]);
        i++;
    }
    i++;
    if (i >= msg.length())
    {
        string ex = msg;
        rc4_crypt(ex, to_string(secretKey));
        return {ex, "Server"};
    }
    string newMsg = msg.substr(i);
    // cout << name << ": ";
    return {newMsg, name};
}

void *receiveMsg(void *arg)
{
    int sockfd = *((int *)arg);
    while (true)
    {
        comms comm(sockfd);
        string msg = comm.receive();
        string name = "Server";
        if (secure)
        {
            if (msg == "!file")
            {
                msg = comm.receive();
                long long sz = stoll(comm.receive());
                auto file = parseTheString(msg, '/');
                comm.recvfile(downloadDirectory + "over_net_" + file.back(), sz, secretKey);
                continue;
            }
            auto res = removeUsername(msg);
            msg = res.first;
            name = res.second;
            rc4_crypt(msg, to_string(secretKey));
        }
        if (msg == "!close")
        {
            cout << "Server closed the connection." << endl;
            exit(0);
        }
        else if (msg == "!goodbye")
        {
            secure = false;
            secretKey = 0;
            continue;
        }
        else if (msg == "!handshake")
        {
            privateKey = getPrivateKey();
            comm.sendMsg("!key " + to_string(createA(privateKey)));
            continue;
        }
        else if (msg == "!keys")
        {
            string B = comm.receive();
            // cout << B << endl;
            secretKey = createSecretKey(stoll(B), privateKey);
            // cout << secretKey << endl;
            secure = true;
            continue;
        }

        cout << string(65, ' ') << CYN << name << " : " << GRN << msg << NRM << endl
             << endl;
    }
}
void *sendMsg(void *arg)
{
    int sockfd = *((int *)arg);
    while (true)
    {
        string msg;
        getline(cin, msg);
        string prompt = username + "(You): ";
        if (msg.size())
            cout << GOUP << left << CYN << prompt << MAG << msg << NRM << endl
                 << endl;
        if (msg == "!clear")
        {
            cout << CLR;
            continue;
        }
        comms comm(sockfd);
        if (secure)
        {
            auto parsed_message = parseTheString(msg, ' ');
            if (parsed_message[0] == "!file")
            {
                comm.sendMsg("!file");
                comm.sendMsg(parsed_message[1]);
                comm.sendMsg(to_string(get_file_size(parsed_message[1])));
                comm.sendfile(parsed_message[1], secretKey);
                continue;
            }
            if (msg != "!goodbye" && msg != "!close")
                rc4_crypt(msg, to_string(secretKey));
        }
        comm.sendMsg(msg);
        sleep(0.5);
        if (msg == "!close")
            exit(0);
    }
}

void exit_handler(int sig)
{
    comms comm(sockfd);
    comm.sendMsg("!close");
    sleep(0.5);
    comm.disconnect();
    exit(1);
}

void getPublicKeys(string s)
{
    string strG, strP;
    int i = 0;
    while (i < s.length() && s[i] != ' ')
    {
        strG.push_back(s[i]);
        i++;
    }
    i++;
    while (i < s.length() && s[i] != ' ')
    {
        strP.push_back(s[i]);
        i++;
    }
    setPublicKeys(stoll(strG), stoll(strP));
}

int main(int argc, char **argv)
{
    string currentPath = fs::current_path();
    string subdirectory = "Files";

    downloadDirectory = currentPath + "/" + subdirectory + "/";

    if (!fs::exists(downloadDirectory))
    {
        // Create the directory if it doesn't exist
        if (fs::create_directory(downloadDirectory))
        {
            cout << "Directory created: " << downloadDirectory << std::endl;
        }
        else
        {
            cerr << "Failed to create directory: " << downloadDirectory << std::endl;
            exit(1);
        }
    }

    srand(time(NULL));
    pthread_t receiverThread, senderThread;
    string ip_address;
    cout << "Where do you want to connect to?\n";
    cin >> ip_address;
    connector client(0, PORT, ip_address);
    signal(SIGINT, exit_handler);
    sockfd = client.connectToServer();
    cout << "Connected to server" << endl;
    comms comm(sockfd);
    getPublicKeys(comm.receive());
    cout << "Enter your username: ";
    cin >> username;
    comm.sendMsg(username);
    welcomeMessage();
    pthread_create(&receiverThread, NULL, receiveMsg, &sockfd);
    pthread_create(&senderThread, NULL, sendMsg, &sockfd);
    pthread_join(receiverThread, NULL);
    pthread_join(senderThread, NULL);
    comm.disconnect();
    return 0;
}