## John Conway's Game of Life

### Version 2.0 description:

#### The edges of the universe wrap around, so the top is connected to the bottom, and the right is connected to the left.

Simple implementation of multithreaded programm. Main thread calls another thread which deals with game logic and then waits for users input.
Controls available during the game:

* C - change colour of alive cells;
* V - change colour of dead cells;
* K - pause the game;
* R - restart current game or choose another pattern;
* X - quit the game.

To set game speed just type desired value in milliseconds during execution.
