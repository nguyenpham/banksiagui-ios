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

let version_lc0 = "0.26.3"

// Warning: watchOS limit the app size only 75MB, the new net makes the app over that limit (108MB)
// we must use the older net to reduce the size
//#if os(watchOS)
//let version_stockfish = "13 dev"
//let network_nnue = "nn-03744f8d56d8.nnue"
//#else
//let version_stockfish = "14 dev"
//let network_nnue = "nn-9e3c6298299a.nnue"
//#endif

let version_stockfish = "16"
let network_sf = "nn-5af11540bbfe.nnue"

let network_rubi = "nn-c257b2ebf1-20230812.nnue"

let network_lc0 = "703810.pb.gz"

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

let etherealEngineInfo = EngineInfo(name: "Ethereal", shortName: "Et", version: "12.50", author: "Andrew Grant",
                                    frc: true, idNumb: ethereal, elo: 3365, nnue: false, bench: false)
let xiphosEngineInfo = EngineInfo(name: "Xiphos", shortName: "Xp", version: "0.5", author: "Milos Tatarevic",
                                  frc: true, idNumb: xiphos, elo: 3339, nnue: false, bench: false)
let defenchessEngineInfo = EngineInfo(name: "Defenchess", shortName: "Dc", version: "2.2", author: "Can Cetin, Dogac Eldenk",
                                      frc: true, idNumb: defenchess, elo: 3280, nnue: false, bench: false)


let rubichessEngineInfo = EngineInfo(name: "RubiChess", shortName: "Rb", version: "2.2 (20230918)", author: "Andreas Matthies",
                                     frc: true, idNumb: rubi, elo: 3296, nnue: true, bench: false)
let laserEngineInfo = EngineInfo(name: "Laser", shortName: "Ls", version: "1.6", author: "Jeffrey An, Michael An",
                                 frc: true, idNumb: laser, elo: 3288, nnue: false, bench: false)

//let igelEngineInfo = EngineInfo(name: "Igel", shortName: "Ig", version: "2.6", author: "Medvedev, Shcherbyna",
//                                frc: true, idNumb: igel, elo: 3240, nnue: true, bench: false)

//let nemorinoEngineInfo = EngineInfo(name: "Nemorino", shortName: "Nm", version: "6", author: "Christian Günther",
//                                    frc: true, idNumb: nemorino, elo: 3149, nnue: true, bench: false)
//let fruitEngineInfo = EngineInfo(name: "Fruit", shortName: "Fr", version: "2.1", author: "Fabien Letouzey",
//                                 frc: true, idNumb: fruit, elo: 2200, nnue: false, bench: false)


final class EngineData: ObservableObject {
  
  //  let allEngines: [EngineInfo]
  #if STOCKFISHONLY
  let allEngines = [
    stockfishEngineInfo
  ]
  #elseif LC0ONLY
  let allEngines = [
    lc0EngineInfo
  ]
  #elseif NNONLY
  let allEngines = [
    stockfishEngineInfo,
    lc0EngineInfo,
//    igelEngineInfo,
//    nemorinoEngineInfo
  ]
  #else
  let allEngines = [
    stockfishEngineInfo,
    lc0EngineInfo,
    etherealEngineInfo,
    xiphosEngineInfo,
    rubichessEngineInfo,
    
    laserEngineInfo,
    defenchessEngineInfo,
//    igelEngineInfo,
//    nemorinoEngineInfo,
//    fruitEngineInfo
  ]
  #endif
  
  init() {
  }
  
}
