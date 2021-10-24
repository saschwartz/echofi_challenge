#include "BrokerClient.hpp"
#include <iostream>

/// Helper function for checking position equality.
static bool positionsEqual(SecurityPosition a, SecurityPosition b) {
  return a.name == b.name && a.price == b.price && a.quantity == b.quantity;
}

/// Helper function for checking order equality.
static bool ordersEqual(Order a, Order b) {
  return a.kind == b.kind && positionsEqual(a.position, b.position);
}

/// Check everything is correct in initial construction.
void testEmpty() {
  std::cout << " * Running test: " << __FUNCTION__ << std::endl;
  BrokerClient client = BrokerClient(10000);
  assert(client.GetPositions().empty());
  assert(client.GetTransactions().empty());
}

/// Try a simple buy order.
void testBuySimple() {
  std::cout << " * Running test: " << __FUNCTION__ << std::endl;
  BrokerClient client = BrokerClient(10000);
  Order order = {
      .kind = Buy,
      .position = {.name = std::string("AAPL"), .quantity = 10, .price = 100}};
  uint32_t bought = client.SubmitOrder(order);
  assert(bought == 10);

  std::vector<SecurityPosition> portfolio = client.GetPositions();
  assert(positionsEqual(portfolio[0], order.position));

  std::vector<Order> orders = client.GetTransactions();
  assert(ordersEqual(orders[0], order));
}

/// Try a buy order that ought to be partially completed.
void testBuyPartial() {
  std::cout << " * Running test: " << __FUNCTION__ << std::endl;
  BrokerClient client = BrokerClient(10000);
  Order order = {
      .kind = Buy,
      .position = {.name = std::string("AAPL"), .quantity = 101, .price = 100}};
  uint32_t bought = client.SubmitOrder(order);
  assert(bought == 100);

  Order expectedOrder = {
      .kind = Buy,
      .position = {.name = std::string("AAPL"), .quantity = 100, .price = 100}};
  std::vector<SecurityPosition> portfolio = client.GetPositions();
  assert(positionsEqual(portfolio[0], expectedOrder.position));
  std::vector<Order> orders = client.GetTransactions();
  assert(ordersEqual(orders[0], expectedOrder));
  assert(client.GetCashBalance() == 0);
}

/// Check we correctly average the price across multiple buys and sells.
void testBuySellMultipleAvgPrice() {
  std::cout << " * Running test: " << __FUNCTION__ << std::endl;
  uint32_t transacted;
  std::vector<SecurityPosition> portfolio;

  BrokerClient client = BrokerClient(10000);

  Order order1 = {
      .kind = Buy,
      .position = {.name = std::string("AAPL"), .quantity = 10, .price = 10}};
  transacted = client.SubmitOrder(order1);
  assert(transacted == 10);

  Order order2 = {
      .kind = Buy,
      .position = {.name = std::string("AAPL"), .quantity = 10, .price = 40}};
  transacted = client.SubmitOrder(order2);
  assert(transacted == 10);
  portfolio = client.GetPositions();
  assert(portfolio[0].price == 25);
  assert(portfolio[0].quantity == 20);

  Order order3 = {
      .kind = Sell,
      .position = {.name = std::string("AAPL"), .quantity = 5, .price = 60}};
  transacted = client.SubmitOrder(order3);
  assert(transacted == 5);
  portfolio = client.GetPositions();
  assert(portfolio[0].price == 30);
  assert(portfolio[0].quantity == 15);

  Order order4 = {
      .kind = Sell,
      .position = {.name = std::string("AAPL"), .quantity = 10, .price = 60}};
  transacted = client.SubmitOrder(order4);
  assert(transacted == 10);
  portfolio = client.GetPositions();
  assert(portfolio[0].price == 40);
  assert(portfolio[0].quantity == 5);

  Order order5 = {
      .kind = Buy,
      .position = {.name = std::string("AAPL"), .quantity = 5, .price = 45}};
  transacted = client.SubmitOrder(order5);
  assert(transacted == 5);
  portfolio = client.GetPositions();
  assert(portfolio[0].price == 42.5);
  assert(portfolio[0].quantity == 10);

  std::vector<Order> orders = client.GetTransactions();
  assert(ordersEqual(orders[0], order1));
  assert(ordersEqual(orders[1], order2));
  assert(ordersEqual(orders[2], order3));
  assert(ordersEqual(orders[3], order4));
  assert(ordersEqual(orders[4], order5));
}

/// Try a simple buy and then sell.
void testBuySellSimple() {
  std::cout << " * Running test: " << __FUNCTION__ << std::endl;
  BrokerClient client = BrokerClient(10000);
  Order buyOrder = {
      .kind = Buy,
      .position = {.name = std::string("AAPL"), .quantity = 10, .price = 100}};
  uint32_t bought = client.SubmitOrder(buyOrder);
  assert(bought == 10);
  Order sellOrder = {
      .kind = Sell,
      .position = {.name = std::string("AAPL"), .quantity = 10, .price = 100}};
  uint32_t sold = client.SubmitOrder(sellOrder);
  assert(sold == bought);
  assert(client.GetPositions().empty());
}

/// Check we see an increase / decrease in net cash after buying then selling.
void testBuySellCheckProfit() {
  std::cout << " * Running test: " << __FUNCTION__ << std::endl;
  BrokerClient client = BrokerClient(10000);
  Order buyOrder = {
      .kind = Buy,
      .position = {.name = std::string("AAPL"), .quantity = 10, .price = 100}};
  uint32_t bought = client.SubmitOrder(buyOrder);
  assert(bought == 10);
  Order sellOrder = {
      .kind = Sell,
      .position = {.name = std::string("AAPL"), .quantity = 10, .price = 200}};
  uint32_t sold = client.SubmitOrder(sellOrder);
  assert(sold == bought);
  assert(client.GetCashBalance() == 11000);

  // Shouldn't be in portfolio anymore.
  assert(client.GetPositions().empty());
}

/// Try sell shares we don't have any of.
void testSellNone() {
  std::cout << " * Running test: " << __FUNCTION__ << std::endl;
  BrokerClient client = BrokerClient(10000);
  Order order = {
      .kind = Sell,
      .position = {.name = std::string("AAPL"), .quantity = 10, .price = 100}};
  uint32_t sold = client.SubmitOrder(order);
  assert(sold == 0);
  assert(client.GetPositions().empty());
  assert(client.GetTransactions().empty());
}

int main(void) {
  std::cout << "Running BrokerClientTests" << std::endl;
  testEmpty();
  testBuySimple();
  testBuyPartial();
  testBuySellMultipleAvgPrice();
  testBuySellSimple();
  testBuySellCheckProfit();
  testSellNone();
  std::cout << "All tests passed!" << std::endl;
}
