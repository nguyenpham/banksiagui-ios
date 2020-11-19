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

import Foundation

enum ChessVariant : Int {
  case standard, chess960, none
}

enum PieceTypeStd : Int {
  case empty, king, queen, rook, bishop, knight, pawn
  
  private static let allNames = ["empty", "king", "queen", "rook", "bishop", "knight", "pawn" ]
  private static let allChars = [ ".", "k", "q", "r", "b", "n", "p" ]  // ".kqrbnp"
  
  func getName() -> String{
    return PieceTypeStd.allNames[ self.rawValue ]
  }
  
  func getCharactor() -> String {
    return PieceTypeStd.allChars[self.rawValue]
  }

  static func charactor2PieceType(str: String) -> PieceTypeStd {
    let k = PieceTypeStd.allChars.firstIndex(of: str.lowercased())
    let t: Int = PieceTypeStd.allChars.distance(from: PieceTypeStd.allChars.startIndex, to: k ?? 0)
    return PieceTypeStd(rawValue: t) ?? PieceTypeStd.empty
  }
}


enum Side : Int {
  case black = 0
  case white = 1
  case none = 2
  
  private static let allNames = ["Black", "White", "None"]
  private static let allShortNames = ["B", "W", "N"]

  func getValue() -> Int {
    return self.rawValue
  }
  
  func getName() -> String {
    return Side.allNames[self.rawValue]
  }
  
  func getShortName() -> String {
    return Side.allShortNames[self.rawValue]
  }
  
  static func getXSide(side: Side) -> Side {
    return side == white ? black : side == black ? white : none
  }
  
  func getXSide() -> Side {
    return Side.getXSide(side: self)
  }
  
  static func getSide(name: String?) -> Side {
    return name == nil ? none : name == Side.allNames[white.rawValue] ? white : name == Side.allNames[black.rawValue] ? black : none
  }
  
  static func getSide(val: Int) -> Side {
    return val == white.rawValue ? white : val == black.rawValue ? black : none
  }
}

enum ResultType: Int {
    case noresult, win, draw, loss
};

enum  ReasonType : Int {
       case noreason,
       mate,
       stalemate,
       repetition,
       resign,
       fiftymoves,
       insufficientmaterial,
       illegalmove,
       timeout,
       adjudication_length,
       adjudication_egtb,
       adjudication_score,
       adjudication_manual,
       perpetualchase,
       bothperpetualchase,
       extracomment,
       crash,
       abort
   };

enum Turn : Int, CaseIterable {
  case human_white, human_black, human_both
//       , computer_both
  
  private static let allNames = ["Human white, computer black", "Human black, computer white", "Human both"
//                                 , "Computer both"
  ]
  
  func getName() -> String {
    return Turn.allNames[ self.rawValue ]
  }
}

enum TimeControlMode : Int, CaseIterable {
  case standard, depth, nodes, movetime, infinite
  
  private static let allNames = ["Standard", "Depth", "Nodes", "Move time", "Infinite" ]
  
  func getName() -> String {
    return TimeControlMode.allNames[self.rawValue]
  }
}

enum EvaluationMode : Int, CaseIterable {
  case classic, hybrid, nnue
  
  private static let allNames = ["Classic", "Hybrid", "Pure NNUE" ]
  
  func getName() -> String {
    return EvaluationMode.allNames[self.rawValue]
  }
}

enum SoundMode : Int, CaseIterable {
  case off, simple, speech
  
  private static let allNames = ["Off", "Simple", "Speech" ]
  
  func getName() -> String {
    return SoundMode.allNames[self.rawValue]
  }
}

enum CopyMode : Int, CaseIterable {
  case fen, pgn, img
  
  private static let allNames = ["FEN", "PGN", "Image" ]
  
  func getName() -> String {
    return CopyMode.allNames[self.rawValue]
  }
}

