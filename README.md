Currently this project supports single type of request only ie Getting list of all the supported instrument list from the PriceManager to Consumer.

Pending Work: 
Need to read the priceUpdate from the file, process those updates and send to the consumer

A few improvments needed:
Instead of building own serilization-deserilization tool, I could have used google-protobuf/json kind of tool

To Run:

1. First compile and run PriceManager using g++ -std=c++17 price_manager.cpp -o price_manager && ./price_manager
2. Once PriceManager is Up, try to compile and run PriceConsumer using g++ -std=c++17 price_consumer.cpp -o price_consumer && ./price_consumer
