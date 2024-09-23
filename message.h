#include <iostream>
#include <set>
enum MessageType
{
	SymbolList  = 0,
	PriceUpdate = 1
};

inline std::string MessageTypeToString (const MessageType type)
{
	switch(type)
	{
	case 0:
		return "SymbolList";
	case 1:
		return "PriceUpdate";
	default:
		return "UnknownRequest";
	}
}

/*To send the message from Price-Manager to Price-Consumer and vice-versa
if we use already available tech such as Google-Protobuf, serilization 
and deserilization will be more efficient
*/
struct Message
{
	MessageType message_type;
	char message[10000];
	Message () = default;
	Message (const MessageType type, const std::string& data)
	{
		message_type = type;
		strncpy(message, data.c_str(), std::size(data)-1);
		message[size(data)] = '\0';
	}

	Message (const MessageType type, const std::set<std::string>& instrument_list)
	{
		message_type = type;
		std::string symbol = "";
		for(auto& instrName: instrument_list)
		{
			symbol = symbol + instrName + " ";
		}
		strncpy(message, symbol.c_str(), std::size(symbol)-1);
		message[std::size(symbol)-1] = '\0';
	}

	size_t getSize () const
	{
		return std::size(this->message);
	}
};