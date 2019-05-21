#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#define BUFFER_SIZE 1024
#define JOIN_WAITING_TIME 10
#define ROUND_WAITING_TIME 10
typedef struct
{
    int client_id;
    int num_lives;
    int client_fd;
    bool has_played; //Bool expression to indicate that the state of the client
    int move; //to store move made by client
    int toParentMovePipe[2];		// Pipe child uses to write the move to parent
    int fromParentPipe[2];		// Pipe child uses to read from parent
} Client;

int dice1, dice2;

/*
*Validates that packets received from a client are in the correct format
*@param client_id: ID of the client who sent the packet
*@param message: Message received from the client
*@return bool: True if message is in correct format, else false
*/
bool validate_message(char * message, int client_id)
{
    printf("%s\n",message);
    if (strlen(message) > 14) {
        printf("The packet is too long, causing it to be invalid\n");
        return false;
    }
    if(strstr(message,"INIT") != NULL && !strcmp(message, "INIT")) return true;
    else if(strstr(message,"MOV") != NULL)
    {
        char packet[14];
        if (strstr(message,"EVEN") != NULL) {
            sprintf(packet,"%d,MOV,EVEN", client_id);
            if(strcmp(message, packet) != 0) {
                printf("Invalid EVEN move packet\n");
                return false;
            }
        }
        if (strstr(message,"ODD") != NULL) {
            sprintf(packet,"%d,MOV,ODD", client_id);
            if(strcmp(message, packet) != 0) {
                printf("Invalid ODD move packet\n");
                return false;
            }
        }

        if (strstr(message,"DOUB") != NULL) {
            sprintf(packet,"%d,MOV,DOUB", client_id);
            if(strcmp(message, packet) != 0) {
                printf("Invalid DOUB move packet\n");
                return false;
            }
        }
        if (strstr(message,"CON") != NULL) {
            sprintf(packet,"%d,MOV,CON,", client_id);
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
        fprintf(stderr,"Client write failed - %s\n",message);
        exit(EXIT_FAILURE);
    }
    sleep(1);
}

/*
*Receives message sent from client
*@sender_fd - Clients socket fd
*@return char* - Message from client
*/
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

/*
*Interprets message sent from client
*@param message: message sent from client
*@param Client: Struct containing client information
*@return int representing move made by client
*/
int parse_message(char *message, Client client )
{
    int enum_value = 9;		// Default, empty value
    if(validate_message(message,client.client_id))
    {
        if(strstr(message,"INIT") != NULL)
        {
            int bytes_required = strlen("WELCOME,") + sizeof(char) + 1;
            char *welcome_message = calloc(bytes_required, sizeof(char));
            if(welcome_message != NULL)
            {
                sprintf(welcome_message,"WELCOME,%i",client.client_id);
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

    int bytes_required = sizeof(char) + strlen(",XXXX") + 1;
    char *result_message = calloc(bytes_required, sizeof(char));

    if(result_message != NULL) sprintf(result_message,"%d",client->client_id);

    else
    {
        printf("Cannot allocate %i bytes of memory\n",bytes_required);
        exit(EXIT_FAILURE);
    }

    if (enum_value < 0 || enum_value > 9)
    {
        fprintf(stderr,"Invalid enum value");
        return;
    }

    else if (enum_value == 0)    // Odd
    {    // Fails if even or dice-sum is not greater than 5
        if ((dice1 + dice2)%2 == 0 || dice1 + dice2 <= 5) failed_round = true;
        printf("Player %d chose Odd\n",client->client_id);
    }

    else if (enum_value == 7)    // Doubles            else
    {
        if (dice1 != dice2) failed_round = true;
        printf("Player %d chose Doubles\n",client->client_id);
    }

    else if (enum_value == 8)    // Even
    {    // Fails if odd or doubles
        if ((dice1 + dice2)%2 == 1 || dice1 == dice2) failed_round = true;
        printf("Player %d chose Even\n",client->client_id);
    }

    else if (enum_value == 9)
    {
        failed_round = true;
        printf("Player %d did not select a move\n",client->client_id);
    }
    else
    {
        if (dice1 != enum_value && dice2 != enum_value) failed_round = true;
        printf("Player %d chose %d\n", client->client_id, enum_value);
    }

    if (failed_round)
    {
        strcat(result_message,",FAIL");
        write(client->fromParentPipe[1],result_message,strlen(result_message)+1);
        client->num_lives--;
        printf("Player %d has this failed round, lives left: %d\n", client->client_id, client->num_lives);
    }
    else {
        strcat(result_message,",PASS");
        write(client->fromParentPipe[1],result_message,strlen(result_message)+1);
        printf("Player %d has survived... this round\n",client->client_id);
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
    Client **a_connected_clients = &connected_clients;
    int num_people_kicked = 0;
    for(int i = 0; i < num_clients; i++)
    {
        if (connected_clients[i].num_lives == 0)
        {
            num_people_kicked++;
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
	    printf("kicking %d, clients %d\n", num_people_kicked, num_clients);
            char *result_message;
            if (connected_clients[j].num_lives == 0) //Client dead so kick them
            {
                send_message("ELIM", connected_clients[j].client_fd);
                close(connected_clients[j].client_fd);
            }
            else
            {
                surviving_players[index] = connected_clients[j];
                index++;
            }
        }
        num_clients = num_clients - num_people_kicked;
        *a_connected_clients = surviving_players;
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
    fcntl(server_fd,F_SETFL, O_NONBLOCK); //Set server socket to non blocking
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
    time_t timer_start,timer_end;
    double elapsed;
    int max_clients = 10;
    int num_clients = 0;
    int id_iterator = 0;
    Client *connected_clients;
    time(&timer_start);
    while (true) {
        socklen_t client_len = sizeof(client);
        char client_id[2];
        while (num_clients <= max_clients) {
            time(&timer_end);
            elapsed = difftime(timer_end,timer_start);
            if(elapsed > JOIN_WAITING_TIME)
            {
                break;
            }
            client_fd = accept(server_fd, (struct sockaddr *) &client, &client_len);
            if (client_fd < 0) {
                // printf("%f\n",elapsed);
                continue;
            }
            id_iterator = (id_iterator + 1) % 10;
            // snprintf(client_id, 2, "%d", id_iterator);
            // sprintf(client_id, "%d", id_iterator); 			// Set client ids
            Client single_client = {id_iterator,num_lives,client_fd,false,9};
            pipe(single_client.toParentMovePipe);
            pipe(single_client.fromParentPipe);
            fcntl(single_client.toParentMovePipe[0], F_SETFL, O_NONBLOCK);
            num_clients++;
            //Store a connected reference to the client
            if(num_clients == 1)
            {
                connected_clients = calloc(1,sizeof(Client));
                connected_clients[num_clients-1] = single_client;
            }
            else
            {
                connected_clients = realloc(connected_clients,num_clients * sizeof(Client));
                connected_clients[num_clients-1] = single_client;
            }
            if(connected_clients == NULL)
            {
                printf("Cannot allocate %lu bytes of memory\n",num_clients * sizeof(Client));
                exit(EXIT_FAILURE);
            }
        }
        int reject_process_fd[2];
        pipe(reject_process_fd);
        fcntl(reject_process_fd[0], F_SETFL, O_NONBLOCK);
        if(fork() == 0) // Child Process used to reject clients who are trying connect when game has started
        {
            char read_buf[BUFFER_SIZE];
            printf("%i pid of the rejector\n",getpid());
            while(true)
            {
                sleep(1);
                int bytes_read = read(reject_process_fd[0],read_buf,BUFFER_SIZE);
                if(bytes_read > 0 && strstr("stop",read_buf) != NULL)
                {
                    close(server_fd);
                    exit(EXIT_SUCCESS);
                }
                client_fd = accept(server_fd, (struct sockaddr *) &client, &client_len);
                if (client_fd < 0) {
                    // printf("%f\n",elapsed);
                    continue;
                }
                send_message("REJECT",client_fd);
            }

        }
    	for(int i = 0; i < num_clients; i++) 		// Create a child for each client
    	{
    	    if(fork() == 0)				// If child
    	    {
                printf("%i pid of the client\n",getpid());
                char read_buf[BUFFER_SIZE];
        		printf("I am a child\n");
        		Client player = connected_clients[i];	// Set the current player
        		char *response = receive_message(player.client_fd); //Receive INIT
        		parse_message(response, player);

        		char start_message[BUFFER_SIZE];
        		sprintf(start_message,"START,%i,%i",num_clients,num_lives);
        		send_message(start_message,player.client_fd); //Send start to Client
                int num_lives = player.num_lives;
        		while (num_lives > 0)
                {			// Plays the game, looping while there is lives left
        		    response = receive_message(player.client_fd);
        		    while (strstr(response, "MOV") == NULL) {	// Wait for move
        		        response = receive_message(player.client_fd);
        		    }
                    int parsed_response = parse_message(response, player);
		    printf("parsed response %i\n", parsed_response);
        		    write(player.toParentMovePipe[1],&parsed_response, sizeof(int));
                    read(player.fromParentPipe[0],read_buf,BUFFER_SIZE);
                    if(strstr(read_buf,"FAIL") != NULL)
                    {
                        char *fail_message = malloc(strlen(read_buf)+1);
                        strcpy(fail_message,read_buf);
                        num_lives--;
/**
                        if(num_lives == 0)
                        {
                            send_message("ELIM", player.client_fd);
                            close(player.client_fd);
                            break;
                        }
**/
                        read(player.fromParentPipe[0],read_buf,BUFFER_SIZE);
                        if(strstr(read_buf,"VICT") != NULL)
                        {
                            send_victory(player);
                            close(player.client_fd);
                            break;
                        }
                        else
                        {
                            send_message(fail_message, player.client_fd);
                        }
                    }
                    if(strstr(read_buf,"PASS") != NULL)
                    {
                        char *pass_message = malloc(strlen(read_buf)+1);
                        strcpy(pass_message,read_buf);
                        read(player.fromParentPipe[0],read_buf,BUFFER_SIZE);
                        if(strstr(read_buf,"VICT") != NULL) //No need to send pass message. Just Victory
                        {
                            send_victory(player);
                            close(player.client_fd);
                            break;
                        }
                        else
                        {
                            send_message(pass_message, player.client_fd);
                        }
                        free(pass_message);
                    }
        		}
                close(server_fd);
                exit(EXIT_SUCCESS);
    	    }
    	}

    	int num_prev_clients;		// Number of clients in previous round
        int bytes_read;
        bool everyone_played;
        printf("%i pid of the parent\n",getpid());

    // Parent Loop
    	while (true) {			// Infinite loop while game is runnning
            for(int i = 0; i < num_clients;i++)
            {
                connected_clients[i].has_played = false;
            }
            time_t time_current, time_start;
            double elapsed_time;
            everyone_played = false;
    	    num_prev_clients = num_clients;
    	    dice1 = ((rand() % 6) + 1) ;
    	    dice2 = ((rand() % 6) + 1) ;
            time(&time_start);
            printf("Dice 1: %i, Dice 2: %i\n",dice1,dice2);
    	    while (everyone_played == false) {	// Goes through each client, if at least one is not ready, set it to false
                time(&time_current);
                elapsed_time = difftime(time_current, time_start);
                if(elapsed_time > ROUND_WAITING_TIME)
                {
                    break;
                }
    	        everyone_played = true;
    	        for(int i =  0; i < num_clients; i++)
                {
                    // printf("client %i, has played? %i\n",i,connected_clients[i].has_played);
                    if(!connected_clients[i].has_played)
                    {
                        everyone_played = false;
                        bytes_read = read(connected_clients[i].toParentMovePipe[0],&connected_clients[i].move,sizeof(int));
                        if(bytes_read > 0)
                        {
                            connected_clients[i].has_played = true;
                        }
                    }
    	        }
    	    }
            for(int i = 0; i <num_clients;i++)
            {
    	        calculate (dice1, dice2, connected_clients[i].move, &connected_clients[i]);
            }
    	    num_clients = kick_player(num_clients,connected_clients);
    	    if (num_clients == 1) {
		printf("The Winner is... Player %d with %d lives left\n", connected_clients[0].client_id,connected_clients[0].num_lives);
                write(connected_clients[0].fromParentPipe[1],"VICT",strlen("VICT"));
                write(reject_process_fd[1],"stop",strlen("stop")+1);
                close(connected_clients[0].client_fd);
                close(server_fd);
                exit(EXIT_SUCCESS);
    	    }
    	    else if (num_clients == 0) {	// There is a tie
		    printf("There was a tie\n");
        		for(int i = 0; i < num_prev_clients; i++) {
                    write(connected_clients[i].fromParentPipe[1],"VICT",strlen("VICT")+1);
                    write(reject_process_fd[1],"STOP",strlen("STOP")+1);
                    close(connected_clients[i].client_fd);
        		}
                close(server_fd);
                exit(EXIT_SUCCESS);
    	    }
            else
            {
                for(int i = 0; i < num_clients;i++)
                {
                    write(connected_clients[i].fromParentPipe[1],"continue",strlen("continue")+1);
                    connected_clients[i].has_played = false;
                    connected_clients[i].move = 9;
                }
                printf("-----------------------\n");
            }
    	}
    }
}
