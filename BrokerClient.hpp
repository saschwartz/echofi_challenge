/**
 * @file brokerClient.hpp
 *
 * Header file describing an interface called BrokerClient, for buying and
 * selling securities as well as querying positions and orders.
 */

#include <map>
#include <queue>
#include <string>
#include <vector>

/**
 * Enumeration describing the different varieties of order that may be placed.
 */
enum OrderKind {
  /// Order is for the purchase of a security.
  Buy,

  /// Order is for the sale of a security.
  Sell,
};

/**
 * Struct representing a holding of a certain security.
 */
typedef struct {
  /// String representing the ticker name of the security.
  std::string name;

  /// Quantity of shares of the given security in this position.
  uint32_t quantity;

  /// Price of the given security.
  double price;
} SecurityPosition;

/**
 * Struct representing an order that may be placed using the BrokerClient
 * interface.
 */
typedef struct {
  /// Type of the order.
  OrderKind kind;

  /// Represents the security to buy or sell.
  SecurityPosition position;
} Order;

/**
 * @class BrokerClient
 *
 * This class describes an interface that can be used to buy and sell securities
 * (in whole quantities only) as well as retrieve transaction history, and
 * current portfolio position.
 *
 * The interface manages a cash balance which cannot be overdrawn.
 */
class BrokerClient {
public:
  /**
   * Constructor for the BrokerClient.
   *
   * @param[in] cashBalance
   *    The initial amount of cash that the client will be instantiated with.
   */
  BrokerClient(double cashBalance);

  /**
   * Submit an order to buy or sell a given security. Returns the number
   * of shares that were actually bought or sold.
   *
   * @note
   *    This method will attempt to process orders partially, even if the
   *    total order would exceed allowable parameters. For example, if a
   *    sell order is placed for a security that the client does not own the
   *    requisite quantity of, only as many shares as the client owns will
   *    be sold. Similarly for buy orders, as many shares will be bought as
   *    do not exceed the client's cash balance.
   *
   * @note
   *    Only whole quantities of securities may be bought or sold.
   *
   * @param[in] order
   *    An Order object representing the necessary details to process the
   *    transaction.
   *
   * @retval
   *    The number of shares that were bought or sold as part of the order.
   */
  uint32_t SubmitOrder(Order order);

  /**
   * Get the current outstanding positions of the client, i.e.
   * a representation of all shares owned by the client.
   *
   * @note
   *    The price represented in the returned positions reflects the average
   *    purchase price of the security, across all buy orders that the
   *    client placed.
   *
   * @retval
   *    A vector of SecurityPosition objects representing the client's
   *    current portfolio.
   */
  std::vector<SecurityPosition> GetPositions();

  /**
   * Get a list of orders that the client submitted and that were
   * successfully processed.
   *
   * @note
   *    This list reflects the transactions that actually happened, not
   *    necessarily the transactions the client requested. For example,
   *    if a client tries to sell shares they don't own, the transaction
   *    will not appear in this list (or a partial transaction may appear
   *    if they tried to sell more shares than they owned). See the comment
   *    in SubmitOrder for more details.
   *
   * @retval
   *    A vector of Order objects representing transactions processed by
   *    this interface on behalf of the client.
   */
  std::vector<Order> GetTransactions() { return transactions_; };

  /**
   * Get the client's current cash balance.
   *
   * @retval
   *    The client's current cash balance.
   */
  double GetCashBalance() { return cashBalance_; }

private:
  /// Representation of the current balance of the client's cash holdings.
  double cashBalance_;

  /**
   * Stores the current portfolio managed by the client. Prices in this
   * portfolio reflect the average purchase price across all buy orders.
   *
   * The portfolio is represented as a map keyed by the security name, which
   * is assumed to be globally unique across all securities.
   */
  std::map<std::string, SecurityPosition> portfolio_;

  /**
   * Stores all the processed transactions of securities, in order of
   * processing.
   */
  std::vector<Order> transactions_;

  /**
   * Stores a map of security name to the latest buy orders for that security,
   * that have not yet had their contents sold. This is necessary for
   * calculating the weighted average of a security's price after a sale.
   *
   * Buy orders are inserted and removed (when the security is sold) on a
   * FIFO basis.
   */
  std::map<std::string, std::queue<Order>> outstandingBuyOrderMap_;

  /**
   * Handles a buy order, updating internal state (including portfolio
   * status and transaction history).
   *
   * @note
   *    This method expects that the order has already been validated
   *    (i.e. that it will not overdraw the cash balance).
   *
   * @param[in] order
   *    New buy order from which to update internal state.
   */
  void HandleBuy(Order order);

  /**
   * Handles a sell order, updating internal state (including portfolio
   * status and transaction history).
   *
   * @note
   *    This method expects that the order has already been validated (i.e.
   *    that we will not sell shares that we don't have).
   *
   * @param[in] order
   *    New sell order from which to update internal state.
   */
  void HandleSell(Order order);
};
