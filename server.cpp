#include <bits/stdc++.h>
#include "include/dh.hpp"
#include "include/rc4.hpp"
#include "include/mim.hpp"
#include "include/const.hpp"
#include "include/comms.hpp"
#include "include/connector.hpp"
#include "include/mim.hpp"
#include <unistd.h>

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

string serverDir;
queue<int> waiting_client_sockets;
mutex mtx;
unordered_map<string, int> usernames;
unordered_map<string, string> partner;
unordered_map<int, string> commonFile;
int intrusiveness;

// Function definations:
void sendStatus(string username);
void *handle_client(void *arg);
bool connectToClient(string username, vector<string> &parsedMsg, string &logFile);
void closeSession(string username);

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

    // dbg
    //  cout<<"now i will send accpet messg to client"<<endl;
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
        if (parsedMsg[0] == "!test")
            sendStatus(username);
        else if (parsedMsg[0] == "!connect")
        {
            logFile = serverDir + genRandName() + ".txt";
            connectToClient(username, parsedMsg, logFile);
        }
        else if (parsedMsg[0] == "!key")
        {
            string A = parsedMsg[1];
            cout << A << endl;
            ofstream of(commonFile[client_socket], ios::app);
            of << A << endl;
            of.close();
            comms comm1(usernames[partner[username]]);
            comm1.sendMsg("!keys");
            comm1.sendMsg(A);
        }
        else if (parsedMsg[0] == "!close")
        {
            closeSession(username);
            break;
        }
        else if (partner.find(username) != partner.end())
        {
            comms reciever(usernames[partner[username]]);
            if (parsedMsg[0] == "!file")
            {
                string fileName = comm.receive();
                ll sz = stoll(comm.receive());
                cout << YEL << "Transferring a File of size " << formatBytes(sz) << " (" << sz << " Bytes ) " << NRM << endl;
                reciever.sendMsg("!file");
                reciever.sendMsg(fileName);
                auto file = parseTheString(fileName, '/');
                reciever.sendMsg(to_string(sz));
                ofstream of(commonFile[client_socket], ios::app);
                of << "!file " << serverDir + "files/" + file.back() << endl;
                of.close();
                transfile(client_socket, usernames[partner[username]], sz, serverDir + "files/" + file.back());
                continue;
            }

            if (msg == "!goodbye")
            {
                cout << username << " said " << msg << endl;
                closeSession(username);
                continue;
            }
            string s;
            s += username + " $" + msg;
            cout << s << endl;
            ofstream of(commonFile[client_socket], ios::app);
            of << username + " $ ";
            for (int i = 0; i < msg.size(); i++)
            {
                if (i != msg.size() - 1)
                    of << charToHexString(msg[i]) << " ";
                else
                    of << charToHexString(msg[i]);
            }
            of << endl;
            of.close();
            reciever.sendMsg(s);
        }
        else
        {
            string s = RED;
            s += "Invalid Command. Please refer to the readme for commands.";
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
    statusMsg += "\n";

    for (auto it : usernames)
        names.push_back(it.first);
    for (int i = 0; i < names.size(); i++)
    {
        string status = partner.find(names[i]) == partner.end() ? string(GRN) + "FREE" + string(NRM) : string(RED) + "BUSY" + string(NRM);
        statusMsg += string(78, ' ') + string(BLU) + names[i] + string(NRM) + " " + status;
        if (i != names.size() - 1)
            statusMsg += "\n";
    }
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
        errMessage += "You are already in a chat. Please close the current chat to connect to another client.";
        errMessage += NRM;
        comm.sendMsg(errMessage);
        cout << CYN << username << RED << " connection rejected due to already existing connection." << NRM << endl;
        return false;
    }
    if (username == partnerName)
    {
        string errMessage = RED;
        errMessage += "To connect name can't be same as parent connection name";
        errMessage += NRM;
        comm.sendMsg(errMessage);
        cout << CYN << username << RED << " connection rejected as an user can't connect with itself" << NRM << endl;
        cout << RED << "Destination name can't be same as source Name" << NRM << endl;
        return false;
    }
    if (usernames.find(partnerName) == usernames.end())
    {
        string errMessage = RED;
        errMessage += "Username doesn't exist.";
        errMessage += NRM;
        comm.sendMsg(errMessage);
        cout << CYN << username << RED << " connection rejected as " << CYN << partnerName << RED << " is not a valid username." << NRM << endl;
        return false;
    }
    if (partner.find(partnerName) != partner.end())
    {
        string errMessage = RED;
        errMessage += "" + string(CYN) + partnerName + string(NRM) + " is BUSY. Please try after someone, or chat with someone else.";
        errMessage += NRM;
        comm.sendMsg(errMessage);
        cout << CYN << username << RED << " connection rejected as " << CYN << partnerName << RED << " is busy." << NRM << endl;
        return false;
    }
    partner[username] = partnerName;
    partner[partnerName] = username;

    commonFile[usernames[username]] = commonFile[usernames[partnerName]] = logFile;
    ofstream of(logFile, ios::app);
    of << to_string(G) << endl;
    of << to_string(P) << endl;
    of.close();

    cout << GRN << "Connected: " + string(BLU) + username + string(NRM) + " and " << BLU << partnerName << NRM << endl;

    // Notify the clients about their successful connection:
    string msg = GRN;
    msg += "You are now connected to ";
    comm.sendMsg(msg + partnerName + NRM);
    comms comm1(usernames[partnerName]);
    comm1.sendMsg(msg + username + NRM);

    // Exchange keys
    string handshake = "!handshake";
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
        errMessage += "You are not connected to anyone.";
        errMessage += NRM;
        comm.sendMsg(errMessage);
        cout << RED << "Need to connect to someone to close the connection." << NRM << endl;
        return;
    }

    string partnerName = partner[username];
    string msg = GRN;
    msg += username + " closed the chat session" + NRM;
    comms comm1(usernames[partnerName]);
    comm1.sendMsg(msg);
    cout << msg << endl;
    if (partner.find(partnerName) != partner.end())
        partner.erase(partnerName);
    if (partner.find(username) != partner.end())
        partner.erase(username);
    msg = "";
    msg += CYN;
    msg += "Session closed";
    msg += NRM;
    comms comm(usernames[username]);
    comm.sendMsg(msg);
    sleep(0.1);
    string message = "!goodbye";
    comm.sendMsg(message);
    comm1.sendMsg(message);
    cout << "Sent goodbyes to both\n";
    if (intrusiveness)
    {
        cout << "Server Operating in intrusive mode" << endl;
        fix(commonFile[usernames[username]]);
    }
    if (commonFile.find(usernames[username]) != commonFile.end())
        commonFile.erase(usernames[username]);
    if (commonFile.find(usernames[partnerName]) != commonFile.end())
        commonFile.erase(usernames[partnerName]);

    cout << "Disconnected " + string(CYN) + username + string(NRM) + " and " + string(CYN) + partnerName << NRM << endl;
}

