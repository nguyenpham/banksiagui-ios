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

struct WatchEngineList: View {
    @EnvironmentObject var game: Game
    
    @State var showBenchmark = false
    @State private var extraInfo = ""
    
    @State private var oldEngineIdx = 0
    
    var body: some View {
        ScrollView {
            if oldEngineIdx != game.engineIdx {
                Text("Warning: the app will be reset because engine changed")
                    .foregroundColor(.red)
                    .font(.system(size: 12))
                    .fixedSize(horizontal: false, vertical: true)
            }
            
            ForEach (0 ..< game.engineData.allEngines.count) { index in
                engineView(index)
            }
            
            if showBenchmark {
                benchmarkView()
            }
            Button(showBenchmark ? "Hide benchmark" : "Benchmark", action: {
                showBenchmark.toggle()
            })
        }
        .onAppear {
            oldEngineIdx = game.engineIdx
        }
        .onDisappear() {
            if oldEngineIdx != game.engineIdx {
                exit(0)
            }
        }
        
    }
    
    func engineView(_ index: Int) -> some View {
        VStack {
            let enginInfo = self.game.engineData.allEngines[index]
            HStack {
                Text("\(index + 1). \(enginInfo.name)")
                Spacer()
                WatchGameSetup.selectedMark(selected: index == self.game.engineIdx)
            }
            HStack {
                Text("ver \(enginInfo.version), by: \(enginInfo.author)\n\(Game.networkName(engineIdx: enginInfo.idNumb))")
                    .foregroundColor(Color.gray)
                    .font(.system(size: 11))
                Spacer()
            }
        }.onTapGesture {
            self.game.engineIdx = index
            self.game.write()
        }
    }
    
    func benchmarkView() -> some View {
        VStack {
            Text(game.benchInfo.isEmpty ? "Computing\(extraInfo)..." : game.benchInfo)
                .foregroundColor(Color.black)
                .frame(width: nil, height: nil, alignment: .topLeading)
                .background(Color(red: 0.95, green: 0.95, blue: 0.95))
            
            ScrollView(.vertical) {
                Text(game.benchComputing)
                    .font(.system(size: 10))
                    .foregroundColor(Color.yellow)
                    .frame(width: nil, height: nil, alignment: .topLeading)
                    .padding(6)
            }
        }
        .onAppear {
            game.benchInfo = ""
            game.benchComputing = ""
            game.benchMode = true
            
            if game.getEngineIdNumb() == stockfish {
                extraInfo = " (\(game.maxcores) threads)"
            } else {
                extraInfo = ""
            }
            
            game.benchmark(core: game.maxcores)
        }
        .onDisappear() {
            if game.benchMode {
                game.sendUciStop()
            }
        }
    }
}

struct EngineSelector_Previews: PreviewProvider {
    static var previews: some View {
        WatchEngineList()
    }
}
