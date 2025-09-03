#include <iostream>                            // Console output
#include <thread>                              // Multi-threading
#include <mutex>                               // Thread synchronization
#include <chrono>                              // Time functions
#include <nlohmann/json.hpp>                   // JSON handling
#include <websocketpp/config/asio_no_tls.hpp>  // WebSocket++ without encryption
#include <websocketpp/server.hpp>              // Main WebSocket server class

using json = nlohmann::json;
using server = websocketpp::server<websocketpp::config::asio>;

// Thread-safe global variables
std::mutex data_mutex;                         // Prevents data races between threads
websocketpp::connection_hdl client_connection; // Stores current client connection
bool client_connected = false;                 // Connection status flag
server* srv_ptr = nullptr;                     // Pointer to server instance



//intput values from controller
float velocity =0;

int serverFunction() {
    server srv;                    // Create WebSocket server instance
    srv_ptr = &srv;                // Store global pointer to server
    srv.init_asio();               // Initialize ASIO networking library
    
    srv.clear_access_channels(websocketpp::log::alevel::all);
    srv.set_access_channels(websocketpp::log::alevel::connect | 
                           websocketpp::log::alevel::disconnect |
                           websocketpp::log::alevel::fail);
    
    srv.clear_error_channels(websocketpp::log::elevel::all);
    srv.set_error_channels(websocketpp::log::elevel::rerror | 
                          websocketpp::log::elevel::fatal);



    //Connection Handler
    srv.set_open_handler([&](websocketpp::connection_hdl hdl) {
    std::lock_guard<std::mutex> lock(data_mutex);  // Lock for thread safety
    client_connection = hdl;                       // Store client connection
    client_connected = true;                       // Set connection flag
    std::cout << "✅ Client connected!" << std::endl;
    });

    //Disconnection Handler
    srv.set_close_handler([&](websocketpp::connection_hdl hdl) {
    std::lock_guard<std::mutex> lock(data_mutex);
    client_connected = false;  // Update connection status
    std::cout << "❌ Client disconnected!" << std::endl;
    });

    // Error Handler
    srv.set_fail_handler([&](websocketpp::connection_hdl hdl) {
    std::cout << "⚠️ Connection failed!" << std::endl;
    });


    //receive data from client.
    srv_ptr->set_message_handler([&](websocketpp::connection_hdl hdl, server::message_ptr msg) {
    std::string payload = msg->get_payload();  // Get received data
    //std::cout << "📨 Received: " << payload << std::endl;

    // Try to parse as JSON
    try {
        json rec_data = json::parse(payload);
        velocity = rec_data["velocity"];
    } catch (const std::exception& e) {std::cout << "📝 Plain text message" << std::endl;}

    // Send acknowledgment back to client
    json ack = {/*...*/};
    srv_ptr->send(hdl, ack.dump(), websocketpp::frame::opcode::text);
    });



    srv.listen(9002);        // Listen on port 9002
    srv.start_accept();      // Start accepting connections
    srv.run();  
    return 0;

}




int main(){
    std::thread server_thread(serverFunction); //mutli-threading server
    // Give server time to start
    
    while (true)
    {   

        //sending data from server.
        if (client_connected && srv_ptr != nullptr){
            //creating data to be sent
            float quatW = -1;
            float quatX = 1;
            float quatY = 1;
            float quatZ = -1;

            json data={
                "quatW",quatW,
                "quatX",quatX,
                "quatY",quatY,
                "quatZ",quatZ,
            };

            try{srv_ptr->send(client_connection, data.dump(), websocketpp::frame::opcode::text);
            std::cout << "[x] " << velocity <<std::endl;}
            catch(const std::exception& e){
                //std::cout << "❌ Failed to send: " << e.what() << std::endl;
                client_connected = false;  // reseting to stop sending data
            }
        }
        //std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    

    return 0;


}