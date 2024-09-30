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
#include <queue>
#include <unordered_map>
#include <cctype>
#include "instrument_symbol.h"
#include "message.h"
#include <map>

std::queue<std::string> priceUpdateStream;
std::unordered_map<double, Order> bidOrders_;
std::unordered_map<double, Order> askOrders_;
std::unordered_map<std::string, std::map<double, int>> instrToBidOrderBook_;
std::unordered_map<std::string, std::map<double, int>> instrToAskOrderBook_;

//ideally Executor should only execute the function and should not 
//contain function associated to process the price update
class Executor
{
private:
    std::atomic<bool> is_done;
    std::vector<std::thread> worker_thread_;
    std::mutex mtx_;
    std::unordered_map<std::string, std::atomic<bool>> is_being_processed;

    void UpdateInstrToOrderBook(std::vector<std::string> update)
    {
        auto price_level = std::stod(trim(update[3])); //need to handle exception
        auto qty   = std::stod(trim(update[4])); //need to handle exception
        if (update[2] == "B")
        {
            auto it = instrToBidOrderBook_.find(update[0]);
            if (update[1] == "INSERT")
            {
                if (it !=instrToBidOrderBook_.end())
                {
                    it->second[price_level] = it->second[price_level] + qty;
                }
                else
                {
                    instrToBidOrderBook_[update[0]].insert({price_level, qty});
                }
            
            }
            if (update[1] == "REMOVE")
            {
                it->second[price_level] = it->second[price_level] - qty;
            }
        }
        else
        {

            auto it = instrToAskOrderBook_.find(update[0]);
            if (update[1] == "INSERT")
            {
                if (it !=instrToAskOrderBook_.end())
                {
                    it->second[price_level] = it->second[price_level] + qty;
                }
                else
                {
                    instrToAskOrderBook_[update[0]].insert({price_level, qty});
                }
            }
            else
            {
                it->second[price_level] = it->second[price_level] - qty;
            }
        }
    }

    //instead of processing different updates on different threads, 
    //if we have dedicated thread for each instrument, it will be way faster than 
    //this appraoch
    void ProcessPriceUpdates()
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        while(true)
        {
            mtx_.lock();
            if(!priceUpdateStream.empty())
            {
                auto priceUpdate = priceUpdateStream.front();
                std::stringstream ss(priceUpdate);

                //std::cout << "PriceStreamUpdate:\t" << priceUpdate << std::endl;
                
                std::vector<std::string> price_update;
                std::string token;
                while(std::getline(ss, token, ','))
                {
                    price_update.push_back(token);
                }

                UpdateInstrToOrderBook(price_update);

                priceUpdateStream.pop();
            }
            else
            {
                mtx_.unlock();
                break;
            }
            mtx_.unlock();
            //this will terminate the thread
            //and application will be able to start shutdown
            if (is_done)
            {
                break;
            }
        }
        
    }

public:
    Executor(const int worker_count)
    {
        is_done = false;
        for(auto i=0; i<worker_count; i++)
        {
            worker_thread_.push_back(std::thread(&Executor::ProcessPriceUpdates, this));
        }
    }

    ~Executor()
    {
        is_done = true;
        for (std::thread& worker : worker_thread_)
        {
            if(worker.joinable())
            {
                worker.join();
            }
        }
    }
};

class PriceManager {
private:
    int server_fd;           // PriceManager file descriptor
    int port;
    struct sockaddr_in address;  // PriceManager address structure
    const int backlog = 10;  // number of connections
    std::set<std::string> symbolList;
    std::shared_ptr<Executor> thread_executor;

    std::map<double, int> GetInstrToBidOrderBook(const std::string instr_name) const
    {
        return instrToBidOrderBook_[instr_name];
    }

    std::map<double, int> GetInstrToAskOrderBook(const std::string instr_name) const
    {
        return instrToAskOrderBook_[instr_name];
    }

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
        		//std::cout << "token:\t" << token << std::endl;
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

        thread_executor = std::make_shared<Executor>(5);

        std::cout << "Server started on port " << port << std::endl;
    }

    void OnPriceUpdate()
    {
        std::ifstream file("Dummy_TBT.CSV");

        if (!file.is_open())
        {
            std::cout << "Some problem with priceUpdate file" << std::endl;
            return;
        }

        std::string line;
        std::getline(file, line);

        int count = 0;

        std::cout << line << std::endl;
        while(std::getline(file, line))
        {
            count++;
            priceUpdateStream.push(line);
            //std::cout << "Record:\t" << line << std::endl;
        }

        std::cout << "total record inserted:\t" << count << std::endl;

        file.close();
    }

    void HandleInstrumentListRequest (const int client_socket)
    {
    	//OnPriceUpdate();
    	Message msg(MessageType::SymbolList, symbolList);
    	send(client_socket, &msg, msg.getSize(), 0);
    }

    void HandleInstrumentPriceUpdateRequest (const int client_socket) 
    {
        std::cout << "Asking for price-update" << std::endl;
        std::string instr = "PPP";
        auto bid_order_book_ = GetInstrToBidOrderBook(instr);
        auto ask_order_book_ = GetInstrToAskOrderBook(instr);

        auto it1 = bid_order_book_.begin();
        auto it2 = ask_order_book_.rbegin();
        Message msg1(MessageType::PriceUpdate, it1->first, it1->second, instr, "Bid");
        Message msg2(MessageType::PriceUpdate, it2->first, it2->second, instr, "Ask");
        send(client_socket, &msg1, msg1.getSize(), 0);
        send(client_socket, &msg2, msg2.getSize(), 0);
    }

    // Function to handle communication with the client
    void handle_consumer (int client_socket) 
    {
        bool should_close_connection = false;
        while(true)
        {
        	Message msg;
            ssize_t bytesReceived = recv(client_socket, &msg, sizeof(msg), 0);

            std::cout << "Request received from consumer, RequestType:\t" << MessageTypeToString(msg.message_type) << std::endl;
        	switch(msg.message_type)
        	{
        		case 0: //send list of all the available instrument
        			HandleInstrumentListRequest(client_socket);
        			break;
        		case 1:
        			HandleInstrumentPriceUpdateRequest(client_socket);
        			break;
        		default:
                    should_close_connection = true;
        			std::cout << "Closing the client connection" << std::endl;
        			break;
        	}

            if (should_close_connection)
                break;
        }
        close(client_socket);
    }

    // accept the request from consumer
    void start () 
    {
        int addrlen = sizeof(address);
        while (true) 
        {
            int client_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
            if (client_socket < 0) {
                perror("Accept failed");
                continue;
            }
            std::thread(&PriceManager::handle_consumer, this, client_socket).detach();
        }
    }

    ~PriceManager() 
    {
        close(server_fd);

    }
};

int main ()
{
    PriceManager pm(8080);
    pm.initialize();
    pm.OnPriceUpdate();       
    pm.start();
    return 0;
}

