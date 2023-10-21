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

enum CellStyle : Int, CaseIterable {
    case green, red, gray
    
    private static let allNames = ["Green", "Red", "Gray" ]
    
    private static let imgNames = [
        "greenw", "greenb",
        "redw", "redb",
        "", ""
    ]
    
    func getName() -> String{
        return CellStyle.allNames[self.rawValue]
    }
    
    func whiteImgName() -> String {
        return CellStyle.imgNames[self.rawValue * 2]
    }
    func backImgName() -> String {
        return CellStyle.imgNames[self.rawValue * 2 + 1]
    }
}


struct GCell: View {
    let pos: Int
    let width: CGFloat
    let cellStyle: CellStyle
    let isBlack: Bool
    let showCursor: Bool
    let showSelected: Bool
    
    static let gradient = Gradient(colors: [Color.yellow.opacity(0.1), Color.yellow.opacity(0.7)])
    
    var body: some View {
        ZStack {
            if cellStyle == .gray {
                Rectangle()
                    .fill(isBlack ? Color(red: 0.5, green: 0.5, blue: 0.5) : Color(red: 0.9, green: 0.9, blue: 0.9))
                    .frame(width: width, height: width)
            } else {
                Image(isBlack ? cellStyle.backImgName() : cellStyle.whiteImgName())
                    .resizable()
                    .aspectRatio(contentMode: .fill)
                    .frame(width: width, height: width)
            }
            
            if showCursor {
                Rectangle()
                    .fill(RadialGradient(gradient: GCell.gradient, center: .center, startRadius: 1, endRadius: 100))
                    .border(Color.purple, width: 1)
                    .frame(width: width, height: width)
            }
            
            if showSelected {
                Rectangle()
                    .fill(Color.white.opacity(0.2))
                    .border(Color.green, width: width * 0.08)
                    .frame(width: width, height: width)
            }
        }
    }
}

struct GCell_Previews: PreviewProvider {
    static var previews: some View {
        GCell(pos: 0, width: 40, cellStyle: CellStyle.red, isBlack: true, showCursor: true,
              showSelected: true)
    }
}
