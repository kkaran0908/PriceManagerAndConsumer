Functionalities Implemented:
1. Consumer can request for the latest bid/ask price for a particular instrument.
2. When PriceManager receives the price updates, these price updates are processed on different threads.
3. Consumer and PriceManager can communicate using the predefined message type.

Pending Work: 
1. Need to make consumer side multithreaded so that individual client can receive the prices update from the targeted instrument.
2. Instead of using our own serilization/deserilization tool, we should use already available tool such as json/google-protobuf etc.
3. Ideally to make thigns faster, we could have dedicate one-queue(this queue will contain price update for one particular instrument) to each thread.
4. On consumer side, we can take another component(kind of library), may be we can call it, PriceHandler, it will receive the processed price update from PriceManager and will forward the updates to all the interested clients/subscribers that we can reduce the number of messages flowing between PriceManager and PriceConsumer.
5. Executor must not implement any of the low-level functionalities, instead it should only execute different tasks.

To Run:

1. First compile and run PriceManager using g++ -std=c++17 price_manager.cpp -o price_manager && ./price_manager
2. Once PriceManager is Up, try to compile and run PriceConsumer using g++ -std=c++17 price_consumer.cpp -o price_consumer && ./price_consumer
