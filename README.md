# ChatRoom
This repository using socket programming in C to make a simple chat between clients via a server.

# To run this program:
You should open 3 terminals, one for server and two for clients.
1. Run `make` to run server at port 3000.
2. With 2 client terminals, run simple `./client x 3000`, x is a random integer.

# Flow of program:
1. First, when connect to server, client will receive a menu with 2 options: Login or Exit.
2. Choose Exit if you want to exit the program. If you choose Login option, you must enter your account and password.
3. After login successfully, you will receive a menu with 3 options: Chat 1-1, Chat group and Exit.
4. Choose an option and do with the instruction.

# Warning:
1. Accounts (for example) is saved in `nguoidung.txt` file and will be shown when you run server.
2. Message between client and server is encode and decode with 2 number characters (00, 01, ...) and they are added before message.
Encode and decode function is coded in files, please check.

# Have fun :v
