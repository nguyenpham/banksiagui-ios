# Banksia GUI for iOS


## What is Banksia, Banksia GUI and Banksia GUI for iOS?
- Banksia is an open-source chess interface, running in console mode (cli), released at https://github.com/nguyenpham/Banksia
- Banksia GUI is a chess Graphical User Interface for desktop computers (Win, Mac, Linux), helps users to interact visually with chess engines. It is built based on Banksia. The home page is at https://banksiagui.com/
- Banksia GUI for iOS is a light-open-source version of Banksia GUI to run on iOS devices (iPhone, iPad, Apple Watch). The source code released at https://github.com/nguyenpham/banksiagui-ios, the app released at https://apps.apple.com/us/app/banksiagui/id1534720619


## Screenshots
- iPhone
![iPhoneDemo](bsg.png | width=600)

- Apple watch
![AppleWatchDemo](bsgwatch.png | width=600)
![AppleWatchDemo2](watch.jpg | width=400)


## Tech
The project is used the latest tech to develop:

- Switf + SwitfUI


## Engines
Typically a chess engine can’t work independently (as an independent app) under iOS system. Thus it must be integrated to a chess GUI (BanksiaGUI) by coding, both the chess GUI and that engines code must be changed to. 

Some chess engines are integrated and can run well under Banksia GUI:
- Stockfish NNUE 13dev
- LeelaChessZero
- Ethereal
- Xiphos
- Rubichess
- Laser
- Defenchess
- Igel


## Release on AppStore
We planned to publish BanksiaGUI as several apps, based on which engines released with. For example, Lc0 would be Lc0 engine + BanksiaGUI. However, AppStore did not allow to release some apps with quite similar functions. Thus we have published only Banksia GUI app with all available engines at:
https://github.com/nguyenpham/banksiagui-ios

It is totally freeware.


## Add new chess engines 
You are welcome to add new engines to Banksia GUI for iOS. The conditions to add a new engine:
- it is open source with free lincess (such as GPL/MIT, free or similar) or
- you are the authors or 
- you got writen permission from authors
- it is written with C/C++ and can be compiled with C++17 compiler/libraries
- it should be UCI protocol or you need to complete some extra work

The integration process glancely as the following:
- write a function to initiate chess. For example, the function stockfish_init
- remove the main loop
- Create a function to process UCI commands as strings
- Instead of printing out info, bestmoves to screen, call a function from BSG library
- Compile BSG with new engine code, solve all problems such as code folders, compiling/linking flags, conflicts between other engines’ code and the new one. For example, it may conflict if it uses some Fathom library code
- Modify BSG code thus it knows there is a new engine and how to call

- Reduce compiling warnings

You may see the code of some engines come with BSG as examples.


## You can help
If you download and use BanksiaGUI app on AppStore you already help me. You may contribute to this project and/or donate (donate at https://banksiagui.com/download/) too.  All bug reports, suggestions, contributions, donations are more than welcome. 


## License/Credits

Banksia GUI for iOS was written by Nguyen Pham (axchess at yahoo dot com). Banksia GUI for iOS is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

All integrated chess engines are open-source with GNU license or similar:
- Stockfish, version: 13Dev, authosr: Stockfish team, link: https://github.com/official-stockfish/Stockfish
- LeelaChessZero, version:  0.26.3, authors: LeelaChessZero team, link: https://github.com/LeelaChessZero/lc0
- Ethereal, version: 12.50, authors: Andrew Grant, link: https://github.com/AndyGrant/Ethereal
- Xiphos, version: 0.5, authors: Milos Tatarevic, link: https://github.com/milostatarevic/xiphos
- Defenchess, version: 2.2, authors: Can Cetin, Dogac Eldenk, link: https://github.com/cetincan0/Defenchess
- RubiChess, version: 1.8, authors: Andreas Matthies, link: https://github.com/Matthies/RubiChess
- Laser, version: 1.6, authosr: Jeffrey An, Michael An, link: https://github.com/jeffreyan11/laser-chess-engine
- Igel, version: 2.6, authors: Medvedev, Shcherbyna, link: https://github.com/vshcherbyna/igel


## History
- 2020 Nov 20: version 1.0 offical first release
