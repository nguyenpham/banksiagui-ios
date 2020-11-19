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

struct WatchAboutView: View {
  let arr = [
    "BanksiaGUI ver \(ContentView.version)",
    "by Nguyen Pham, 2020",
    "",
    "This software is an open source and dedicated to my wife and anyone who have been fighting with breast cancer",
    "Please help us to improve the app and/or donate, thanks!",
    "",
    "https://banksiagui.com/",
    "https://github.com/nguyenpham/"
  ]
  
  var body: some View {
    ScrollView {
      ForEach (arr, id:\.self) { s in
        Text(s)
          .font(.caption2)
          .multilineTextAlignment(.center)
          .allowsTightening(true)
          .fixedSize(horizontal: false, vertical: true)
      }
    }
  }
}

struct WatchAboutView_Previews: PreviewProvider {
  static var previews: some View {
    WatchAboutView()
  }
}
