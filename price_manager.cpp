#include <iostream>
#include <thread>
#include <fstream>
#include <sstream>
#include <string>
#include <set>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h> 
#include <sys/socket.h>
#include <netinet/in.h>

#include "instrument_symbol.h"
#include "message.h"

class PriceManager {
private:
    int server_fd;           // PriceManager file descriptor
    int port;
    struct sockaddr_in address;  // PriceManager address structure
    const int backlog = 10;  // number of connections
    std::set<std::string> symbolList;

    //prepare the list of all the supported instrument list so that consumer can choose among all the available instruments
    bool CreateListOfSupportedInstrument ()
    {
    	std::ifstream file("Dummy_TBT.CSV");
    	if (!file.is_open())
    	{
    		std::cout << "Error in opening the file" << std::endl;
    		return false;
    	}

    	size_t count = 0;
    	std::string line;
    	std::vector<std::vector<std::string>> data;
    	std::getline(file, line);

    	while (std::getline(file, line)) 
    	{
    		std::stringstream ss(line);

    		//std::cout << "ss:\t" << ss;
        	std::string token;
        	char delim = ',';
        	if (std::getline(ss, token, delim)) 
        	{
        		count++;
        		std::cout << "token:\t" << token << std::endl;
            	symbolList.insert(token);
        	}
    	}

    	std::cout << "Total Instruments Supported by This Server:\t" << symbolList.size() << std::endl;

    	file.close();

    	return true;
    }


public:
    PriceManager (int port) : port(port) {}

    void initialize () 
    {
        server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd == 0) {
            perror("Socket failed");
            exit(EXIT_FAILURE);
        }

        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(port);

        if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
            perror("Bind failed");
            close(server_fd);
            exit(EXIT_FAILURE);
        }

        if (listen(server_fd, backlog) < 0) 
        {
            perror("Listen failed");
            close(server_fd);
            exit(EXIT_FAILURE);
        }

        //we will prepare a set of all the supported instrument by this server
        if (!CreateListOfSupportedInstrument())
        {
        	close(server_fd);
            exit(EXIT_FAILURE);
        }

        std::cout << "Server started on port " << port << std::endl;
    }

    void HandleInstrumentListRequest (const int client_socket)
    {
    	Message msg(MessageType::SymbolList, symbolList);
    	send(client_socket, &msg, msg.getSize(), 0);
    }

    void HandleInstrumentPriceUpdateRequest (const int client_socket) 
    {
    	/*will contain the implementation of fetching the price update from the file and 
    	forwarding the update to the consumer*/
    }

    // Function to handle communication with the client
    void handle_consumer (int client_socket) 
    {

    	if (1)
    	{
	    	Message msg;
	        ssize_t bytesReceived = recv(client_socket, &msg, sizeof(msg), 0);

	        std::cout << "Request received from consumer, RequestType:\t" << MessageTypeToString(msg.message_type) << std::endl;
        	switch(msg.message_type)
        	{
        		case 0: //send list of all the available instrument
        			HandleInstrumentListRequest(client_socket);
        			break;
        		case 1: //send list of all the available instrument
        			HandleInstrumentPriceUpdateRequest(client_socket);
        			break;
        		default:
        			std::cout << "This is an invalid request" << std::endl;
        			break;
        	}
	    }
		close(client_socket);
    }

    // accept the request from consumer
    void start () {
        int addrlen = sizeof(address);
        while (true) {
            int client_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
            if (client_socket < 0) {
                perror("Accept failed");
                continue;
            }

            // Create a new thread to handle the consumer connection
            std::thread(&PriceManager::handle_consumer, this, client_socket).detach();
        }
    }

    ~PriceManager() {
        close(server_fd);
    }
};

int main ()
{
    PriceManager pm(8080);  
    pm.initialize();     
    pm.start();

    return 0;
}

