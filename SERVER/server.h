#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/socket.h>
#include<sys/stat.h>
#include<netinet/in.h>
#include<unistd.h>
#include<fcntl.h>
#define BACKLOG 1
#define PORT 4000
#define MAX_FILES 20
#define input_size 500
#define bytes_per_read 10000

struct sockaddr_in server_address;
socklen_t sin_size = sizeof(server_address);

int server_setup(); // Setting up the server and return -1 or sock_fd

char** all_file_name(char* raw_data,int* Total_files);  // Extracting Filenames and returns array of pointers pointing to filenames and initiate total_files

int downloadfile(int new_fd,char** token_ptr_array,int Total_file); // For downloading valid file and returns number of valid files

int download_now(int new_fd,int fd);    // For downloading the given file. Return -1 on failure and 0 on succes.

