/**
* Based on code found at https://github.com/mafintosh/echo-servers.c (Copyright (c) 2014 Mathias Buus)
* Copyright 2019 Nicholas Pritchard, Ryan Bunney
**/

/**
 * @brief A simple example of a network server written in C implementing a basic echo
 *
 * This is a good starting point for observing C-based network code but is by no means complete.
 * We encourage you to use this as a starting point for your project if you're not sure where to start.
 * Food for thought:
 *   - Can we wrap the action of sending ALL of out data and receiving ALL of the data?
 */
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#define BUFFER_SIZE 1024

typedef struct
{
    int client_fd;
    int num_lives;
    char *client_id;
} Client;

int dice1, dice2;

bool validate_message(char * message, char *client_id)
{
    if (strlen(message) > 14) {
        printf("The packet is too long, causing it to be invalid\n");
        return false;
    }
    if(strstr(message,"INIT") != NULL && !strcmp(message, "INIT")) return true;
    else if(strstr(message,"MOV") != NULL)
    {
        char packet[14];
        if (strstr(message,"EVEN") != NULL) {
            sprintf(packet,"%s,MOV,EVEN", client_id);
            if(strcmp(message, packet) != 0) {
                printf("Invalid EVEN move packet\n");
                return false;
            }
        }
        if (strstr(message,"ODD") != NULL) {
            sprintf(packet,"%s,MOV,ODD", client_id);
            if(strcmp(message, packet) != 0) {
                printf("Invalid ODD move packet\n");
                return false;
            }
        }

        if (strstr(message,"DOUB") != NULL) {
            sprintf(packet,"%s,MOV,DOUB", client_id);
            if(strcmp(message, packet) != 0) {
                printf("Invalid DOUB move packet\n");
                return false;
            }
        }
        if (strstr(message,"CON") != NULL) {
            sprintf(packet,"%s,MOV,CON,", client_id);
            printf("%s\n",client_id);
            printf("%s\n",message);
            printf("%s\n",packet);
            if(strstr(message, packet) == NULL) {
                printf("Invalid CON move packet\n");
                return false;
            }
        }
        printf("It's an old message, but it checks out\n");
        return true;
    }
    printf("Not a proper INIT or MOV packet, causing to be invalid\n");
    return false;
}

/*
*Kicks player out by removing them from pointer containing connected clients
*Will not kick players out if draw
*@param num_clients: Number of clients still connected
*@param connected_clients: Pointer containing list of connected clients
*@return void
*/
void kick_player(int *num_clients,Client *connected_clients)
{
    int num_people_kicked = 0;
    for(int i = 0; i < num_clients; i++)
    {
        if (connected_clients[i]->num_lives == 0)
        {
            num_people_kicked++;
        }
    }
    if(num_people_kicked != num_clients) //Not a draw
    {
        Client *surviving_players;
        int index = 0;
        if(surviving_players == NULL)
        {
            printf("Cannot allocate %i bytes of memory\n",bytes_required);
            exit(EXIT_FAILURE);
        }
        int bytes_required = (num_clients - num_people_kicked) * sizeof(Client);
        for(int j = 0; j < num_clients; j++)
        {
            if (connected_clients[i]->num_lives == 0) //Client dead so kick them
            {
                strcpy(result_message,client->client_id);
                strcat(result_message,",ELIM");
                send_message(result_message, client->client_fd);
            }
            else
            {
                surviving_players[index] = connected_clients[i]
                index++
            }
        }
        *(num_clients) = *num_clients - num_people_kicked;
        *(connected_clients) = surviving_players;
    }
    else
    {
        *(num_clients) = 0;
    }
}

/*
*Sends client a message.
*length of message to be received by client.
*@param message: message to obe sent to the client
*@param width: Length of the final String
*@return void
*/
void send_message(char *message, int destination_fd)
{
    int err = send(destination_fd,message,strlen(message),0);
    if (err < 0){
        fprintf(stderr,"Client write failed\n");
        exit(EXIT_FAILURE);
    }
    sleep(1);
}

char* receive_message(int sender_fd)
{
    char *buf = calloc(BUFFER_SIZE, sizeof(char)); // Clear our buffer so we don't accidentally send/print garbage
    if (buf == NULL)
    {
        printf("Cannot allocate %i bytes of memory\n",BUFFER_SIZE);
        exit(EXIT_FAILURE);
    }
    int read = recv(sender_fd, buf, BUFFER_SIZE, 0);    // Try to read from the incoming client
    if (read < 0){
        fprintf(stderr,"Client read failed\n");
        exit(EXIT_FAILURE);
    }
    buf[read] = '\0';
    return buf;
}

// Function called by child to interpret message
int parse_message(char *message, Client client )
{
    int enum_value = 10;		// Default, empty value
    if(validate_message(message,client.client_id))
    {
        if(strstr(message,"INIT") != NULL)
        {
            int bytes_required = strlen("WELCOME,") + strlen(client.client_id) + 1;
            char *welcome_message = calloc(bytes_required, sizeof(char));
            if(welcome_message != NULL)
            {
                strcpy(welcome_message,"WELCOME,");
                strcat(welcome_message,client.client_id);
                send_message(welcome_message, client.client_fd);
                printf("Sent welcome message!\n");
            }
            else
            {
                printf("Cannot allocate %i bytes of memory\n",bytes_required);
                exit(EXIT_FAILURE);
            }
        }

        else if(strstr(message,"MOV") != NULL)
        {
            if (strstr(message,"EVEN") != NULL)
            {
                enum_value = 8;
            }

            if (strstr(message,"ODD") != NULL)
            {
                enum_value = 0;
            }

            if (strstr(message,"DOUB") != NULL)
            {
                enum_value = 7;
            }

            if (strstr(message,"CON") != NULL)
            {
                if (strstr(message,"CON,1") != NULL) enum_value = 1;
                if (strstr(message,"CON,2") != NULL) enum_value = 2;
                if (strstr(message,"CON,3") != NULL) enum_value = 3;
                if (strstr(message,"CON,4") != NULL) enum_value = 4;
                if (strstr(message,"CON,5") != NULL) enum_value = 5;
                if (strstr(message,"CON,6")!= NULL) enum_value = 6;
            }
        }
    }

    else
    {
        send_message("You have been caught cheating\n"
                     "Kicking you out\n",
                       client.client_fd);
        exit(EXIT_FAILURE);
    }
    return enum_value;
}

