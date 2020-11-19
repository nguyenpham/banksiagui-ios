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

struct OptionColorScheme: View {
  private let cellwidth: CGFloat = 40
  @EnvironmentObject private var userData: UserData
  
  var body: some View {
    List (CellStyle.allCases, id:\.self) { cellStyle in
      HStack {
        Text("\(cellStyle.getName())")
        Spacer()
        createCellLines(cellStyle: cellStyle)
        
        if cellStyle == self.userData.cellStyle {
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
        self.userData.cellStyle = cellStyle
      }
    }
    .navigationBarTitle("Board color", displayMode: .inline)
    .padding(0)
  }
  
  func createCellLines(cellStyle: CellStyle) -> some View {
    ZStack {
      GBoard.createLines(x: self.cellwidth / 2,
                         y: self.cellwidth / 2,
                         cellwidth: self.cellwidth,
                         cellStyle: cellStyle,
                         cellCnt: 4,
                         rowCnt: 2)
    }
    .padding(0)
    .frame(width: self.cellwidth * 4, height: self.cellwidth * 2, alignment: .center)
  }
}

struct OptionColorScheme_Previews: PreviewProvider {
  static var previews: some View {
    OptionColorScheme()
  }
}
