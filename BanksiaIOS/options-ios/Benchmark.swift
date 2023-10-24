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

struct Benchmark: View {
    @Binding var isNavigationBarHidden: Bool
    @EnvironmentObject private var game: Game
    @State var benchmarkInfo = ""
    @State private var computing = false
    
    var body: some View {
        VStack {
            //if game.getEngineIdNumb() != lc0
            if game.maxcores > 1
                && !computing {
                Button("Standard benchmark", action: {
                    startBenchmark()
                })
                .padding()
                
                Button("Benchmark with \(game.maxcores) threads", action: {
                    startBenchmark(core: game.maxcores)
                })
                .padding()
                
                Spacer()
            }
            
            Text(getExtraInfo())
                .font(.system(.body, design: .monospaced))

            if !game.benchComputing.isEmpty {
                Text(game.benchInfo.isEmpty ? "Computing..." : game.benchInfo)
                    .font(.system(.body, design: .monospaced))
                    .frame(width: nil, height: nil, alignment: .topLeading)
                    .background(Color(red: 0.95, green: 0.95, blue: 0.95))
                    .padding()
                
                ScrollView(.vertical) {
                    Text(game.benchComputing)
                        .font(.system(size: 12))
                        .frame(width: nil, height: nil, alignment: .topLeading)
                        .padding(6)
                }
            }
        }
        .navigationBarTitle("Benchmark \(game.getEngineDisplayName())", displayMode: .inline)
        .onAppear {
            isNavigationBarHidden = false
            
            computing = false
            game.benchInfo = ""
            game.benchComputing = ""
            game.benchMode = true
            
            //if game.getEngineIdNumb() == lc0 || game.maxcores <= 1 {
            if game.maxcores <= 1 {
                startBenchmark()
            }
        }
        .onDisappear() {
            isNavigationBarHidden = true
            if game.benchMode {
                game.sendUciStop()
            }
            computing = false
        }
    }
    
    func getExtraInfo() -> String {
        var s =  "Version: \(game.getEngineVersion())"

        if game.engineIdx != lc0 {
            s += "\nThreads: \(game.benchCores)"
        }
        
        let network = game.networkName()
        if !network.isEmpty {
            s += "\nNetwork: \(network)"
        }
        return s
    }

    func startBenchmark(core: Int = 1) {
        computing = true
        game.benchmark(core: core)
    }
}

