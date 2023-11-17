#include <bits/stdc++.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <cassert>
#include <string>
#include <unordered_map>
#include <cstring>
#include <queue>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <mutex>
#include "dh.hpp"
#include "rc4.hpp"

using namespace std;

#define PORT 8083
#define MAX_SIZE 1024
#define NRM "\x1B[0m"
#define RED "\x1B[31m"
#define GRN "\x1B[32m"
#define YEL "\x1B[33m"
#define BLU "\x1B[34m"
#define MAG "\x1B[35m"
#define CYN "\x1B[36m"
#define WHT "\x1B[37m"
#define GOUP "\033[A\r"
#define CLR "\e[2J\e[H"

// Global Variables:
queue<int> waiting_client_sockets;
mutex mtx;
unordered_map<string, int> usernames;
unordered_map<string, string> partner;
unordered_map<int, string> commonFile;

// Function definations:
string readme(string username);
void sendStatus(string username);
void *handle_client(void *arg);
bool connectToClient(string username, vector<string> &parsedMsg, string &logFile);
void closeSession(string username);
vector<string> parseTheString(string &s, char delimiter);

void err(string s)
{
    perror(s.c_str());
    exit(1);
}

string genRandFileName()
{
    string s = "txt.";
    for (int i = 0; i < 16; i++)
    {
        s.push_back((char)'0' + (rand() % 10));
    }
    reverse(s.begin(), s.end());
    return s;
}

class connector
{
    int sockfd;
    struct sockaddr_in saddr;

public:
    connector(bool s_c, uint16_t port, string addr = "") // s_c is 1 for Server and 0 for Client
    {
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (!~sockfd)
            err("Can't create socket.");
        memset(&saddr, 0, sizeof(sockaddr));
        saddr.sin_family = AF_INET;
        saddr.sin_port = htons(port);
        if (s_c)
        {
            saddr.sin_addr.s_addr = htonl(INADDR_ANY);
            int reuse = 1;
            if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const char *)&reuse, sizeof(reuse)) < 0)
                perror("setsockopt(SO_REUSEADDR) failed");
#ifdef SO_REUSEPORT
            if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, (const char *)&reuse, sizeof(reuse)) < 0)
                perror("setsockopt(SO_REUSEPORT) failed");
#endif
            if (!~bind(sockfd, (sockaddr *)&saddr, sizeof saddr))
                err("Can't bind the socket.");
        }
        else if (!~inet_pton(AF_INET, addr.c_str(), &saddr.sin_addr))
            err("Wrong address format.");
    }
    // Client side functions:
    int connectToServer()
    {
        if (!~connect(sockfd, (sockaddr *)&saddr, sizeof saddr))
            err("Cannot connect.");
        return sockfd;
    }
    // Server side functions:
    void listenForClients()
    {
        if (!~listen(sockfd, 10))
            err("Cannot listen.");
        cout << "Waiting for Connections on port: " << PORT << endl;
    }
    int acceptNow()
    {
        int new_fd;
        socklen_t a = sizeof(saddr);
        if (!~(new_fd = accept(sockfd, (sockaddr *)&saddr, (socklen_t *)&a)))
            err("Cannot accept.");
        return new_fd;
    }
    void closeServer()
    {
        close(sockfd);
    }
};
const int MESSAGE_SIZE_LENGTH = 20;
const int MESSAGE_CONTENT_LENGTH = MAX_SIZE - MESSAGE_SIZE_LENGTH;
void hexDump(const string &data) {
    for (char c : data) {
        cerr << hex << setw(2) << setfill('0') << static_cast<int>(static_cast<unsigned char>(c)) << " ";
    }
    cerr << endl;
}
class comms {
    int fd;

public:
    comms(int f) : fd(f) {}

    bool sendMsg(string s) {
        // Construct the message size header (20 bytes)
        cerr<<"before sending: \n";
        hexDump(s);
        string sizeString = to_string(s.size());
        sizeString = string(MESSAGE_SIZE_LENGTH - sizeString.length(), '0') + sizeString;

        // Pad the message content with random characters to make it (512-20) bytes
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
        // cout<<"hehe"<<endl;
        char buf[MAX_SIZE] = {0};
        int byte_total = 0, byte_now = 0, len = MAX_SIZE;

        // Read until 512 bytes are received
        while (byte_total < len) {
            if (!((byte_now = read(fd, &buf[byte_total], len - byte_total)) != -1)) {
                perror("cannot receive");
                return "";
            }
            byte_total += byte_now;
        }

        // Extract the message content from the buffer
        std::string s(buf + MESSAGE_SIZE_LENGTH, MESSAGE_CONTENT_LENGTH);
        int sizeofMessage=stoi(string(buf,20));
        s=s.substr(0,sizeofMessage);
        cerr<<"after recieving: \n";
        hexDump(s);
        return s;
    }

    void disconnect() {
        close(fd);
    }
};

