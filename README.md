Task 2: GetCopyright and plugins to server

Simulating DLL export using sockets

Server has map guid:plugin and updates it each time when registry is changed. 
Client sends guid to server and gets according DLL function. 
Supplier writes new guid:plugin key-value pair to registry.

Launch tutorial:

1. Build solution
2. Launch server.exe , then client.exe with parameters "127.0.0.1 {00000000-0000-0000-0100-000000000000}" You will see that Plugin 1_1 was sent
3. Launch supplier.exe, then client.exe with same parameters as in step 2. You will see that Plugin 1_2 was sent now