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
    bool has_played; //Bool expression to indicate that the
    int move[BUFFER_SIZE]; //to store move made by client
    int toParentMovePipe[2];		// Pipe child uses to write the move to parent
    int fromParentPipe[2];		// Pipe child uses to read from parent
} Client;

int dice1, dice2;
int i;
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

void send_victory (Client client)
{
    int bytes_required = strlen("VICT") + 1;
    char *victory_message = calloc(bytes_required, sizeof(char));
    if(victory_message != NULL)
    {
        strcpy(victory_message,"VICT");
        send_message(victory_message, client.client_fd);
    }
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
        write(client->fromParentPipe[1],result_message,strlen(result_message));
        client->num_lives--;
        printf("Player has this failed round, lives left: %d\n", client->num_lives);
    }
    else {
        strcat(result_message,",PASS");
        write(client->fromParentPipe[1],result_message,strlen(result_message));
        printf("Player has survived... this round\n");
    }
}

/*
*Kicks player out by removing them from pointer containing connected clients
*Will not kick players out if draw
*@param num_clients: Number of clients still connected
*@param connected_clients: Pointer containing list of connected clients
*@return void
*/
int kick_player(int num_clients,Client *connected_clients)
{
    printf("%s kkkakfafka\n",connected_clients[0].client_id);
    int num_people_kicked = 0;
    for(int i = 0; i < num_clients; i++)
    {
        if (connected_clients[i].num_lives == 0)
        {
            num_people_kicked++;
            break;
        }
    }
    if(num_people_kicked != num_clients) //Not a draw
    {
        Client *surviving_players = calloc(num_clients-num_people_kicked,sizeof(Client));
        int index = 0;
        if(surviving_players == NULL)
        {
            printf("Unable to allocate memory\n");
            exit(EXIT_FAILURE);
        }
        for(int j = 0; j < num_clients; j++)
        {
            char *result_message;
            if (connected_clients[i].num_lives == 0) //Client dead so kick them
            {
                result_message = malloc(strlen(",ELIM")+2);
                printf("%s\n",connected_clients[i].client_id);
                strcpy(result_message,connected_clients[i].client_id);
                strcat(result_message,",ELIM");
                send_message(result_message, connected_clients[i].client_fd);
                free(result_message);
                close(connected_clients[i].client_fd);
            }
            else
            {
                surviving_players[index] = connected_clients[i];
                index++;
            }
        }
        num_clients = num_clients - num_people_kicked;
        connected_clients = surviving_players;
        return num_clients;
    }
    else
    {
        return 0;
    }
}

int main (int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr,"Usage: %s [port], [number of lives]\n",argv[0]);
        exit(EXIT_FAILURE);
    }
    srand ( time(NULL) ); //set seed
    int server_fd, client_fd, err, opt_val;
    int port = atoi(argv[1]);
    int num_lives = atoi(argv[2]);
    struct sockaddr_in server, client;
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

    int id_iterator = 1;
    int num_clients = 0;
    Client *connected_clients;
    while (true) {
// *************** Add timer/max players
        socklen_t client_len = sizeof(client);
        // Will block until a connection is made
        client_fd = accept(server_fd, (struct sockaddr *) &client, &client_len);
        if (client_fd < 0) {
            fprintf(stderr,"Could not establish connection with new client\n");
            continue;
        }
        char client_id[10];
        sprintf(client_id, "%d", id_iterator); 			// Set client ids
        id_iterator = (id_iterator + 1) % 10;
        Client client = {client_fd,num_lives,client_id,false};
        pipe(client.toParentMovePipe);
        pipe(client.fromParentPipe);
        num_clients++;
        //Store a connected reference to the client
        if(connected_clients)
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
    	for(i = 0; i < num_clients; i++) 		// Create a child for each client
    	{
    	    if(fork() == 0)				// If child
    	    {
                char read_buf[BUFFER_SIZE];
        		printf("I am a child\n");
        		Client player = connected_clients[i];	// Set the current player
        		char *response = receive_message(player.client_fd); //Receive INIT
        		parse_message(response, player);

        		char start_message[BUFFER_SIZE];
        		sprintf(start_message,"START,%i,%i",1,num_lives);
        		send_message(start_message,player.client_fd); //Send start to Client
                int num_lives = player.num_lives;
        		while (num_lives > 0)
                {			// Plays the game, looping while there is lives left
        		    response = receive_message(player.client_fd);
        		    while (strstr(response, "MOV") == NULL) {	// Wait for move
        		        response = receive_message(player.client_fd);
        		    }
                    int parsed_response = parse_message(response, player);
        		    write(player.toParentMovePipe[1],&parsed_response, sizeof(int));
                    read(player.fromParentPipe[0],read_buf,BUFFER_SIZE);
                    if(strstr(read_buf,"FAIL") != NULL)
                    {
                        num_lives--;
                    }
                    send_message(read_buf,client.client_fd);
        		}
                exit(EXIT_SUCCESS);
    	    }
            else
            {
                break;
            }
    	}

    	int num_prev_clients;		// Number of clients in previous round
    	bool everyone_played = false;
        int bytes_read = 0;
    // Parent Loop
    	while (true) {			// Infinite loop while game is runnning
            for(int i = 0; i < num_clients;i++)
            {
                connected_clients[i].has_played = false;
            }
    	    num_prev_clients = num_clients;
    	    dice1 = ((rand() % 6) + 1) ;
    	    dice2 = ((rand() % 6) + 1) ;
    	    while (everyone_played == false) {	// Goes through each client, if at least one is not ready, set it to false
    //********* add timer for losing life for not doing move
    	        everyone_played = true;
    	        for(i = 0; i < num_clients; i++)
                {
                    if(!connected_clients[i].has_played)
                    {
                        everyone_played = false;
                        bytes_read = read(connected_clients[i].toParentMovePipe[0],&connected_clients[i].move,1);
                        if(bytes_read > 0)
                        {
                            connected_clients[i].has_played = true;
                        }
                    }
    	        }
    	    }
            for(int i = 0; i <num_clients;i++)
            {
    	        calculate (dice1, dice2, *connected_clients[i].move, &connected_clients[i]);
            }
    	    num_clients = kick_player(num_clients,connected_clients);
    	    if (num_clients == 1) {
        		send_victory(connected_clients[0]);
                close(connected_clients[0].client_fd);
        		break;			// Breaks to end the game
    	    }
    	    if (num_clients == 0) {	// There is a tie
        		for(i = 0; i < num_prev_clients; i++) {
        		    send_victory(connected_clients[i]);
                    close(connected_clients[i].client_fd);
        		    break;
        		}
    	    }
    	}
    }
}