void calculate (int dice1, int dice2, int enum_value, Client* client)
{
    // enumvalues - 0 = Odd, 1-6 = Choice of Dice, 7 = Doubles, 8 = Even
    bool failed_round = false;

    int bytes_required = strlen(client->client_id) + strlen(",XXXX") + 1;
    char *result_message = calloc(bytes_required, sizeof(char));

    if(result_message != NULL) strcpy(result_message,client->client_id);

    else
    {
        printf("Cannot allocate %i bytes of memory\n",bytes_required);
        exit(EXIT_FAILURE);
    }

    if (enum_value < 0 || enum_value > 8)
    {
        fprintf(stderr,"Invalid enum value");
        return;
    }

    else if (enum_value == 0)    // Odd
    {    // Fails if even or dice-sum is not greater than 5
        if ((dice1 + dice2)%2 == 0 || dice1 + dice2 <= 5) failed_round = true;
        printf("Player chose Odd\n");
    }

    else if (enum_value == 7)    // Doubles
    {
        if (dice1 != dice2) failed_round = true;
        printf("Player chose Doubles\n");
    }

    else if (enum_value == 8)    // Even
    {    // Fails if odd or doubles
        if ((dice1 + dice2)%2 == 1 || dice1 == dice2) failed_round = true;
        printf("Player chose Even\n");
    }

    else
    {
        if (dice1 != enum_value && dice2 != enum_value) failed_round = true;
        printf("Player chose %d\n", enum_value);
    }

    if (failed_round)
    {
        strcat(result_message,",FAIL");
        send_message(result_message, client->client_fd);

        client->num_lives--;
        printf("Player has this failed round, lives left: %d\n", client->num_lives);
        if (client->num_lives == 0)
        {
            strcpy(result_message,client->client_id);
            strcat(result_message,",ELIM");
            send_message(result_message, client->client_fd);
        }
    }
    else {
        strcat(result_message,",PASS");
        send_message(result_message, client->client_fd);
        printf("Player has survived... this round\n");
    }
}

int main (int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr,"Usage: %s [port], [number of lives]\n",argv[0]);
        exit(EXIT_FAILURE);
    }
    srand ( time(NULL) ); //set seed
    int port = atoi(argv[1]);
    int server_fd, client_fd, err, opt_val;
    int num_lives = atoi(argv[2]);
    char client_id[4];
    int num_clients = 0;
    Client *connected_clients;
    struct sockaddr_in server, client;
    char *buf;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (server_fd < 0){
        fprintf(stderr,"Could not create socket\n");
        exit(EXIT_FAILURE);
    }

    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = htonl(INADDR_ANY);

    opt_val = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt_val, sizeof opt_val);

    err = bind(server_fd, (struct sockaddr *) &server, sizeof(server));
    if (err < 0){
        fprintf(stderr,"Could not bind socket\n");
        exit(EXIT_FAILURE);
    }

    err = listen(server_fd, 128);
    if (err < 0){
        fprintf(stderr,"Could not listen on socket\n");
        exit(EXIT_FAILURE);
    }

    printf("Server is listening on %d\n", port);

    while (true) {
        socklen_t client_len = sizeof(client);
        // Will block until a connection is made
        client_fd = accept(server_fd, (struct sockaddr *) &client, &client_len);
        if (client_fd < 0) {
            fprintf(stderr,"Could not establish connection with new client\n");
            continue;
        }
        sprintf(client_id, "%d", rand() % 900 + 100); //Generate a 3 digit id number
// Might create same ID
        Client client = {client_fd,num_lives,client_id};
        num_clients++;
        //Store a connected reference to the client
        if(connected_clients == NULL)
        {
            connected_clients = calloc(1,sizeof(Client));
            connected_clients[num_clients-1] = client;
        }
        else
        {
            connected_clients = realloc(connected_clients,num_clients * sizeof(Client));
            connected_clients[num_clients-1] = client;
        }
        if(connected_clients == NULL)
        {
            printf("Cannot allocate %lu bytes of memory\n",num_clients * sizeof(Client));
            exit(EXIT_FAILURE);
        }
        while (true) {
            char *response = receive_message(client_fd);
            parse_message(response, client);
            // char welcome_message[BUFFER_SIZE];
            // sprintf(welcome_message,"WELCOME,%d",client.client_id);
            // send_message(welcome_message,client.client_fd);
            char start_message[BUFFER_SIZE];
            sprintf(start_message,"START,%i,%i",1,num_lives);
            printf("%s\n",start_message);
            fflush(stdout);
            send_message(start_message,client.client_fd);

            while (client.num_lives > 0) {	// Play the game
                response = receive_message(client_fd);
                while (strstr(response, "MOV") == NULL) {	// Wait for move
                    response = receive_message(client_fd);
                }
                dice1 = ((rand() % 6) + 1) ;
                dice2 = ((rand() % 6) + 1) ;
                int choice = parse_message(response, client);
                calculate (dice1, dice2, choice, &client);
                printf ("Dice were: %d and %d\n", dice1, dice2);
            }
            exit(0);
        }
    }
}
