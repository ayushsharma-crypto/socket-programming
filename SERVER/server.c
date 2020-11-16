#include "server.h"

int server_setup()
{
    // creating socket descriptor for server [ IPV4 ]
    int sock_fd = socket(PF_INET, SOCK_STREAM, 0);
    if(sock_fd<0) return -1;

    // setting options for sockfd
    int yes=1;
    if(setsockopt(sock_fd,SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &yes,sizeof(int)) < 0) return -1;

    // binding to port PORT
    server_address.sin_port = htons(PORT);
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_family = AF_INET;

    if( bind(sock_fd,(struct sockaddr*)&server_address, sin_size) < 0 ) return -1;


    // Server started Listen from port PORT
    if( listen(sock_fd,BACKLOG)< 0 ) return -1;

    return sock_fd;
}

char** all_file_name(char* raw_data,int *Total_files)
{
    char** token_ptr_array = malloc(sizeof(char*)*(MAX_FILES+4));
    if(!token_ptr_array) return NULL;
    
    int i=0;
    (*Total_files)=0;
    token_ptr_array[0]=strtok(raw_data," ");

    while ( ((i+1)<=MAX_FILES) && (token_ptr_array[i] != NULL) )
    {
        token_ptr_array[++i]=strtok(NULL," ");
        // printf("fi=%d and token=%s\n",i,token_ptr_array[i]);
    }

    if(i==MAX_FILES && token_ptr_array[MAX_FILES]!=NULL) 
    {
        *Total_files=MAX_FILES;
        token_ptr_array[++i]=NULL;
    }
    else *Total_files=i-1;
    
    return token_ptr_array;
}

int downloadfile(int new_fd,char** token_ptr_array,int Total_file)
{
    int valid_file=0;
    for(int i=1;i<=Total_file;i++)
    {
        char filename[60];
        sprintf(filename,"./downloadable_files/%s",token_ptr_array[i]);

        int fd = open(filename,O_RDONLY);
        char buffer[60];
        if( fd < 0)
        {
            sprintf(buffer,"Unknown File: %s",token_ptr_array[i]);
            printf("%s\n",buffer);
        }
        else
        {
            valid_file++;
            sprintf(buffer,"Downloading %s",token_ptr_array[i]);
            printf("Sending File: %s\n",buffer+12);
        }

        buffer[strlen(buffer)]='\0';
        int sent_data=send(new_fd,buffer,sizeof(buffer),0);
        // printf("sent_data=%d\n",sent_data);
        
        // Getting Acknowledgement
        char ACK[3];
        if(read(new_fd,ACK,3)<0)
        {
            perror("Getting ACK");
            return -1;
        }
        printf("C_ACK:%s\n",ACK);

        // Download file if possible i.e fd>0
        if(fd > 0)
        {
            if(download_now(new_fd,fd)==-1)
            {
                printf("Problem In Sending File!\n");
                return -1;
            }

            // Getting Acknowledgement for Downloading
            char ACKD[3];
            if(read(new_fd,ACKD,3)<0)
            {
                perror("Getting ACKD");
                return -1;
            }
            printf("\nDC_ACK:%s\n",ACKD);
        }

        close(fd);

    }

    return valid_file;
}

int download_now(int new_fd,int fd)
{
    char buffer[bytes_per_read+2];    //Array for storing the read bytes from file
    long long int file_size = lseek(fd,0,SEEK_END);   // Finding file size
    lseek(fd,0,SEEK_SET);   // placing pointer to read from start of the file.

    char complete[100];
    long long int read_till_now=0;
    while(1)
    {
        memset(buffer,0,sizeof(buffer));
        // read from file
        int read_temp=read(fd, buffer, bytes_per_read);
        if(read_temp<0)
        {
            perror("Reading File");
            return -1;
        }
        buffer[read_temp]='\0';
        read_till_now+=read_temp;


        // send to client
        int sd;
        if((sd=send(new_fd,buffer,strlen(buffer),0))<0)
        {
            perror("Sending File");
            return -1;
        }
        // printf("\nsd=%d\n",sd);

        // read ack
        char ack[4];
        memset(ack,0,sizeof(ack));
        if(read(new_fd,ack,sizeof(ack))<0)
        {
            perror("Read Ack");
            return -1;
        }

        // send %completed
        memset(complete,0,sizeof(complete));
        sprintf(complete,"Percentage of the file written  %0.4f %% .....\r",(float)(100*read_till_now)/file_size);
        complete[strlen(complete)]='\0';
        if(send(new_fd,complete,strlen(complete),0)<0)
        {
            perror("Sending percentage donloaded");
            return -1;
        }
        printf("%s",complete);

        // read ack
        memset(ack,0,sizeof(ack));
        if(read(new_fd,ack,sizeof(ack))<0)
        {
            perror("Read Ack");
            return -1;
        }
        // printf("Final=%s",ack);

        // break if done
        if(read_till_now==file_size) break;
    }

    return 0;

}

int main()
{
    int server_sock_fd = server_setup();
    if(server_sock_fd<0)
    {
        perror("Server Setup");
        return -1;
    }

    printf("server setup complete: waiting for connections...\n\n");

    // Loop for checking/ accepting client connection.
    while(1)
    {
        // Accepting the connection from the client.
        int new_fd = accept(server_sock_fd,(struct sockaddr*)&server_address,&sin_size);
        if(new_fd < 0)  
        {
            perror("Accept Server");
            continue;
        }

        printf("client connected...\n");

        // Loop for client request execution
        int not_exit=1;
        while(not_exit)
        {
            char *INPUT = (char*)malloc(input_size);
            if(!INPUT)
            {
                printf("Memory Allocation Error\n");
                return -1;
            }

            // Getting command entered from client
            int szr;
            if( (szr=read(new_fd,INPUT,input_size)) <= 0 )
            {
                if(szr==0) printf("\x1B[1;30mUnusual Client Termination\x1B[1;0m\n\n");
                else perror("Read Input");
                not_exit=0;
                continue;
            }

            printf("REQUEST:%s\n",INPUT);

            // Closing connection REQUEST was `exit`
            if(strcmp(INPUT,"exit")==0)
            {
                printf("\x1B[1;30mclient closed the connection\x1B[1;0m\n\n");
                not_exit=0;
                if(send(new_fd,"Closing Connection",19,0)<0)
                {
                    perror("Send Close Connection");
                }
                continue;
            }

            // Extracting Filenames from the REQUEST
            int Total_files=0;
            char** token_ptr_array = all_file_name(INPUT,&Total_files);
            if(token_ptr_array==NULL)
            {
                printf("Memory Allocation error\n");
                close(new_fd);
                return -1;
            }

            // printf("Total Files=%d\n",Total_files);
            // for(int i=0;i<=Total_files;i++)
            //     printf("i=%d %s\n",i,token_ptr_array[i]);

            // Sending Message for checking net `Total_files`
            char net_file[60];
            sprintf(net_file,"Checking for first %d files...",Total_files);
            if(send(new_fd,net_file,sizeof(net_file),0)<0)
            {
                perror("Total File Message");
                return -1;
            }

            int dwnld=downloadfile(new_fd,token_ptr_array,Total_files);
            if(!dwnld)
            {
                printf("\nNo valid Files\n\n");
                continue;
            }
            if(dwnld==-1)
            {
                perror("Download Time Problem");
                return -1;
            }
            printf("\nAll valid downloads done!\n\n");
        }

        // Closing the connection
        close(new_fd);
    }

    return 0;
}