/*
  Banksia GUI, a chess GUI for iOS
  Copyright (C) 2020 Nguyen Hong Pham

  Banksia GUI is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Banksia GUI is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

import SwiftUI

let version_lc0 = "0.30"
let version_stockfish = "16"

#if os(watchOS)
let networkNames = [
    "nn-5af11540bbfe.nnue",         // Stockfish
    "703810.pb.gz",                 // Lc0: old, small net 6.4 MB, for Watch
    "nn-c257b2ebf1-20230812.nnue",  // RubiChess
]

#else
let networkNames = [
    "nn-5af11540bbfe.nnue",         // Stockfish
    "791556.pb.gz",                 // Lc0
    "nn-c257b2ebf1-20230812.nnue",  // RubiChess
]

#endif

struct EngineInfo : Hashable {
    let name: String
    let shortName: String /// 2 charactors only, use for displaying on clocks in watchOS
    let version: String
    let author: String
    let frc: Bool /// Fischer random chess
    let idNumb: Int
    let elo: Int
    let nnue: Bool
    let bench: Bool
}

let stockfishEngineInfo  = EngineInfo(name: "Stockfish", shortName: "Sf", version: version_stockfish, author: "Stockfish team",
                                      frc: true, idNumb: stockfish, elo: 3500, nnue: true, bench: true)
let lc0EngineInfo  = EngineInfo(name: "Lc0", shortName: "Lc", version: version_lc0, author: "LeelaChessZero team",
                                      frc: true, idNumb: lc0, elo: 3500, nnue: false, bench: true)

let rubichessEngineInfo = EngineInfo(name: "RubiChess", shortName: "Rb", version: "2.2 (20230918)", author: "Andreas Matthies",
                                     frc: true, idNumb: rubi, elo: 3296, nnue: true, bench: true)

final class EngineData: ObservableObject {
    
    let allEngines = [
        stockfishEngineInfo,
        lc0EngineInfo,
        rubichessEngineInfo,
    ]
}
