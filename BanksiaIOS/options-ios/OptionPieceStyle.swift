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

struct OptionPieceStyle: View {
  @EnvironmentObject private var userData: UserData
  private let cellwidth: CGFloat = 36

  var body: some View {
    List (PieceStyle.allCases, id:\.self) { pieceStyle in
      HStack {
        Text("\(pieceStyle.getName())")
        Spacer()
        OptionPieceStyle.createPieces(cellwidth: self.cellwidth, pieceStyle: pieceStyle)
        .padding(6)

        if pieceStyle == self.userData.pieceStyle {
            Image(systemName: "checkmark")
          .resizable()
          .frame(width: 20, height: 20)
          .foregroundColor(.green)
          .shadow(radius: 1)
        } else {
          Rectangle()
            .fill(Color.white)
            .frame(width: 20, height: 20)
        }
      }.onTapGesture {
        self.userData.pieceStyle = pieceStyle
      }
    }
    .navigationBarTitle("Piece style", displayMode: .inline)
  }
  
  static func createPieces(cellwidth: CGFloat, pieceStyle: PieceStyle, editing: Bool = false, make: ((_ piece: Piece, _ from: Int, _ dest: Int, _ promotion: Int, _ task: MoveTask) -> Bool)? = nil) -> some View {
    VStack {
      OptionPieceStyle.createPieces(cellwidth: cellwidth, side: Side.white, pieceStyle: pieceStyle, editing: editing, make: make)
      OptionPieceStyle.createPieces(cellwidth: cellwidth, side: Side.black, pieceStyle: pieceStyle, editing: editing, make: make)
    }
    .frame(width: cellwidth * 6, height: cellwidth * 2)
  }
  
  static func createPieces(cellwidth: CGFloat, side: Side, pieceStyle: PieceStyle, editing: Bool, make: ((_ piece: Piece, _ from: Int, _ dest: Int, _ promotion: Int, _ task: MoveTask) -> Bool)? = nil) -> some View {
    HStack {
      ForEach (1 ..< 7) { i in
        GPiece(piece: Piece(type: i, side: side),
               pos: -1,
               cellwidth: cellwidth,
               pieceStyle: pieceStyle,
               flip: false,
               editing: editing,
               humanSide: Side.none,
               make: { (piece, from, dest, promotion, moveTask) -> Bool in
                return make?(piece, from, dest, promotion, moveTask) ?? true },
               tap: { pos in },
               startDragging: { pos in },
               isArrowEnd: false,
               aniFrom: -1, aniDest: -1, aniPromotion: -1, aniTask: MoveTask.make, selectedPos: -1, showLegalMark: false
        )
      }
    }
    .padding(0)
  }
}

//struct OptionPieceStyle_Previews: PreviewProvider {
//  static var previews: some View {
//    var userData = UserData()
//    OptionPieceStyle(userData: $userData)
//  }
//}