void exit_handler(int sig)
{
    cout << "\nShutting the server down.. \n and disconnecting the clients..\n";
    for (auto it : usernames)
    {
        comms comm(it.second);
        comm.sendMsg("!close");
        comm.disconnect();
    }
    exit(0);
    return;
}

int main(int argc, char **argv)
{
    srand(time(NULL));
    generatePublicKeys();
    if (argc < 2 or argc > 2)
    {
        printf("Too many(few) arguments. Expected ./server <intrusiveness>\n");
        exit(1);
    }
    intrusiveness = stoi(argv[1]);
    connector server(1, PORT);
    server.listenForClients();

    // Singal Handling:
    signal(SIGINT, exit_handler);

    cout << GRN << "Server is up and running" << NRM << endl;
    cout << GRN << "G: " << to_string(G) << endl;
    cout << GRN << "P: " << to_string(P) << endl;
    serverDir = "./logs/";
    auto now = chrono::system_clock::now();
    auto timePoint = chrono::system_clock::to_time_t(now);
    char buffer[80];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", localtime(&timePoint));
    string currentTime(buffer);
    serverDir += currentTime + "/";
    filesystem::create_directory(serverDir);
    filesystem::create_directory(serverDir + "files/");
    while (true)
    {
        int client_socket = server.acceptNow();
        waiting_client_sockets.push(client_socket);
        pthread_t thread_id;
        pthread_create(&thread_id, NULL, handle_client, NULL);
    }
    return 0;
}
