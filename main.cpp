#include <iostream>
#include <vector>
#include <string>
#include "../include/order.h"
#include "httplib.h" // Requires cpp-httplib (https://github.com/yhirose/cpp-httplib)

using namespace std;
using namespace httplib;

static vector<string> users;

int main() {
    Server svr;
    svr.set_mount_point("/static", "./static");

    // Serve the index.html file
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
        if (!username.empty()) {
            users.push_back(username);
        }
        res.set_content("<h2>User added successfully!</h2><a href=\"/trading\">Go to Trading Floor</a><br><a href=\"/users\">View Current Users</a>", "text/html");
    });

    // Serve the trading floor page
    svr.Get("/tradefloor", [&](const Request& req, Response& res) {
        ifstream file("templates/tradeFloor.html");
        if (file) {
            stringstream buffer;
            buffer << file.rdbuf();
            res.set_content(buffer.str(), "text/html");
        } else {
            res.set_content("Error: Could not open tradeFloor.html", "text/plain");
        }
    });

    // Handle order submission
    // svr.Post("/submit_order", [&](const Request& req, Response& res) {
    //     Order order(
    //         req.get_param_value("contract"),
    //         req.get_param_value("order-type"),
    //         req.get_param_value("order-side"),
    //         stod(req.get_param_value("price")),
    //         stoi(req.get_param_value("quantity"))
    //     );
    //     OrderBook.add(order);
    //     Trader.addOrder(order);

    //     res.set_content("Order placed successfully!", "text/plain");
    // });

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