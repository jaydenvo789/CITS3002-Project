import socket
import sys
from time import sleep
import random
current_message = ""

class UnexpectedResponseException(Exception):
    """Raised when client receives an unexpected response"""
    pass

def make_move(client_id):
    """
    Function called to allow client to make a move
    :param client_id: Id of client making the move
    :returns: String representation of move made by client
    """
    move = random.randint(1,4)
    if move == 1:
        return client_id+",MOV,EVEN"
    elif move == 2:
        return client_id+",MOV,ODD"
    elif move == 3:
        return client_id+",MOV,DOUB"
    else:
        dice_number = random.randint(1,6)
        while dice_number < 1 or dice_number > 6:
            print("Value is not between 1-6")
        return client_id+",MOV,CON,"+ str(dice_number)

def recv_message(socket):
    message = socket.recv(14).decode()
    return message

# Create a TCP/IP socket
sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
# Connect the socket to the port where the server is listening
server_address = ('localhost', 8888)
print ('connecting to %s port %s' % server_address)

count=0
try:
    sock.connect(server_address)
    sock.sendall('INIT'.encode())
    response = recv_message(sock)
    #Client attempts to join midgame, client will attempt to reconnect every 10s
    if response == "REJECT":
        print("Unable to join game. A match is currently in progress")
        print("Attempting to find a match .....")
        while not response.startswith("WELCOME"):
            sock.sendall('INIT'.encode())
            response = recv_message(sock)
            sleep(10)
    elif response != "CANCEL":
        while True:
          client_id = response[-3:]
          print("Players ID: " + client_id)
          game_status = recv_message(sock)
          #Server Notified not enough players for match
          if game_status == "CANCEL":
              print("Not enough players for match to begin. Exiting.....")
              break
          elif game_status.startswith("START"):
              game_status = game_status.split(",")
              num_lives = int(game_status[2])
              print("Match with "+ game_status[1] + " players starting")
              print("Starting Number of Lives: " + str(num_lives))
              result = ""
              finish = False
              round = 1
              while num_lives > 0:
                  print(round)
                  move_single = make_move(client_id)
                  print(move_single)
                  sock.sendall(move_single.encode())
                  result = recv_message(sock)
                  if result.endswith("PASS"):
                    print("You have guessed correctly")
                    print("Remaining Lives: " + str(num_lives))
                  elif result.endswith("FAIL"):
                      num_lives = int(num_lives) - 1
                      print("You have guessed wrongly")
                      print("Remaining Lives: " + str(num_lives))
                  elif "VICT" in result:
                     print("You are the winner")
                     finish = True
                     break
                  elif "ELIM" in result:
                    print("You have been eliminated")
                    finish = True
                    break
                  else:
                     raise UnexpectedResponseException("Unexpected Response " + result)
                  round = round +1
          #Obtained an unexpected response
          else:
              raise UnexpectedResponseException("Unexpected Response " + game_status)
          if finish:
              break
except socket.error as e:
    print('%s: Unable to connect to %s port %s' \
           % (str(e),server_address[0],server_address[1]))
except UnexpectedResponseException as e:
    print(str(e))
finally:
    print('closing socket')
    sock.close()
