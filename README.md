# What I have done?

- Implementing a model of a social matching service with several clients, a central server and three distributed back-end servers. 
- Send a request from one of the client to the main server and relay it from the main server to an appropriate back-end server.
- 

# What your code files are and what each one of them does?

## 1. central.c:
-  Connected to clientA and clientB through TCP. Receives inputs usernames from clientA and B. Send processed data back to clientA and clientB
- Connected to serverT,serverS and serverP through UDP. Send related data to them and get the final data from serverP.
## 2. serverT.c
- Read data from edgelist.txt
- Connected to central server via UDP.
- Send the social network graph to central server according to the inputs from central server.
## 3. serverS.c
- Read data from scores.txt
- Connected to central server via UDP.
- Send the compatibility to central server according to the inputs from central server.
## 4. serverP.c
- Connected to central server via UDP and receive related data from central server 
- Use related information to find a social network path that has the smallest matching gap with Dijkstra algorithm and send back to central server.
## 5. clientA.c
- Connected to central via TCP and send the hostname to it.
## 6. clientB.c
- Connected to central via TCP and send the hostname to it.
## makefile
- Complie all the .c files

# The format of all the messages exchanged:

## Central Server:
The Central server is up and running

The Central server received input = "Victor" from client using TCP over port 25913

The Central server received input = "Oliver" from client using TCP over port 25913

The Central server sent a request to Backend-Server S

The Central server received information from Backend-ServerT using UDP over 24913

The Central server sent a request to Backend-Server T

The Central server received information from Backend-ServerS using UDP over 24913

The Central server sent a request to Backend-Server P

The Central server received results from Backend-Server P

## ServerT:
The ServerT is up and running using UDP on port 21913

The ServerT received a request from Central to get the topology

The ServerT finished sending the topology to the Central

## ServerS:
The ServerS is up and running using UDP on port 22913

The ServerS received a request from Central to get the scores

The ServerS finished sending the scores to the Central

## ServerP:
The ServerP is up and running using UDP on port 23913

The ServerP received a request from Central to get the topology

The ServerP received the topology and score infomation

## Client A:
The client is up and running

The client sent Victor to the Central Server

Found compatibility for Victor and Oliver

Victor---Rachael---Oliver

Compability score: 1.06

## Client B:
The client is up and running

The client sent Oliver to the Central Server

Found compatibility for Oliver and Victor

Oliver---Rachael---Victor

Compability score: 1.06


# Idiosyncrasy
There is a maximum buffer which is 512. If a single data exceeds this size, the program may crash

# Resued code
1. Referrence book: Beej's Guide to Network Programming.
2. The code for Dijkstra Algorithm is based on code from csdn.net