#include <string>
#include <vector>
#include <iostream>
#include "../extras/Order.h"

using namespace std;

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
    string getName() { return name; }
    int getId() { return id; }

    void addOrderToTrader(OrderPointer order) {
        orders.push_back(order);
    }
};