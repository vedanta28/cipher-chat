# cipher-chat
Multithreaded Chat Server with Encryption

## Server
The server can be compiled by the command ```g++ server.cpp -o server```.
The server can be run in 2 modes 0(safe mode) or 1(intrusive mode).
In safe mode the server does not use mim attack on the messages and files intercepted, whereas it does do so in intrusive mode

The usage for safe mode is ```./server 0```
The usage for intrusive mode is ```./server 1```

In intrusive mode, as soon as the session terminates between two clients, the server finds out the messages and writes them in a file ```<RANDOMNUMBER>_decrypted.txt``` and
both the encrypted and decrypted files are present in the files folder. Every time the server starts, it makes a new folder with the name being the timestamp of the event of server starting.
All these files can be found inside logs/


The usage of client is similar : ```g++ client.cpp -o client``` 
The client is run using ```./client``` and the first input is the ip of the server use localhost for server on the same machine.
