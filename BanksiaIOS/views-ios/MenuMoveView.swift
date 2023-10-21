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

enum MenuMoveTask : Int, CaseIterable {
    case begin, prev, next, end, takeback, cancel
    
    private static let allNames = ["First move", "Prev move (swipe left)", "Next move (swipe right)", "Last move", "Take back", "Cancel" ]
    
    func getName() -> String {
        return MenuMoveTask.allNames[ self.rawValue ]
    }
}

struct MenuMoveView: View {
    let width: CGFloat
    let menuClick: ((_ menuTask: MenuMoveTask) -> Void)
    
    static let menuHeight: CGFloat = 40
    
    var body: some View {
        ForEach(MenuMoveTask.allCases, id: \.self) { task in
            Button(action: { self.menuClick(task) }) {
                Text(task.getName())
                    .foregroundColor(task == .cancel ? Color.red : (task == .takeback ? Color(red: 0.4, green: 0.1, blue: 0.1) : Color.blue))
                    .fontWeight(task == .cancel ? .bold : .regular)
                    .frame(width: self.width, height: ContentView.menuHeight, alignment: .center)
            }
            .cornerRadius(20.0)
            .padding(0)
        }
    }
}

