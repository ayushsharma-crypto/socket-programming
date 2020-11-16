#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/socket.h>
#include<sys/stat.h>
#include<netinet/in.h>
#include<unistd.h>
#include<arpa/inet.h>
#include<fcntl.h>

#define PORT 4000
#define input_size 500
#define bytes_per_read 10000

int receive_files(int client_sock_fd, char * filename);
int check_cmd(char *INPUT);


int main(int argc, char** argv)
{
    // creating socket descriptor for client [ IPV4 ]
    int client_sock_fd = socket(PF_INET, SOCK_STREAM, 0);
    if(client_sock_fd < 0) 
    {
        perror("Socket Creation");
        return -1;    
    }

    // Managing server's address
    struct sockaddr_in server_address;
    server_address.sin_port = htons(PORT);
    server_address.sin_family = AF_INET;
    socklen_t sin_size = sizeof(server_address);

    // converting IP address from presentation/printable to network format
    int addr = inet_pton(AF_INET, "127.0.0.1", &server_address.sin_addr);
    if(addr < 0)
    {
        perror("Invalid server address");
        return -1;
    }

    // Connecting to the server described by `struct sockaddr_in` instance `server_address`
    if( connect(client_sock_fd, (struct sockaddr *)&server_address, sin_size ) < 0 )
    {
        perror("Connection Problem");
        return -1;
    }

    while(1)
    {
        // Printing prompt
        printf("\x1B[1;31mCLIENT>\x1B[1;0m");

        // Assuming no space in filenames
        size_t input_sz = input_size;
        char *INPUT = (char*)malloc(input_size);
        if(!INPUT)
        {
            printf("Memory Allocation error\n");
            return -1;
        }
        getline(&INPUT,&input_sz,stdin);
        INPUT[strlen(INPUT)-1]='\0';

        // Cross-checking cmd inputted
        if(strlen(INPUT)==0) continue;
        if(!check_cmd(INPUT))
        {
            printf("Unknown Request! Support only `exit` or `get <filename>`\n");
            continue;
        }

        // Sending command entered
        int snt;
        if( (snt=send(client_sock_fd,INPUT,strlen(INPUT),0) )< 0 )
        {
            perror("Sending Filename");
            return -1;
        }
        // printf("SNT=%d\n",snt);

        // Closing connection if REQUEST was `exit`
        if(strcmp(INPUT,"exit")==0)
        {
            char buff[50];
            if(read(client_sock_fd,buff,50)<0)
            {
                perror("Read Message");
                return -1;
            }
            printf("\x1B[1;30m%s\x1B[1;0m\n",buff);
            break;
        }
        
        // Accepting Message for checking net `Total_files`
        char net_file[60];
        if( read(client_sock_fd,net_file,60) < 0 )
        {
            perror("Total file Message");
            return -1;
        }
        printf("%s\n\n",net_file);
    
        int st=19; char num[20];
        while(net_file[st]!=' ') 
        {
            num[st-19]=net_file[st];
            st++;
        }
        int total_file=atoi(num);

        // Accepting Response for each valid file
        for(int i=0;i<total_file;i++)
        {
            // printf("\ni=%d\n",i);
            char buff[100];
            int rd=read(client_sock_fd,buff,100);
            if(rd<=0)
            {
                perror("Problem in Read Response");
                return -1;
            }
            printf("[ %d/%d ] %s\n",i+1,total_file,buff);

            // Sending ACK for Response
            char buffack[3];buffack[0]='o';buffack[1]='k';buffack[2]='\0';
            if(send(client_sock_fd,buffack,sizeof(buffack),0)<0)
            {
                perror("Sending ACK");
                return -1;
            }
            
            // Receiving/Downloading file if possible
            if(buff[0]=='D')
            {
                // printf("\tDOWNLOAD BEGINS ");

                // Assuming downloads folder exists
                if(-1 == receive_files(client_sock_fd, buff+12))
                {
                    printf("Problem at receiving files.\n");
                    return -1;
                }
                

                // Sending ACK for Download Response
                if(send(client_sock_fd,buffack,sizeof(buffack),0)<0)
                {
                    perror("Sending DACK");
                    return -1;
                }
                printf("\nDownload Complete\n\n");
            }
        }
    }

    return 0;
}

int check_cmd(char *INPUT)
{
    if( (INPUT[0]=='e') && (INPUT[1]=='x') && (INPUT[2]=='i') && (INPUT[3]=='t') && (strlen(INPUT)==4) )
        return 1;
    if( (INPUT[0]=='g') && (INPUT[1]=='e') && (INPUT[2]=='t') && (INPUT[3]==' ') && (strlen(INPUT)>4) )
        return 2;
    else
        return 0;
    
}

int receive_files(int client_sock_fd, char* filename)
{

    // getting file descriptor for downloaded file
    char d_filepath[60];
    sprintf(d_filepath,"./downloads/%s",filename);
    int d_fd = open(d_filepath,O_WRONLY|O_CREAT, 0700);

    char buffer[bytes_per_read+2];    //Array for storing the read bytes from server
    char complete[100];
    while(1)
    {
        // read and write downloads
        memset(buffer,0,sizeof(buffer));
        int k=read(client_sock_fd,buffer,sizeof(buffer));
        if(k<0)
        {
            perror("reading Download");
            return -1;
        }
        buffer[strlen(buffer)]='\0';
        // printf("\nk=%d\n",k);

        // write to file locally
        if(write(d_fd,buffer,strlen(buffer))<0)
        {
            perror("Writing Local file");
            return -1;
        }
        
        // send ack
        char ack[4];
        ack[0]='o';ack[1]='k';ack[2]='\0';
        if(send(client_sock_fd,ack,4,0)<0)
        {
            perror("Sending ACK");
            return -1;
        }

        // receive %completed
        memset(complete,0,sizeof(complete));
        if(read(client_sock_fd,complete,100)<0)
        {
            perror("Reading Percentage completed");
            return -1;
        }
        // printf("%s",complete);

        // send ack
        if(send(client_sock_fd,ack,4,0)<0)
        {
            perror("Sending ACK");
            return -1;
        }

        // break if done
        printf("%s",&complete[0]);
        if(complete[32]=='1' && complete[33]=='0' && complete[34]=='0' && complete[35]=='.' && complete[36]=='0' && complete[37]=='0' && complete[38]=='0') break;
    }


    return 0;
}