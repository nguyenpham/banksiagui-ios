# Xiphos

### Overview

Xiphos is a UCI chess engine. The engine employs the modern search techniques and supports multithreading. The latest release, including the binaries for various platforms, can be found [here](https://github.com/milostatarevic/xiphos/releases).

### Acknowledgements

This project is inspired by Garry Kasparov's "[Deep Thinking: Where Machine Intelligence Ends and Human Creativity Begins](http://www.kasparov.com/deep-thinking-ai)." This fantastic book has evoked my childhood passion, computer chess. I was wondering how hard it could be to develop an engine strong enough to surpass the legendary Deep Blue, so I gave it a try.

I would like to thank the authors and the community involved in the creation of the open source projects listed below. Their work greatly inspired me, and without them, this project wouldn't exist.

* [Chess Programming Wiki](https://www.chessprogramming.org/)
* [Crafty](http://www.craftychess.com/)
* [Demolito](https://github.com/lucasart/Demolito/)
* [Ethereal](https://github.com/AndyGrant/Ethereal/)
* [Fathom](https://github.com/jdart1/Fathom/)
* [Hakkapeliitta](https://github.com/mAarnos/Hakkapeliitta/)
* [Laser](https://github.com/jeffreyan11/laser-chess-engine/)
* [Micro-Max](http://home.hccnet.nl/h.g.muller/max-src2.html)
* [Senpai](https://www.chessprogramming.net/senpai/)
* [Stockfish](https://github.com/official-stockfish/Stockfish/)
* [Sunfish](https://github.com/thomasahle/sunfish)
* [Syzygy](https://github.com/syzygy1/tb)


Modified by BanksiaGUI team:
- main.c -> init
- uci.c:
divide uci_loop into two function
convert some char* into const char*

bitboard.h and position.h
add #define _NOPOPCNT


uci.c & search.c
-> use engine_message_c to print out computing info
