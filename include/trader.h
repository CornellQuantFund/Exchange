#include <string>
#include <vector>
#include "order.h"

using namespace std;

class Trader {
    int id;
    string name;
    double pnl;
    vector<Order> orders;

public:
    Trader(int id, string name, double pnl, vector<Order> orders) : id(id), name(name), pnl(pnl), orders(orders) {};
    Trader(int id, string name, double pnl) : id(id), name(name), pnl(pnl), orders({}) {};
    Trader(int id, string name) : id(id), name(name), pnl(0), orders({}) {};

    double getPnl() { return pnl; }
    string getName() { return name; }
    int getId() { return id; }

    void placeOrder(Order order);

};