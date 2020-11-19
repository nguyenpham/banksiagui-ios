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


class Move {
  var from: Int, dest: Int, promotion: Int
  static let illegalMove = Move(from: -1, dest: -1, promotion: -1)
  
  init(move: Move) {
    from = move.from
    dest = move.dest
    promotion = move.promotion
  }

  init(from: Int, dest: Int, promotion: Int = Piece.EMPTY) {
    self.from = from
    self.dest = dest
    self.promotion = promotion
  }
  
  class func isValidPromotion(promotion: Int) -> Bool {
    return promotion == Piece.EMPTY || (promotion > PieceTypeStd.king.rawValue && promotion < PieceTypeStd.pawn.rawValue);
  }
  
  func isValid() -> Bool  {
    return Move.isValid(from: from, dest: dest)
  }
  
  static func isValid(from: Int, dest: Int) -> Bool {
    return from != dest  && from >= 0 && dest >= 0
  }
  
  static func == (first: Move, second: Move) -> Bool {
    return first.from == second.from && first.dest == second.dest && first.promotion == second.promotion;
  }
}


class MoveFull : Move {
  var piece: Piece
  var score: Int = 0;
  
  static let illegalMoveFull = MoveFull(from: -1, dest: -1, promotion: Piece.EMPTY)
  
  init(move: MoveFull) {
    piece = Piece(piece: move.piece)
    score = move.score
    super.init(move: move)
  }
  
  init(piece: Piece, from: Int = -1, dest: Int = -1, promotion: Int = Piece.EMPTY)
  {
    self.piece = piece
    super.init(from: from, dest: dest, promotion: promotion)
  }
  
  override init(from: Int, dest: Int, promotion: Int = Piece.EMPTY)
  {
    self.piece = Piece.emptyPiece
    super.init(from: from, dest: dest, promotion: promotion)
  }
  
  func set(piece: Piece, from: Int, dest: Int, promotion: Int = Piece.EMPTY) {
    self.piece = piece
    self.from = from
    self.dest = dest
    self.promotion = promotion
  }
  
  func set(from: Int, dest: Int, promotion: Int) {
    self.piece = Piece.emptyPiece
    self.from = from
    self.dest = dest
    self.promotion = promotion
  }
};
