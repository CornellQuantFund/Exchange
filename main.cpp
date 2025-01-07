/*******************************************
 * main.cpp
 *******************************************/
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <thread>

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/throw_exception.hpp>
#include <boost/exception/diagnostic_information.hpp>

#include "httplib.h"        // https://github.com/yhirose/cpp-httplib
#include "inja/inja.hpp"    // https://github.com/pantor/inja
#include "include/exchange.h" // Your Exchange + Trader classes, etc.

using namespace std;
using namespace httplib;
using json          = nlohmann::json;
namespace beast     = boost::beast;
namespace websocket = boost::beast::websocket;
namespace asio      = boost::asio;

boost::wrapexcept<std::runtime_error> myWrappedEx(std::runtime_error("Some error"));

//------------------------------------
// GLOBALS
//------------------------------------
Exchange session(3);
static unordered_map<string,int> traderIDs;
static boost::asio::io_context g_ioc;
static std::unique_ptr<boost::asio::ip::tcp::acceptor> g_acceptor;

// A list of active WebSocket connections. 
// We’ll protect it with a mutex for thread safety.
static vector<shared_ptr<websocket::stream<beast::tcp_stream>>> activeSessions;
static mutex sessionMutex;

static unordered_map<websocket::stream<beast::tcp_stream>*, string> sessionToUserMap;
static mutex mapMutex;


/**
 * Broadcast a string message (typically JSON) to all connected WebSocket sessions.
 */
void broadcastMessage(const string &msg) {
    lock_guard<mutex> lock(sessionMutex);
    for (auto &ws : activeSessions) {
        if (ws && ws->is_open()) {
            beast::error_code ec;
            ws->text(ws->got_text());
            ws->write(asio::buffer(msg), ec);
        }
    }
}

/**
 * Build a JSON string that includes:
 *   1) All orderbooks (bids/asks)
 *   2) Each trader’s orders
 * Because we might have multiple traders, the front-end can filter by username.
 * Or you can build a “just the relevant user’s orders” response.
 */
string buildFullMarketJson() {
    // For simplicity, we’ll return a single JSON object with:
    //  { "orderbooks": [...], "allTraders": {...} }
    // You could also do one big combined structure—whatever your front-end expects.
    json root;
    
    // Orderbooks
    int nb = session.getNumOrderBooks();
    json arr = json::array();
    for(int i = 0; i < nb; i++){
        auto &ob = session.getOrderBook(i);
        auto levels = ob.GetOrderInfos();

        json bookObj;
        bookObj["contractIndex"] = i;

        // Bids
        json bidsArr = json::array();
        for (auto &b : levels.GetBids()) {
            json bObj;
            bObj["price"]    = b.price_;
            bObj["quantity"] = b.quantity_;
            bidsArr.push_back(bObj);
        }
        bookObj["bids"] = bidsArr;

        // Asks
        json asksArr = json::array();
        for (auto &a : levels.GetAsks()) {
            json aObj;
            aObj["price"]    = a.price_;
            aObj["quantity"] = a.quantity_;
            asksArr.push_back(aObj);
        }
        bookObj["asks"] = asksArr;

        arr.push_back(bookObj);
    }
    root["orderbooks"] = arr;

    // For demonstration: include *all traders’* orders in a map: { "username": [orders], ...}
    json allT;
    for (auto &item : traderIDs) {
        auto &uname = item.first;
        int tid = item.second;
        allT[uname] = session.getTrader(tid).getOrdersJson();
    }
    root["allTraders"] = allT;

    json allTraderData = json::object(); // Use object instead of array

    for (auto& [username, id] : traderIDs) {
        json traderData = session.getTrader(id).getOrdersJson();
        allTraderData[username] = traderData;
    }
    root["allTraderData"] = allTraderData; // Use camel case to match frontend

    return root.dump(); // Return as string
}

/**
 * The function handling a single WebSocket session. 
 * Accept connections, read/write messages.
 */
