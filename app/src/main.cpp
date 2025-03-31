# include "NetLinkClient.h"
# include <iostream>
int main(){
    NetlinkClient client; // Create a Netlink client object
    std::string message = "subscribe"; // Message to send
    if (!client.sendMessage(message)) { // Send the message to the kernel
        std::cerr << "Error: Failed to send message to kernel\n";
        return 1;
    }
    client.receiveMessage(); // Receive the reply from the kernel
    message = "unsubscribe"; // Message to send
    if (!client.sendMessage(message)) { // Send the message to the kernel
        std::cerr << "Error: Failed to send message to kernel\n";
        return 1;
    }
    std::cout << "finished " << message << std::endl;
    return 0;

}