#pragma once
#include "Order.h"

class OrderModify {
public:
    OrderModify(OrderId orderId, Side side, Price price, Quantity quantity, int contract)
        : orderId_{ orderId }
        , price_{ price }
        , side_{ side }
        , quantity_{ quantity }
        , contract_{ contract }
    { }

    OrderId GetOrderId() const { return orderId_; }
    Price GetPrice() const { return price_; }
    Side GetSide() const { return side_; }
    Quantity GetQuantity() const { return quantity_; }
    int GetContract() const { return contract_; }

    OrderPointer ToOrderPointer(OrderType type) const
    {
        return std::make_shared<Order>(type, GetOrderId(), GetSide(), GetPrice(), GetQuantity(), GetContract());
    }

private:
    OrderId orderId_;
    Price price_;
    Side side_;
    Quantity quantity_;
    int contract_;
};
