import socket
from time import sleep

class UnexpectedResponseException(Exception):
    """Raised when client receives an unexpected response"""
    pass

def make_move(client_id):
    move = int(input('Select a move:\n 1 - Even\n 2 - Odd\n 3 - Doubles\n 4 - Contains "n" \n Move: '))
    while move != 1 and move != 2 and move != 3 and move != 4:
        move = input('Invalid input. Please re-enter move: ')
    if move == 1:
        return client_id+"MOV,EVEN"
    elif move == 2:
        return client_id+"MOV,ODD"
    elif move == 3:
        return client_id+"MOV,DOUB"
    else:
        dice_number = int(input("Select a number from 1-6: "))
        while dice_number < 1 or dice_number > 6:
            print("Value is not between 1-6")
            dice_number = int(input("Select a number from 1-6: "))
        return client_id+"MOV,CON,"+ str(dice_number)

def recv_all():
    while True:
        bytes_expected = int(socket.recv(4).decode())
        message = sock.recv(bytes_expected).decode()
        yield message
# Create a TCP/IP socket
sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

# Connect the socket to the port where the server is listening
server_address = ('localhost', 4444)
print ('connecting to %s port %s' % server_address)
sock.connect(server_address)

count=0
try:
    sock.sendall('INIT'.encode())
    response = sock.recv(16).decode()
    print(response)
    #Client attempts to join midgame, client will attempt to reconnect every 10s
    if response == "REJECT":
        print("Unable to join game. A match is currently in progress")
        print("Attempting to find a match .....")
        while not response.startswith("WELCOME"):
            sock.sendall('INIT'.encode())
            response = sock.recv(16).decode()
            sleep(10)
    while True:
      client_id = response.decode()[-3:]
      game_status = sock.recv(16).decode()
      #Server Notified not enough players for match
      if game_status == "CANCEL":
          print("Not enough players for match to begin. Exiting.....")
          break
      elif game_status.startswith("START"):
          game_status = game_status.split(",")
          num_lives = game_status[2]
          print("Match with "+ game_status[1] + " players starting")
          print("Starting Number of Lives: " + num_lives)
          result = None;
          while result != "ELIM" and result != "VIC":
              move = make_move(client_id)
              sock.sendall(move.encode())
              result = sock.recv(16).decode()
              if result == "PASS":
                print("You have guessed correctly")
              elif result == "FAIL":
                  num_lives -= 1
                  print("You have guessed correctly")
                  print("Remaining Lives: " + num_lives)
              else:
                  raise UnexpectedResponseException("Unexpected Response "+ result)
          if result == "ELIM":
             print("You have been eliminated")
          else:
             print("You are the winner")
      #Obtained an unexpected response
      else:
          raise UnexpectedResponseException("Unexpected Response " + game_status)
except UnexpectedResponseException as e:
    print(str(e))
finally:
    print('closing socket')
    sock.close()
