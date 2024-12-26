#include "../include/trader.h"
#include <iostream>

using namespace std;

void Trader::placeOrder(Order order) {
    orders.push_back(order);
    // cout << "Order placed by " << name << ": " << order.getSymbol() << " " << order.getQuantity() << " " << order.getPrice() << endl;
}