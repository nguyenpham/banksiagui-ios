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

final class UserData: ObservableObject {
  @Published var showAnalysis = true
  @Published var showAnalysisArrows = true
  @Published var showBookMoves = true
  @Published var showCoordinates = true
  @Published var showLegalMoves = true
  @Published var showAnalysisMove = true

  @Published var soundMode = 1
  @Published var speechRate = 5
  @Published var speechName = "Karen"

  @Published var pieceStyle = PieceStyle.alpha
  @Published var cellStyle = CellStyle.green

  @Published var userName = ""

  @Published var moveList_showComment = true
  
  init() {
    read()
  }

  func read() {
    let defaults = UserDefaults.standard
    if defaults.string(forKey: "written") == nil {
      return
    }

    showAnalysis = defaults.bool(forKey: "showAnalysis")
    showAnalysisArrows = defaults.bool(forKey: "showAnalysisArrows")
    showBookMoves = defaults.bool(forKey: "showBookMoves")
    showCoordinates = defaults.bool(forKey: "showCoordinates")
    showLegalMoves = defaults.bool(forKey: "showLegalMoves")
    showAnalysisMove = defaults.bool(forKey: "showAnalysisMove")
    soundMode = defaults.integer(forKey: "soundMode")

    pieceStyle = PieceStyle(rawValue: defaults.integer(forKey: "pieceStyle")) ?? PieceStyle.silver
    cellStyle = CellStyle(rawValue: defaults.integer(forKey: "cellStyle")) ?? CellStyle.green

    userName = defaults.string(forKey: "userName") ?? ""

    moveList_showComment = defaults.bool(forKey: "showComment")
  }
  
  func write() {
    let defaults = UserDefaults.standard
    
    defaults.set(showAnalysis, forKey: "showAnalysis")
    defaults.set(showAnalysisArrows, forKey: "showAnalysisArrows")
    defaults.set(showBookMoves, forKey: "showBookMoves")
    defaults.set(showCoordinates, forKey: "showCoordinates")
    defaults.set(showLegalMoves, forKey: "showLegalMoves")
    defaults.set(showAnalysisMove, forKey: "showAnalysisMove")

    defaults.set(soundMode, forKey: "soundMode")

    defaults.set(pieceStyle.rawValue, forKey: "pieceStyle")
    defaults.set(cellStyle.rawValue, forKey: "cellStyle")

    defaults.set(userName, forKey: "userName")

    defaults.set(moveList_showComment, forKey: "showComment")

    defaults.set("written", forKey: "written")
  }
}
