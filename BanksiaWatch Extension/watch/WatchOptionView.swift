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

struct WatchOptionView: View {
  @EnvironmentObject var userData: UserData
  private let cellwidth: CGFloat = 18
  
  var body: some View {
    ScrollView {
      Section(header: Text("Sound")) {
        createSoundOption(soundMode: .off)
        createSoundOption(soundMode: .simple)
        createSoundOption(soundMode: .speech)
      }
      
      Section(header: Text("Piece")) {
        createPieces(pieceStyle: PieceStyle.alpha)
        createPieces(pieceStyle: PieceStyle.fantasy)
      }
      
      Section(header: Text("Board color")) {
        createCellLines(cellStyle: CellStyle.green)
        createCellLines(cellStyle: CellStyle.red)
        createCellLines(cellStyle: CellStyle.gray)
      }
    }
    .onDisappear() {
      self.userData.write()
    }
  }
  
  func createSoundOption(soundMode: SoundMode) -> some View {
    HStack {
      WatchGameSetup.selectedMark(selected: soundMode.rawValue == self.userData.soundMode)
      Text("\(soundMode.getName())")
      Spacer()
    }
    .onTapGesture {
      self.userData.soundMode = soundMode.rawValue
    }
  }

  func createPieces(pieceStyle: PieceStyle) -> some View {
    VStack {
      HStack {
        WatchGameSetup.selectedMark(selected: pieceStyle == self.userData.pieceStyle)
        Text("\(pieceStyle.getName())")
        Spacer()
      }
      HStack {
        Spacer()
        WatchOptionView.createPieces(cellwidth: self.cellwidth, pieceStyle: pieceStyle)
          .padding(6)
          .background(Color.gray.opacity(0.5))
      }
    }
    .onTapGesture {
      self.userData.pieceStyle = pieceStyle
    }
    
  }
  
  static func createPieces(cellwidth: CGFloat, pieceStyle: PieceStyle, editing: Bool = false, make: ((_ piece: Piece, _ from: Int, _ dest: Int, _ promotion: Int, _ task: MoveTask) -> Bool)? = nil) -> some View {
    VStack {
      WatchOptionView.createPieces(cellwidth: cellwidth, side: Side.white, pieceStyle: pieceStyle, editing: editing, make: make)
      WatchOptionView.createPieces(cellwidth: cellwidth, side: Side.black, pieceStyle: pieceStyle, editing: editing, make: make)
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
  
  func createCellLines(cellStyle: CellStyle) -> some View {
    HStack {
      WatchGameSetup.selectedMark(selected: cellStyle == self.userData.cellStyle)
      Text("\(cellStyle.getName())")
      Spacer()
      ZStack {
        GBoard.createLines(x: cellwidth / 2,
                           y: cellwidth / 2,
                           cellwidth: self.cellwidth,
                           cellStyle: cellStyle,
                           cellCnt: 4,
                           rowCnt: 2)
      }
      .padding(0)
      .frame(width: self.cellwidth * 4, height: self.cellwidth * 2, alignment: .center)
      
    }.onTapGesture {
      self.userData.cellStyle = cellStyle
    }
  }
  
}

struct WatchOptionView_Previews: PreviewProvider {
  static var previews: some View {
    WatchOptionView()
  }
}
