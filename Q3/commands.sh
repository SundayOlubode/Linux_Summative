echo -e "Compiling chat_server.c and chat_client.c \n"
gcc -o chat_server chat_server.c -pthread;
gcc -o chat_client chat_client.c -pthread;

echo -e "\nRunning chat_server and chat_client...\n"
./chat_server
./chat_client