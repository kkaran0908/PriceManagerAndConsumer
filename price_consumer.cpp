#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>   // For inet_addr and sockaddr_in
#include <unistd.h>      // For close
#include <cstring>       // For memset
#include "instrument_symbol.h"
#include "message.h"

class PriceConsumer 
{
private:
    int sock = 0;  // Consumer socket
    struct sockaddr_in serv_addr;  // PriceManager address structure
    std::string server_ip;
    int server_port;

public:
    PriceConsumer (const std::string& ip, int port) : server_ip(ip), server_port(port) {}

    void connect_to_price_manager () 
    {
        sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) {
            std::cerr << "Socket creation error" << std::endl;
            exit(EXIT_FAILURE);
        }

        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(server_port);

        if (inet_pton(AF_INET, server_ip.c_str(), &serv_addr.sin_addr) <= 0) {
            std::cerr << "Invalid address/ Address not supported" << std::endl;
            exit(EXIT_FAILURE);
        }

        // Connect to the server
        if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
            std::cerr << "Connection failed" << std::endl;
            exit(EXIT_FAILURE);
        }

        std::cout << "Connected to server " << server_ip << " on port " << server_port << std::endl;
    }

    void send_message (const Message& message) 
    {
        send(sock, &message, message.getSize(), 0);
        std::cout << "Request sent to PriceManager, RequestType: " << MessageTypeToString(message.message_type) << std::endl;
    }

    std::string receive_update () {
    	Message msg;
		ssize_t bytesReceived = recv(sock, &msg, sizeof(msg), 0);
        if ( bytesReceived < 0) {
            std::cerr << "Failed to receive message" << std::endl;
        }
        return std::string(msg.message);
    }

    // Close the connection
    void close_connection () {
        close(sock);
        std::cout << "\nConnection closed" << std::endl;
    }
};

int main () 
{
    std::string server_ip = "127.0.0.1";
    int server_port = 8080;
    PriceConsumer pc(server_ip, server_port);
    pc.connect_to_price_manager();
    while(true)
    {
        std::cout << "1. Press-1 for instrumentList!" << std::endl;
        std::cout << "2. Press-2 for priceUpdate!" << std::endl;
        std::cout << "3. Press-3 to close the connection!" << std::endl;
        int userInput;
        std::cin >> userInput; 

        if(userInput == 1)
        {
            Message message(MessageType::SymbolList, "Please send the whole list of available instruments\n");
            pc.send_message(message);
            std::string response = pc.receive_update();
            std::cout << "\n*********Supported List of Instruments: " << response << std::endl;
        }
        else if(userInput == 2)
        {
            std::string instrName;
            std::cin >> "Please provide the instrument:\t" << instrName << std::endl;
            Message message(MessageType::PriceUpdate, "Please send the PriceUpdate\n", instrName);
            pc.send_message(message);
            std::string response1 = pc.receive_update();
            std::string response2 = pc.receive_update();
            std::cout << "\nPriceUpdate:\t" << response1 << std::endl;
            std::cout << "\nPriceUpdate:\t" << response2 << std::endl;
        }
        else
        {
            Message message(MessageType::PriceDisconnect, "Please disconnect the connection\n");
            pc.send_message(message);
            pc.close_connection();
            break;
        }
    }
    

    return 0;
}