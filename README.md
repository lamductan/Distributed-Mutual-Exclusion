# Distributed Mutual Exclusion Algorithms

This repos is my implementation of the [Ricart-Agrawala algorithm](https://dl.acm.org/doi/10.1145/358527.358537) and [Maekawa algorithm](https://dl.acm.org/doi/abs/10.1145/214438.214445) for distributed mutual exclusion.

## 1. Ricart-Agrawala Algorithm
This repos implements Ricart-Agrawala algorithm for distributed mutual exclusion, with [the optimization proposed by Roucairol and Carvalho](https://dl.acm.org/doi/10.1145/358024.940975), in a client-server model.

### Assumptions:
1. There are three servers in the system, numbered from zero to two.
2. There are five clients in the system, numbered from zero to four.
3. Assume that each file is replicated on all the servers, and all replicas of a file are consistent in the beginning. To host files, create separate directory for each server.
4. A client can perform a READ or WRITE operation on the files.
(a) For READ operation, one of the servers is chosen randomly by the client to read from it.
(b) For WRITE operation, the request should be sent to all of servers and all of the replicas of the target file should be updated in order to keep them consistent.
5. READ/WRITE on a file can be performed by only one client at a time. However, different clients are allowed
to concurrently perform a READ/WRITE on different files.
6. In order to ensure the mentioned conditions, you must implement Ricart-Agrawala algorithm for distributed mutual exclusion, with the optimization proposed by Roucairol and Carvalho, so that no READ/WRITE violation could occur. The operations on files can be seen as critical section executions.
7. The supported operations by servers are as follows:
(a) ENQUIRY: A request from a client for information about the list of hosted files.
(b) READ: A request to read last line from a given file.
(c) WRITE: A request to append a string to a given file. The servers must reply to the clients with appropriate messages after receiving each request.
8. Assume that the set of file does not change during the program’s execution. Also, assume that no server failure occurs during the execution of the program.
9. The client must be able to do the following:
(a) Gather information about the hosted files bu querying the servers and keep the metadata for future.
(b) Append a string hclient id, timestampi to a file f_i during WRITE operation. Timestamp is the value of the clients, local clock when the WRITE request is generated. This must be done to all replicas of f_i .
(c) Read last line of a file f_i during READ.
10. Write an application that periodically generates READ/WRITE requests for a randomly chosen file from the set of files stored by the servers.
11. Display appropriate log messages to the console or to a file. Keep in mind to close all open sockets when your program exits/terminates.

## 2. Maekawa Algorithm
Implement the same functionality as Ricart-Agrawala algorithm, with the same number of servers (3) and clients (5), except for the following differences:
### Assumptions:
1. The clients, numbered C_0 through C_4 execute Maekawa’s mutual exclusion algorithm among them (instead of the Ricart-Agrawala algorithm) to decide which client gets to enter the critical section to access a file.
2. Quorum size is equal to three.
3. Quorum of client C_i consists of the set {C_i , C_(i+1) mod 5 , C_(i+2) mod 5}.
4. Make sure to implement all messages for Maekawa’s algorithm, including ENQUIRE, YIELD and FAILED.

## 3. How the code works
### To compile:
```
1. mkdir build
2. cd build
3. cmake ..
4. make
```

### A one-line command to run the program: 
```
1. cd build
2. ./prog
```

### For running in ssh:
0. Add these 2 lines to the file `~/.ssh/config`
```
Host *
StrictHostKeyChecking no
```
1. Copy this folder containing this file to a directory in your UTD machine. Edit your netid in the first line of file `test_script.sh` and edit the `PREFIX` in the second line of the file as the directory of your choice.
2. Compile the project with 4 commands on the top of this file.
3. Edit the number of requests `$n_requests` in the third line of the file `test_script.sh`.
4. In the terminal of your local machine, run `bash test_script.sh`