string sendPublicKeys()
{
    return to_string(G) + " " + to_string(P) + " ";
}

// Function which handles a particular client:
void *handle_client(void *arg)
{
    // First come first serve:
    mtx.lock();
    int client_socket = waiting_client_sockets.front();
    waiting_client_sockets.pop();
    mtx.unlock();

    comms comm(client_socket);
    comm.sendMsg(sendPublicKeys());
    string username = comm.receive();
    // cout<<username<<"heheheheheh"<<endl;

    if (usernames.count(username) > 0)
    {
        comm.sendMsg("Username already exists.\n");
        comm.disconnect();
        return NULL;
    }
    usernames[username] = client_socket;

    //dbg
    // cout<<"now i will send accpet messg to client"<<endl;
    comm.sendMsg("Username accepted.\n");
    cout << GRN << username << " connected." << NRM << endl;
    // comm.sendMsg(readme(username));

    string msg = "";
    string logFile;
    while (true)
    {
        msg = comm.receive();
        if (msg.size() == 0)
            continue;
        vector<string> parsedMsg = parseTheString(msg, ' ');
        if (parsedMsg[0] == "status")
            sendStatus(username);
        else if (parsedMsg[0] == "connect")
        {

            logFile = "./logs/" + genRandFileName();
            connectToClient(username, parsedMsg, logFile);
        }
        else if (parsedMsg[0] == "key")
        {
            string A = parsedMsg[1];
            cout << A << endl;
            comms comm1(usernames[partner[username]]);
            comm1.sendMsg("keys");
            comm1.sendMsg(A);
        }
        else if (parsedMsg[0] == "close")
        {
            closeSession(username);
            break;
        }
        else if (partner.find(username) != partner.end())
        {
            string s;
            s += "\t";
            s += "[" + username + "] $" + msg;
            comms comm1(usernames[partner[username]]);
            // transform s here
            // ----------------
            cout << s << endl;
            comm1.sendMsg(s);

            if (msg == "goodbye")
            {
                cout << username << " said" << msg << endl;
                closeSession(username);
            }
        }
        else
        {
            string s = RED;
            s += "\t[server]: Invalid Command. Please refer to the readme for commands.";
            s += NRM;
            comm.sendMsg(s);
        }
    }
    usernames.erase(username);
    comm.disconnect();
    cout << RED << username << " disconnected." << NRM << endl;
    return NULL;
}

// Sends the status of all the clients to the client:
void sendStatus(string username)
{
    vector<string> names;
    string statusMsg;
    statusMsg += RED;
    statusMsg += "\t[server]:\n";

    for (auto it : usernames)
        names.push_back(it.first);
    for (int i = 0; i < names.size(); i++)
    {
        string status = partner.find(names[i]) == partner.end() ? string(GRN) + "FREE" + string(NRM) : string(RED) + "BUSY" + string(NRM);
        statusMsg += "\t" + string(BLU) + names[i] + string(NRM) + " " + status;
        if (i != names.size() - 1)
            statusMsg += "\n";
    }
    statusMsg += NRM;
    comms comm(usernames[username]);
    comm.sendMsg(statusMsg);
    return;
}

// Connects two clients:
bool connectToClient(string username, vector<string> &parsedMsg, string &logFile)
{
    comms comm(usernames[username]);
    string partnerName = parsedMsg[1];
    cout << "Session request from " + username + " to " + partnerName << endl;
    if (partner.find(username) != partner.end())
    {
        string errMessage = RED;
        errMessage += "\t[server]: You are already in a chat. Please close the current chat to connect to another client.";
        errMessage += NRM;
        comm.sendMsg(errMessage);
        cout << CYN << username << RED << " connection rejected due to already existing connection." << NRM << endl;
        return false;
    }
    if (username == partnerName)
    {
        string errMessage = RED;
        errMessage += "\t[server]: To connect name can't be same as parent connection name";
        errMessage += NRM;
        comm.sendMsg(errMessage);
        cout << CYN << username << RED << " connection rejected as an user can't connect with itself" << NRM << endl;
        cout << RED << "Destination name can't be same as source Name" << NRM << endl;
        return false;
    }
    if (usernames.find(partnerName) == usernames.end())
    {
        string errMessage = RED;
        errMessage += "\t[server]: Username doesn't exist.";
        errMessage += NRM;
        comm.sendMsg(errMessage);
        cout << CYN << username << RED << " connection rejected as " << CYN << partnerName << RED << " is not a valid username." << NRM << endl;
        return false;
    }
    if (partner.find(partnerName) != partner.end())
    {
        string errMessage = RED;
        errMessage += "\t[server]: " + string(CYN) + partnerName + string(NRM) + " is BUSY. Please try after someone, or chat with someone else.";
        errMessage += NRM;
        comm.sendMsg(errMessage);
        cout << CYN << username << RED << " connection rejected as " << CYN << partnerName << RED << " is busy." << NRM << endl;
        return false;
    }
    partner[username] = partnerName;
    partner[partnerName] = username;

    commonFile[usernames[username]] = commonFile[usernames[partnerName]] = logFile;

    cout << GRN << "Connected: " + string(BLU) + username + string(NRM) + " and " << BLU << partnerName << NRM << endl;

    // Notify the clients about their successful connection:
    string msg = GRN;
    msg += "\t[server]: You are now connected to ";
    comm.sendMsg(msg + partnerName + NRM);
    comms comm1(usernames[partnerName]);
    comm1.sendMsg(msg + username + NRM);

    // Exchange keys
    string handshake = "handshake";
    comm.sendMsg(handshake);
    comm1.sendMsg(handshake);

    return true;
}

