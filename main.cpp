#include <iostream>
#include <vector>
#include <string>
#include "include/exchange.h"
#include "httplib.h" // Requires cpp-httplib (https://github.com/yhirose/cpp-httplib)
#include "inja/inja.hpp" 

using namespace std;
using namespace httplib;
using json = nlohmann::json;

Exchange session(3);
static vector<string> users;
static unordered_map<string, int> traderIDs;

int main() {
    // Clear the contents of completed.json file before starting the server
    ofstream completedFile("completed.json", ofstream::out | ofstream::trunc);
    completedFile.close();

    
    httplib::Server svr;
    svr.set_mount_point("/static", "./static");

    // Home Page for log in
    svr.Get("/", [&](const Request& req, Response& res) {
        ifstream file("templates/home.html");
        if (file) {
            stringstream buffer;
            buffer << file.rdbuf();
            res.set_content(buffer.str(), "text/html");
        } else {
            res.set_content("Error: Could not open home.html", "text/plain");
        }
    });

    // Handle form submission for adding users
    svr.Post("/submit", [&](const Request& req, Response& res) {
        auto username = req.get_param_value("username");
        if (!username.empty() && traderIDs.find(username) == traderIDs.end()) {
            users.push_back(username);

            Trader newTrader(username, 0, vector<OrderPointer>());
            session.addTrader(newTrader);
            traderIDs[username] = newTrader.getId();
            
            inja::Environment env;
            auto tmpl = env.parse_template("templates/tradeFloor.html");
            json data;
            data["username"] = username;
            data["id"] = to_string(traderIDs[username]);
            data["tradeData"] = "";
            string rendered = env.render(tmpl, data);
            res.set_content(rendered, "text/html");
        }
        else {
            res.set_content("Error: Username already exists or is empty", "text/plain");
            return;
        }
    });

    svr.Post("/tradeFloor", [&](const Request& req, Response& res) {
        string username = req.get_param_value("username");
        int currTraderID = traderIDs[username];
        string submissionContent;
        std::cout << "User: " << username << " ID: " << currTraderID << std::endl;

        if (req.get_param_value("isSubmitted") == "true") {
            try {
                int contract = stoi(req.get_param_value("contract")) - 1; // Contracts 1 indexed
                string orderType = req.get_param_value("order-type");
                string orderSide = req.get_param_value("order-side");
                double price = stod(req.get_param_value("price"));
                int quantity = stoi(req.get_param_value("quantity"));

                // Pass the order details to the exchange
                session.placeOrder(contract, orderType, orderSide, price, quantity, currTraderID);
                
                for (auto& u : session.getOrderBooks()) {
                    std::cout << u->Size() << std::endl;
                }

            } 
            catch (const exception& e) {
                submissionContent += "Error placing order: " + string(e.what());
            }
        }

        inja::Environment env;
        // Load and parse the template
        auto tmpl = env.parse_template("templates/tradeFloor.html");
        // Prepare data for placeholders
        json data;
        data["username"] = username;
        data["id"] = currTraderID;
        data["tradeData"] = session.getTrader(currTraderID).getOrdersFormatted() + "\n";
        string rendered = env.render(tmpl, data);
        res.set_content(rendered, "text/html");
    });

    
    // Handle order submission in main.cpp
    svr.Post("/submit_order", [&](const Request& req, Response& res) {
        try {
            int contract = stoi(req.get_param_value("contract")) - 1; // Contracts 1 indexed
            string orderType = req.get_param_value("order-type");
            string orderSide = req.get_param_value("order-side");
            double price = stod(req.get_param_value("price"));
            int quantity = stoi(req.get_param_value("quantity"));
            int id = traderIDs[req.get_param_value("username")];

            // Pass the order details to the exchange
            session.placeOrder(contract, orderType, orderSide, price, quantity, id);
            
            for (auto& u : session.getOrderBooks()) {
                cout << u->Size() << endl;
            }

            res.set_content("Order placed successfully!", "text/plain");
        } catch (const exception& e) {
            res.set_content("Error placing order: " + string(e.what()), "text/plain");
        }
    });

    // Display the orderbook
    svr.Get("/orderbook", [&](const Request& req, Response& res) {
        try {
            // Retrieve the number of order books
            size_t numOrderBooks = session.getNumOrderBooks();
            string html;


            // Start building the HTML with navigation
            html += R"(
                <!DOCTYPE html>
                <html>
                <head>
                    <title>Order Books</title>
                    <style>
                        body { font-family: Arial, sans-serif; margin: 20px; }
                        table { border-collapse: collapse; width: 100%; margin-bottom: 20px; }
                        th, td { border: 1px solid #ddd; padding: 8px; text-align: left; }
                        th { background-color: #f4f4f4; }
                        .tab { margin: 10px 0; cursor: pointer; display: inline-block; padding: 10px 15px; border: 1px solid #ddd; background-color: #f4f4f4; }
                        .tab.active { background-color: #ddd; font-weight: bold; }
                        .orderbook { display: none; }
                        .orderbook.active { display: block; }
                    </style>
                    <script>
                        function showOrderbook(index) {
                            let orderbooks = document.querySelectorAll('.orderbook');
                            let tabs = document.querySelectorAll('.tab');
                            orderbooks.forEach((book, i) => {
                                book.style.display = i === index ? 'block' : 'none';
                                tabs[i].classList.toggle('active', i === index);
                            });
                        }
                    </script>
                </head>
                <body>
                    <h1>Order Books</h1>
            )";

            // Add tabs for navigation
            for (int i = 0; i < numOrderBooks; ++i) {
                html += "<div class='tab' onclick='showOrderbook(" + to_string(i) + ")'>Order Book " + to_string(i + 1) + "</div>";
            }

            // Generate order book tables
            for (int i = 0; i < numOrderBooks; ++i) {
                auto& orderBook = session.getOrderBook(i); // Access the Orderbook using the updated Exchange class
                OrderbookLevelInfos orderBookInformation = orderBook.GetOrderInfos();

                html += "<div class='orderbook' id='orderbook-" + to_string(i) + "'" + (i == 0 ? " style='display:block;'" : "") + ">";
                html += "<h2>Order Book " + to_string(i + 1) + "</h2>";
                html += "<table><tr><th>Bids</th><th>Asks</th></tr><tr>";
                
                // Add bids
                html += "<td><ul>";
                for (const auto& bid : orderBookInformation.GetBids()) {
                    html += "<li>Price: " + to_string(bid.price_) + " Quantity: " + to_string(bid.quantity_) + "</li>";
                }
                html += "</ul></td>";
                
                // Add asks
                html += "<td><ul>";
                for (const auto& ask : orderBookInformation.GetAsks()) {
                    html += "<li>Price: " + to_string(ask.price_) + " Quantity: " + to_string(ask.quantity_) + "</li>";
                }
                html += "</ul></td>";
                
                html += "</tr></table></div>";
            }

            // Add form for calculating PnL
            html += R"(
                <form action="/calcPnL" method="post" style="margin-top: 20px;">
                    <button type="submit">Calculate Traders PnL</button>
                </form>
            )";

            // End HTML
            html += R"(
                </body>
                </html>
            )";

            // Set content
            res.set_content(html, "text/html");
        } catch (const std::exception& e) {
            res.set_content("Error: " + string(e.what()), "text/plain");
        }
    });

    // Add route for calculating PnL
    svr.Post("/calcPnL", [&](const Request& req, Response& res) {
        session.calculateTradersPnl();
        res.set_content("PnL calculation complete.", "text/plain");
    });

    // Display current users

    svr.Get("/users", [&](const Request& req, Response& res) {
        string response = "<h2>Current Users:</h2><ul>";
        for (auto& u : users) {
            response += "<li>" + u + "</li>";
        }
        response += "</ul><a href=\"/\">Go Back</a>";
        res.set_content(response, "text/html");
    });
    cout << "Server started at http://localhost:8080" << endl;
    svr.listen("0.0.0.0", 8080);
    return 0;
}