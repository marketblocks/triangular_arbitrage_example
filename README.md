# Triangular Arbitrage Example

In this example:
  1. All possible triangular sequences are determined from the available trading pairs for each exchange.
  2. A websocket connection is opened and order book updates are listened to in real-time, allowing us to act on small price fluctuations that may only last fractions of a      second.
  3. When an order book change is detected, the triangular sequences containing the updated asset pair are analyzed to look for triangular arbitrage opportunities.

![image](https://user-images.githubusercontent.com/43093246/169861776-ec61c273-792b-453c-88af-0e10d24df07c.png)