void doWebSocketSession(asio::ip::tcp::socket socket) {
    auto ws = make_shared<websocket::stream<beast::tcp_stream>>(std::move(socket));
    beast::error_code ec;

    // Accept the WebSocket handshake
    ws->accept(ec);
    if(ec) {
        cerr << "WS accept failed: " << ec.message() << endl;
        return;
    }

    // Read the initial message to get the username
    beast::flat_buffer buffer;
    ws->read(buffer, ec);
    if(ec) {
        cerr << "Failed to read username: " << ec.message() << endl;
        return;
    }

    // Parse the registration message
    string received = beast::buffers_to_string(buffer.data());
    json regMsg;
    try {
        regMsg = json::parse(received);
    } catch (const json::parse_error &e) {
        cerr << "JSON parse error: " << e.what() << endl;
        return;
    }

    if (regMsg.contains("type") && regMsg["type"] == "register" && regMsg.contains("username")) {
        string username = regMsg["username"];
        {
            lock_guard<mutex> lock(mapMutex);
            sessionToUserMap[ws.get()] = username;
        }
        cout << "WebSocket session registered for user: " << username << endl;
    } else {
        cerr << "Invalid registration message." << endl;
        return;
    }

    {
        lock_guard<mutex> lock(sessionMutex);
        activeSessions.push_back(ws);
    }

    // Send an initial snapshot
    {
        string fullData = buildFullMarketJson();
        ws->text(true);
        ws->write(asio::buffer(fullData), ec);
    }

    // Continuously read messages (if needed)
    while (true) {
        beast::flat_buffer readBuffer;
        ws->read(readBuffer, ec);
        if (ec == websocket::error::closed) {
            break;
        }
        if(ec) {
            cerr << "WS read error: " << ec.message() << endl;
            break;
        }

        // Handle incoming messages if necessary
        // For this use-case, you might not need to handle further messages
    }

    // Remove from active sessions and sessionToUserMap
    {
        lock_guard<mutex> lock(sessionMutex);
        activeSessions.erase(
            remove_if(activeSessions.begin(), activeSessions.end(),
                [&](const shared_ptr<websocket::stream<beast::tcp_stream>> &s) { return s.get() == ws.get(); }),
            activeSessions.end()
        );
    }
    {
        lock_guard<mutex> lock(mapMutex);
        sessionToUserMap.erase(ws.get());
    }
}



/**
 * Start the WebSocket listener on a separate port or the same port but different route. 
 * In a real app, you'd unify with the existing main thread, 
 * but here's a simplified approach using a separate thread.
 */
void doWebSocketSession(boost::asio::ip::tcp::socket socket);

/**
 * Accept loop: asynchronously accept new connections and launch WebSocket sessions.
 */
void doAccept()
{
    // Use async_accept so we don't block.
    g_acceptor->async_accept(
        [/* capture by copy if needed */](boost::system::error_code ec, boost::asio::ip::tcp::socket socket)
        {
            if(!ec) {
                // Launch a WebSocket session in a new thread, or post to a thread pool, etc.
                std::thread(&doWebSocketSession, std::move(socket)).detach();
            } else {
                // If we get here, handle error (log it, etc.).
            }
            // Accept the next connection
            doAccept();
        }
    );
}

/**
 * Start the WebSocket server on the given port, and run the io_context.
 */
void startWebSocketServer(unsigned short port)
{
    try {
        // Create the acceptor bound to (0.0.0.0, port)
        g_acceptor = std::make_unique<boost::asio::ip::tcp::acceptor>(
            g_ioc,
            boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)
        );

        std::cout << "WebSocket server listening on port " << port << std::endl;

        // Start the async accept loop
        doAccept();

        // Finally, run the io_context to process async events
        g_ioc.run();
    }
    catch (const std::exception &e) {
        std::cerr << "WS server error: " << e.what() << std::endl;
    }
}