// Closes the session:
void closeSession(string username)
{
    if (partner.find(username) == partner.end())
    {
        comms comm(usernames[username]);
        string errMessage = RED;
        errMessage += "\t[server]: You are not connected to anyone.";
        errMessage += NRM;
        comm.sendMsg(errMessage);
        cout << RED << "Need to connect to someone to close the connection." << NRM << endl;
        return;
    }

    string partnerName = partner[username];
    string msg = GRN;
    msg += "\n\t[server]: " + username + " closed the chat session" + NRM;
    comms comm1(usernames[partnerName]);
    comm1.sendMsg(msg);
    cout << msg;
    if (partner.find(partnerName) != partner.end())
        partner.erase(partnerName);
    if (partner.find(username) != partner.end())
        partner.erase(username);
    msg = "";
    msg += CYN;
    msg += "\t[server]: Session closed";
    msg += NRM;
    comms comm(usernames[username]);
    comm.sendMsg(msg);
    sleep(0.1);
    string message = "goodbye";
    comm.sendMsg(message);
    comm1.sendMsg(message);
    cout << "Sent goodbyes to both\n";
    if (commonFile.find(usernames[username]) != commonFile.end())
        commonFile.erase(usernames[username]);
    if (commonFile.find(usernames[partnerName]) != commonFile.end())
        commonFile.erase(usernames[partnerName]);

    cout << "Disconnected " + string(CYN) + username + string(NRM) + " and " + string(CYN) + partnerName << NRM << endl;
}

string readme(string username)
{
    string s = CYN;
    s += "\n+------------------------------------------------------------+\n";
    s += "|                                                            |\n";
    s += "|   [server]: Welcome to the chatroom, " + username + string(22 - username.length(), ' ') + "|\n";
    s += "|                                                            |\n";
    s += "|   [server]: Here are the commands you can use:             |\n";
    s += "|                                                            |\n";
    s += "|   1. status             : Lists the status of all users    |\n";
    s += "|   2. connect [username] : Connect to username to start     |\n";
    s += "|                           chatting                         |\n";
    s += "|   3. goodbye            : Ends current chatting session    |\n";
    s += "|   4. close              : Disconnects the user from the    |\n";
    s += "|                           server                           |\n";
    s += "|   5. clear              : Clears the chat from the window  |\n";
    s += "|   6. Ctrl + C (client)  : Disconnects the client and       |\n";
    s += "|                           terminates its chat session      |\n";
    s += "|                           if present                       |\n";
    s += "|   7. Ctrl + C (server)  : Terminates the server and all the|\n";
    s += "|                           clients connected to the server  |\n";
    s += "+------------------------------------------------------------+\n";
    s += NRM;
    return s;
}

vector<string> parseTheString(string &s, char delimiter)
{
    vector<string> res;
    string curr;
    for (auto x : s)
    {
        if (x == delimiter)
        {
            res.push_back(curr);
            curr = "";
        }
        else
            curr += x;
    }
    res.push_back(curr);
    return res;
}

void exit_handler(int sig)
{
    cout << "\nShutting the server down.. \n and disconnecting the clients..\n";
    for (auto it : usernames)
    {
        comms comm(it.second);
        comm.sendMsg("close");
        comm.disconnect();
    }
    exit(0);
    return;
}

int main(int argc, char **argv)
{
    srand(time(NULL));
    generatePublicKeys();
    if (argc > 1)
    {
        err("Too many arguments.");
    }
    connector server(1, PORT);
    server.listenForClients();

    // Singal Handling:
    signal(SIGINT, exit_handler);

    cout << GRN << "Server is up and running" << NRM << endl;
    cout << GRN << "G: " << to_string(G) << endl;
    cout << GRN << "P: " << to_string(P) << endl;

    ofstream oF("./logs/publicKeys.txt");
    if (!oF)
    {
        cerr << "Failed to open the file!" << endl;
        return 1;
    }
    oF << to_string(G) << endl;
    oF << to_string(P) << endl;

    oF.close();

    while (true)
    {
        int client_socket = server.acceptNow();
        waiting_client_sockets.push(client_socket);
        pthread_t thread_id;
        pthread_create(&thread_id, NULL, handle_client, NULL);
    }
    return 0;
}
