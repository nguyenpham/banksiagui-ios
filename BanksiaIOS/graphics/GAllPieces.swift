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

struct GAllPieces: View {
    let cellwidth: CGFloat
    @Binding var chessBoard: ChessBoard
    let pieceStyle: PieceStyle
    let flip: Bool
    let editing: Bool
    let humanSide: Side
    let make: ((_ piece: Piece, _ from: Int, _ dest: Int, _ promotion: Int, _ task: MoveTask) -> Bool)
    let tap: ((_ pos: Int) -> Void)
    let startDragging: ((_ pos: Int) -> Void)
    let arrowFrom: Int
    let arrowDest: Int
    let aniFrom: Int
    let aniDest: Int
    let aniPromotion: Int
    let aniTask: MoveTask
    let selectedPos: Int
    let legalSet: Set<Int>
    
    var body: some View {
        return createView()
    }
    
    func createView() -> some View {
        ZStack {
            ForEach (chessBoard.getAllTPieces(flip: flip, aniFrom: aniFrom, legalSet: legalSet), id: \.name) { piece in
                GPiece(piece: piece,
                       pos: piece.pos,
                       cellwidth: self.cellwidth,
                       pieceStyle: self.pieceStyle,
                       flip: self.flip,
                       editing: self.editing,
                       humanSide: self.humanSide,
                       make: { (piece, from, dest, promotion, moveTask) -> Bool in return make(piece, from, dest, promotion, moveTask) },
                       tap: { pos in self.tap(pos) },
                       startDragging: { pos in self.startDragging(pos) },
                       isArrowEnd: piece.pos == self.arrowFrom,
                       aniFrom: self.aniFrom, aniDest: self.aniDest, aniPromotion: self.aniPromotion, aniTask: self.aniTask,
                       selectedPos: self.selectedPos,
                       showLegalMark: !editing && self.legalSet.contains(piece.pos)
                )
            }
            
            if editing {
                ForEach(0 ..< 2, id: \.self) { sd in
                    ForEach(1 ..< 7, id: \.self) { i in
                        GPiece(piece: Piece(type: i, side: Side(rawValue: sd)!),
                               pos: 64 + i + 8 * sd,
                               cellwidth: self.cellwidth,
                               pieceStyle: self.pieceStyle,
                               flip: self.flip,
                               editing: self.editing,
                               humanSide: self.humanSide,
                               make: { (piece, from, dest, promotion, moveTask) -> Bool in return self.make(piece, from, dest, promotion, moveTask) },
                               tap: { pos in self.tap(pos) },
                               startDragging: { pos in self.startDragging(pos) },
                               isArrowEnd: false,
                               aniFrom: -1, aniDest: -1, aniPromotion: -1, aniTask: MoveTask.make, selectedPos: -1, showLegalMark: false
                               
                        )
                    }
                }
            } else {
                if self.arrowFrom >= 0 && self.arrowDest >= 0 {
                    ArrowView(cellWidth: cellwidth, from: arrowFrom, dest: arrowDest, flip: flip)
                        .zIndex(5)
                }
            }
        }
        
    }
}

//struct GAllPieces_Previews: PreviewProvider {
//  static var previews: some View {
//    GAllPieces(cellwidth: 20, chessBoard: ChessBoard(), pieceStyle: .silver)
//  }
//}
