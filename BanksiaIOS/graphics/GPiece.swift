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

enum PieceStyle : Int, CaseIterable {
  case alpha, fantasy, wiki, silver, gold, italian
  
  private static let allNames = ["Alpha", "Fantasy", "Wiki", "Royal Silver", "Royal Gold", "Italian" ]
  private static let allPrefix = ["a-", "c-", "wiki-", "silver-", "gold-", "ita-" ]
  
  func getName() -> String {
    return PieceStyle.allNames[self.rawValue]
  }
  func getPrefix() -> String {
    return PieceStyle.allPrefix[ self.rawValue ]
  }
}

enum MoveTask {
  case make, prev, next
}


struct GPiece: View {
  let piece: Piece
  let pos: Int
  let cellwidth: CGFloat
  let pieceStyle: PieceStyle
  let flip: Bool
  let editing: Bool
  let humanSide: Side
  let make: ((_ piece: Piece, _ from: Int, _ dest: Int, _ promotion: Int, _ task: MoveTask) -> Bool)
  let tap: ((_ pos: Int) -> Void)
  let startDragging: ((_ pos: Int) -> Void)
  let isArrowEnd: Bool
  let aniFrom: Int
  let aniDest: Int
  let aniPromotion: Int
  let aniTask: MoveTask
  
  let selectedPos: Int
  let showLegalMark: Bool
  
  var image: Image?

  @State private var position = CGPoint(x: 0, y: 0)
  @State private var isDragging = false
  
  @State private var offsetX: CGFloat = 0.0
  @State private var offsetY: CGFloat = 0.0
  
  @State private var oldcellwidth: CGFloat = 0
  
  let aniMoveDuration: Double = 0.1
  
  func createString() -> String {
    return "\(piece.type)-\(piece.side)-\((flip ? 1 : 0))"
  }
  
  var body: some View {
    ZStack {
      createPieceImage()
      .frame(width: cellwidth, height: cellwidth)
      .shadow(radius: 3)

      if showLegalMark {
        Image("ballgreen")
          .resizable()
          .aspectRatio(contentMode: .fit)
          .frame(width: cellwidth * 0.25, height: cellwidth * 0.25)
      }
    }
      .padding(4)
    .position(isDragging ? position : GPiece.calPosition(pos: pos, flip: flip, cellWidth: cellwidth))
      //      .scaleEffect(isDragging ? 1.5 : 1.0)
      .offset(x: offsetX, y: offsetY)
      .zIndex(self.isDragging ? 10 : self.isArrowEnd ? 8 : 0)
      .onAppear {
        startPosition()
        if !editing && aniFrom >= 0 && pos == aniFrom {
          let destPoint = GPiece.calPosition(pos: aniDest, flip: flip, cellWidth: cellwidth)
          
          withAnimation(.easeInOut(duration: aniMoveDuration)) {
            offsetX = destPoint.x - position.x
            offsetY = destPoint.y - position.y
          }
          DispatchQueue.main.asyncAfter(deadline: .now() + self.aniMoveDuration) {
            _ = make(piece, pos, aniDest, aniPromotion, aniTask);
          }
        }
    }
    .onTapGesture {
      self.tap(self.pos)
    }
    .gesture(DragGesture().onChanged({ value in
      if self.piece.isEmpty() {
        return
      }
      if selectedPos >= 0 && selectedPos != pos {
        if piece.side != humanSide {
          _ = make(piece, selectedPos, pos, Piece.EMPTY, MoveTask.make)
        } else {
          _ = make(piece, pos, -1, Piece.EMPTY, MoveTask.make)
        }
        return
      }

      if !editing {
        if piece.type == Piece.EMPTY || piece.side != humanSide {
          return
        }
      }
      startDragging(pos)
      isDragging = true
      position = value.location
    }).onEnded({ value in
      if !isDragging || piece.isEmpty() || (!editing && (piece.type == Piece.EMPTY || piece.side != humanSide)) {
        return
      }
      let dropPos = self.position2ChessPos()
      print("drop pos: ", dropPos)

      isDragging = false
      if (!editing && dropPos < 0) || !make(piece, pos, dropPos, Piece.EMPTY, MoveTask.make) {
        startPosition()
      }
    }))
  }
  
  func createPieceImage() -> some View {
    let name = piece.isEmpty() ? "e" : "\(pieceStyle.getPrefix())\(piece.side == Side.white ? "w" : "b")\(PieceTypeStd(rawValue: piece.type)!.getCharactor())"
    let image = Image(name)
    return image
    .resizable()
    .aspectRatio(contentMode: .fit)
  }
  
  static func calPosition(pos: Int, flip: Bool, cellWidth: CGFloat) -> CGPoint {
    var p = max(0, pos)
    p = flip ? 63 - p : p
    return CGPoint(x: CGFloat(p % 8) * cellWidth + cellWidth * 0.5,
                   y: CGFloat(p / 8) * cellWidth + cellWidth * 0.5)
  }
  
  func startPosition() {
    position = GPiece.calPosition(pos: self.pos, flip: self.flip, cellWidth: self.cellwidth)
  }
  
  func position2ChessPos() -> Int {
    let x = Int(self.position.x / self.cellwidth)
    let y = Int(self.position.y / self.cellwidth)
    if x < 0 || x > 7 || y < 0 || y > 7 {
      return -1
    }
    let pos = y * 8 + x
    return flip ? 63 - pos : pos
  }
}

struct GPiece_Previews: PreviewProvider {
  static var previews: some View {
    GPiece(piece: Piece(type: PieceTypeStd.king.rawValue, side: Side.white),
           pos: 20,
           cellwidth: 40,
           pieceStyle: .gold,
           flip: true,
           editing: false,
           humanSide: Side.white,
           make: { (_, _, _, _, _) -> Bool in
            return true
    },
           tap: { pos in },
           startDragging: { pos in },
           isArrowEnd: false,
           aniFrom: -1, aniDest: -1, aniPromotion: -1, aniTask: MoveTask.make,
           selectedPos: -1,
           showLegalMark: false
    )
  }
}
