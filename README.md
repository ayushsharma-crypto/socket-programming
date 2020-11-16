# Socket Programming in C

## By Ayush Sharma (2019101004)

## Overview
This work deals with basic socket programming context. This root directory
contains two main directory one for `SERVER` node and other for `CLIENT` node.
Overall working is: 
- The client will create/request a connection to the server
- After successfull connection client can send either `get <filenames>` or `exit` REQUEST
- `get <filenames>` will download files to directory `CLIENT/downloads` from `SERVER/downloadable_files` directory
- `exit` will close the connection
- Progress will be printed while downloading the file on both server and client terminals.
- Connection is basically made of type `TCP/IPV4`
- Error handling such as requesting missing files has been handled appropriately.

## Making Code Run
Follow the steps to run the code:
- Open 2 terminal windows
- Open `SERVER` directory in one and `CLIENT` in other.
- In `SERVER` directory run the following commands:
    `gcc server.c -o server` then `clear;./server`
    You will get output on terminal:
    `server setup complete: waiting for connections...`

- In `CLIENT` directory run the following commands:
    `gcc client.c -o client` then `clear;./client`
    You will get a red color prompt.

- Before running any command place some dummy files of your choice in directory `SERVER/downloadable_files`.
    I have already put some dummy files of size ranging from 10 bytes to 1.2 GB there. Check it out.

- Your Client is now connected to the Server and go download the available files...

## Libraries Used

- #include<stdio.h>
- #include<stdlib.h>
- #include<string.h>
- #include<sys/socket.h>
- #include<sys/stat.h>
- #include<netinet/in.h>
- #include<unistd.h>
- #include<arpa/inet.h> // only in client.c
- #include<fcntl.h>