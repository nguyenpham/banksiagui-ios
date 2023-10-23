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

struct AboutView: View {
    var body: some View {
        VStack {
            Text("Banksia GUI for iOS, version \(ContentView.version)")
                .bold()
            Text("by Nguyen Pham, 2020, 2023")
                .italic()
            
            Text("Stockfish network: \(Game.networkName(engineIdx: stockfish))")
            Text("LC0 network: \(Game.networkName(engineIdx: lc0))")
            Text("RubiChess network: \(Game.networkName(engineIdx: rubi))")

            WebView(text: "credit", isFile: true)
            if #available(iOS 14.0, *) {
                Link("BanksiaGUI home page", destination: URL(string: "https://banksiagui.com/")!)
                Link("Nguyen Pham GitHub", destination: URL(string: "https://github.com/nguyenpham/")!)
                Link("Donate", destination: URL(string: "https://banksiagui.com/download/")!)
            }
        }
        .navigationBarTitle("About", displayMode: .inline)
    }
}

struct AboutView_Previews: PreviewProvider {
    static var previews: some View {
        AboutView()
    }
}
