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

enum MenuTask : Int, CaseIterable {
    case new,
         flip,
         paste,
         copy,
         edit,
         cancel
    
    private static let allNames = [
        "New game",
        "Flip",
        "Paste (FEN/PGN)",
        "Copy/Email",
        "Edit position",
        "Cancel" ]
    
    func getName() -> String {
        return MenuTask.allNames[ self.rawValue ]
    }
}

struct MenuView: View {
    let width: CGFloat
    let menuClick: ((_ menuTask: MenuTask) -> Void)
    
    var body: some View {
        ForEach(MenuTask.allCases, id: \.self) { task in
            Button(action: { self.menuClick(task) }) {
                Text(task.getName())
                    .foregroundColor(task == .cancel ? Color.red : Color.blue)
                    .fontWeight(task == .cancel ? .bold : .regular)
                    .frame(width: self.width, height: ContentView.menuHeight, alignment: .center)
            }
            .cornerRadius(20.0)
            .padding(0)
        }
    }
}