int main() {

    // (A) Clear out completedTrades
    ofstream completedFile("completedTrades.json", ofstream::out | ofstream::trunc);
    completedFile.close();

    // (B) Start the WebSocket server in a background thread
    // You could choose a different port or the same (but you’d need a different approach).
    thread wsThread([]{
        startWebSocketServer(9090); // For example, run WS on port 8081
    });
    wsThread.detach();

    // (C) Start the HTTP server via cpp-httplib
    httplib::Server svr;
    svr.set_mount_point("/static", "./static"); // to serve style.css, etc.

    // Root: serve home
    svr.Get("/", [&](const Request& req, Response& res) {
        ifstream file("templates/home.html");
        if(file) {
            stringstream buffer;
            buffer << file.rdbuf();
            res.set_content(buffer.str(), "text/html");
        } else {
            res.set_content("Error: could not open home.html", "text/plain");
        }
    });

    // Add user
    svr.Post("/submit", [&](const Request& req, Response& res) {
        auto username = req.get_param_value("username");
        if (username.empty()) {
            res.set_content("Error: Username cannot be empty", "text/plain");
            return;
        }
        if (traderIDs.find(username) == traderIDs.end()) {
            // new user
            Trader newTrader(username, 0, vector<OrderPointer>());
            session.addTrader(newTrader);
            traderIDs[username] = newTrader.getId();
        }

        int currTraderID = traderIDs[username];

        // Render tradeFloor
        inja::Environment env;
        auto tmpl = env.parse_template("templates/tradeFloor.html");

        json data;
        data["username"] = username;
        data["id"]       = to_string(currTraderID);
        data["tradeDataJson"] = session.getTrader(currTraderID).getOrdersJson();

        string rendered = env.render(tmpl, data);
        res.set_content(rendered, "text/html");
    });

    // Place order route
    svr.Post("/tradeFloor", [&](const Request& req, Response& res) {
        string username = req.get_param_value("username");
        if(username.empty() || traderIDs.find(username) == traderIDs.end()) {
            res.set_content(R"({"error":"Invalid user"})", "application/json");
            return;
        }

        int currTraderID = traderIDs[username];
        if (req.get_param_value("isSubmitted") == "true") {
            try {
                int contract  = stoi(req.get_param_value("contract")) - 1;
                string oType  = req.get_param_value("order-type");
                string oSide  = req.get_param_value("order-side");
                double price  = stod(req.get_param_value("price"));
                int quantity  = stoi(req.get_param_value("quantity"));
                session.placeOrder(contract, oType, oSide, price, quantity, currTraderID);

            } catch (const exception &e) {
                cerr << "Error placing order: " << e.what() << endl;
                res.set_content(R"({"error":"Failed to place order"})", "application/json");
            }
        } else {
            res.set_content(R"({"message":"No order submitted"})", "application/json");
        }

        inja::Environment env;
        // Load and parse the template
        auto tmpl = env.parse_template("templates/tradeFloor.html");
        // Prepare data for placeholders
        json data;
        data["username"] = username;
        data["id"] = currTraderID;

        string updatedJson = buildFullMarketJson();
        broadcastMessage(updatedJson);      

        string rendered = env.render(tmpl, data);
        res.set_content(rendered, "text/html");

    });


        svr.Post("/calculatePnl", [&](const Request& req, Response& res) {
        session.calculateTradersPnl();

        // Prepare JSON response
        json response;
        response["message"] = "PnL calculated and notified to all users.";

        // Broadcast PnL to each user
        {
            lock_guard<mutex> lock(sessionMutex);
            lock_guard<mutex> lockMap(mapMutex);
            for (auto &ws : activeSessions) {
                if (ws && ws->is_open()) {
                    // Get username for this session
                    string username = sessionToUserMap[ws.get()];
                    // Get PnL for this user
                    double pnl = session.getTrader(traderIDs[username]).getPnl();

                    // Create PnL message
                    json pnlMsg;
                    pnlMsg["type"] = "pnl";
                    pnlMsg["pnl"] = pnl;

                    // Send PnL message as text
                    beast::error_code ec;
                    ws->text(true);
                    ws->write(asio::buffer(pnlMsg.dump()), ec);
                    if(ec) {
                        cerr << "Failed to send PnL to " << username << ": " << ec.message() << endl;
                    }
                }
            }
        }

        // Respond to admin with a confirmation
        res.set_content(response.dump(), "application/json");
    });


    svr.Get("/admin", [&](const Request& req, Response& res) {
        ifstream file("templates/admin.html");
        if(file) {
            inja::Environment env;
            auto tmpl = env.parse_template("templates/admin.html");

            json data;
            data["traders"] = json::array();
            for (auto& [username, id] : traderIDs) {
                data["traders"].push_back(username + " (ID: " + to_string(id) + ")");
            }

            string rendered = env.render(tmpl, data);
            res.set_content(rendered, "text/html");
        } else {
            res.set_content("Error: could not open admin.html", "text/plain");
        }
    });

    std::cout << "HTTP server started at http://localhost:8080" << std::endl;
    svr.listen("0.0.0.0", 8080);

    return 0;
}
