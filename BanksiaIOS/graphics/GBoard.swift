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

struct GBoard: View {
    let cellwidth: CGFloat
    let cellStyle: CellStyle
    let showCoordinates: Bool
    let flip: Bool
    let from: Int
    let dest: Int
    let selectedPos: Int
    
    static let cellCount = 64
    
    var body: some View {
        ZStack {
            createBoard()
            createCoordinates(showCoordinates: showCoordinates, flip: flip)
        }
    }
    
    func createBoard() -> some View {
        ForEach(0 ..< 64) { i in
            GCell(pos: i,
                  width: cellwidth,
                  cellStyle: cellStyle,
                  isBlack: isBlack(i),
                  showCursor: isCursor(i),
                  showSelected: isSelected(i)
            )
            .position(x: CGFloat(i % 8) * self.cellwidth + self.cellwidth / 2,
                      y: CGFloat(i / 8) * self.cellwidth + self.cellwidth / 2)
        }
    }
    
    static func createAccessibilityLabels(cellwidth: CGFloat) -> some View {
        ForEach(0 ..< 64) { i in
            Rectangle()
                .fill(Color.white.opacity(0.01))
                .frame(width: cellwidth, height: cellwidth)
                .position(x: CGFloat(i % 8) * cellwidth + cellwidth / 2,
                          y: CGFloat(i / 8) * cellwidth + cellwidth / 2)
                .accessibilityLabel(Text(ChessBoard.posToCoordinateString(i)))
        }
    }

    func isBlack(_ i: Int) -> Bool {
        return ((i >> 3 + i) & 1) != 0
    }
    
    func isCursor(_ pos: Int) -> Bool {
        let p = flip ? 63 - pos : pos
        return p == from || p == dest
    }
    
    func isSelected(_ pos: Int) -> Bool {
        let p = flip ? 63 - pos : pos
        return p == selectedPos
    }
    
    func createCoordinates(showCoordinates: Bool, flip: Bool) -> some View {
        ZStack {
            if showCoordinates {
                ForEach (0 ..< 8) { i in
                    Text("\(flip ? i + 1 : 8 - i)")
                        .font(.system(size: self.cellwidth / 5))
                        .position(x: self.cellwidth * 7.9,
                                  y: self.cellwidth * (CGFloat(i) + 0.1))
                    
                    Text("\(columnNames[flip ? 7 - i : i])")
                        .font(.system(size: self.cellwidth / 5))
                        .position(x: self.cellwidth * (CGFloat(i) + 0.1),
                                  y: self.cellwidth * 7.9)
                }
            }
        }
    }
    
    static func createLines(x: CGFloat, y: CGFloat,
                            cellwidth: CGFloat,
                            cellStyle: CellStyle,
                            cellCnt: Int, rowCnt: Int) -> some View {
        ForEach(0 ..< rowCnt) { r in
            GBoard.createALine(
                x: x,
                y: y + CGFloat(r) * cellwidth,
                cellwidth: cellwidth,
                cellStyle: cellStyle,
                count: cellCnt,
                firstBlack: (Int(r) & 1) != 0)
        }
    }
    
    static func createALine(x: CGFloat, y: CGFloat,
                            cellwidth: CGFloat, cellStyle: CellStyle,
                            count: Int, firstBlack: Bool) -> some View {
        ForEach (0 ..< count) { i in
            GCell(pos: i, width: cellwidth, cellStyle: cellStyle, isBlack: (((firstBlack ? 1 : 0) + Int(i)) & 1) != 0, showCursor: false,
                  showSelected: false
            )
            .position(x: x + CGFloat(i) * cellwidth, y: y)
        }
    }
    
}

struct GBoard_Previews: PreviewProvider {
    static var previews: some View {
        GBoard(cellwidth: 30, cellStyle: CellStyle.green,
               showCoordinates: true,
               flip: false, from: 0, dest: 10,
               selectedPos: 10
        )
    }
}
