#include <string>
#include <vector>
#include <iostream>
#include "../extras/Order.h"
#include "../inja/inja.hpp" 

using namespace std;
using json = nlohmann::json;
static int id_counter = 0;

class Trader {
    int id;
    string name;
    double pnl;
    vector<OrderPointer> orders;

public:
    Trader(string name, double pnl, vector<OrderPointer> orders) : id(id_counter), name(name), pnl(pnl), orders(orders) {}
    Trader(string name, double pnl) : id(id_counter), name(name), pnl(pnl), orders({}) {}
    Trader(string name) : id(id_counter), name(name), pnl(0), orders({}) {}
    Trader() : id(id_counter++), name(""), pnl(0), orders({}) {} // Default constructor

    double getPnl() { return pnl; }
    void setPnl(double pnl) { this->pnl = pnl; }
    void addProfit(double profit) { pnl += profit; }
    string getName() { return name; }
    int getId() { return id; }

    void addOrderToTrader(OrderPointer order) {
        orders.push_back(order);
    }

    vector<OrderPointer>& getOrders() { return orders; }

    string getOrdersFormatted() {
        string formatted = "";
        for (auto& order : orders) {
            if (order->GetRemainingQuantity() == 0) continue;

            formatted += "Order ID: " + to_string(order->GetOrderId()) + " Side: " + (order->GetSide() == Side::Buy ? "Buy" : "Sell") + " Price: " + to_string(order->GetPrice()) + " Quantity: " + to_string(order->GetRemainingQuantity()) + "\n";
        }
        return formatted;
    }

    json getOrdersJson() {
        json ordersJson = json::array();
        for (auto& order : orders) {
            if (order->GetRemainingQuantity() == 0) continue;

            json orderJson;
            orderJson["orderId"] = order->GetOrderId();
            orderJson["contract"] = order->GetContract();
            orderJson["side"] = order->GetSide() == Side::Buy ? "Buy" : "Sell";
            orderJson["price"] = order->GetPrice();
            orderJson["quantity"] = order->GetRemainingQuantity();
            ordersJson.push_back(orderJson);
        }
        return ordersJson;
    }
};