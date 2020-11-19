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

class Piece {
  static let EMPTY: Int = PieceTypeStd.empty.rawValue;
  static let KING: Int = PieceTypeStd.king.rawValue;
  
  static let emptyPiece = Piece(type: Piece.EMPTY, side: Side.none)

  var type: Int;
  var side: Side;

  init(type: Int, side: Side) {
    self.type = type
    self.side = side
  }
  init(piece: Piece) {
    self.type = piece.type
    self.side = piece.side
  }

  func set(type: Int, side: Side) {
    self.type = type
    self.side = side
  }
  
  func setEmpty() {
    set(type: Piece.EMPTY, side: Side.none)
  }

  func isEmpty() -> Bool {
      return type == Piece.EMPTY;
  }

  func isPiece(type: Int, side: Side) -> Bool {
    return self.type == type && self.side == side;
  }

  func isValid() -> Bool {
    return (side == Side.none && type == Piece.EMPTY) || (side != Side.none && type != Piece.EMPTY);
  }

//  bool operator == (const Piece & o) const {
//      return type == o.type && side == o.side;
//  }
//  bool operator != (const Piece & o) const {
//      return type != o.type || side != o.side;
//  }

}

class TPiece : Piece
{
  let pos: Int
  let flip: Bool
  let name: String  // for id
  let hasAni: Bool
  init(piece: Piece, pos: Int, flip: Bool, hasAni: Bool, showLegalMark: Bool) {
    self.pos = pos
    self.flip = flip
    self.hasAni = hasAni
    
    let p = flip ? 63 - pos : pos
    self.name = "\(piece.type)\(piece.side)\(p)-\(hasAni)\(showLegalMark)"

    super.init(piece: piece)
  }
}
