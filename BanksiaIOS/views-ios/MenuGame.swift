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
import Combine

struct GameSetup: View {
    @Binding var isNavigationBarHidden: Bool
    @EnvironmentObject private var game: Game
    
    @State private var nodeString = ""
    //  {
    //    didSet {
    //      if nodeString != oldValue {
    //        game.timer_nodes = Int(nodeString) ?? 0
    //      }
    //    }
    //  }
    @State private var oldEngineIdx = 0
    
    var body: some View {
        List {
            Section(header: Text("Analyse on fly")) {
                Toggle("Analyse on fly", isOn: $game.analysicOnFlyMode)
            }
            
            Section(header: Text("Side")) {
                turnView(turn: Turn.human_white)
                turnView(turn: Turn.human_black)
                turnView(turn: Turn.human_both)
                //        turnView(turn: Turn.computer_both)
            }
            .disabled(game.analysicOnFlyMode)
            
            Section(header: Text("Timer")) {
                /// standard
                HStack {
                    VStack {
                        HStack {
                            Text("Standard")
                                .foregroundColor(game.analysicOnFlyMode ? Color.gray : Color.black)
                            Spacer()
                        }
                        .onTapGesture {
                            self.game.timeControlMode = .standard
                        }
                        
                        Stepper(value: $game.timer_standard_time, in: 1...2000000) {
                            HStack {
                                Spacer()
                                Text("Time: \(game.timer_standard_time) m")
                                    .foregroundColor(game.analysicOnFlyMode ? Color.gray : Color.black)
                            }
                        }
                        Stepper(value: $game.timer_standard_moves, in: 0...100) {
                            HStack {
                                Spacer()
                                Text("For moves: \(game.timer_standard_moves)")
                                    .foregroundColor(game.analysicOnFlyMode ? Color.gray : Color.black)
                            }
                        }
                        
                        Stepper(value: $game.timer_standard_inc, in: 0...120) {
                            HStack {
                                Spacer()
                                Text("Increase: \(game.timer_standard_inc)")
                                    .foregroundColor(game.analysicOnFlyMode ? Color.gray : Color.black)
                            }
                        }
                    }
                    GameSetup.selectedMark(selected: game.timeControlMode == .standard)
                }
                .disabled(game.analysicOnFlyMode)
                
                /// Depth
                HStack {
                    Text("Depth: ")
                        .foregroundColor(game.analysicOnFlyMode ? Color.gray : Color.black)
                        .onTapGesture {
                            self.game.timeControlMode = .depth
                        }
                    Stepper(value: $game.timer_depth, in: 1...100) {
                        HStack {
                            Spacer()
                            Text("\(self.game.timer_depth)")
                                .foregroundColor(game.analysicOnFlyMode ? Color.gray : Color.black)
                        }
                    }
                    GameSetup.selectedMark(selected: game.timeControlMode == .depth)
                }
                .disabled(game.analysicOnFlyMode)
                
                HStack {
                    Text("Nodes: ")
                        .foregroundColor(game.analysicOnFlyMode ? Color.gray : Color.black)
                        .onTapGesture {
                            self.game.timeControlMode = .nodes
                        }
                    Spacer()
                    TextField("Nodes:", text: $nodeString, onEditingChanged: { _ in
                        if let n = Int(nodeString) {
                            game.timer_nodes = n
                        }
                    })
                    //            .frame(width: 100, height: 20)
                    .multilineTextAlignment(.trailing)
                    .keyboardType(.numberPad)
                    
                    GameSetup.selectedMark(selected: game.timeControlMode == .nodes)
                }
                .disabled(game.analysicOnFlyMode)
                
                HStack {
                    Text("Move time: ")
                        .foregroundColor(game.analysicOnFlyMode ? Color.gray : Color.black)
                        .onTapGesture {
                            self.game.timeControlMode = .movetime
                        }
                    Spacer()
                    Stepper(value: $game.timer_movetime, in: 1...2000) {
                        HStack {
                            Spacer()
                            Text("\(self.game.timer_movetime) s")
                                .foregroundColor(game.analysicOnFlyMode ? Color.gray : Color.black)
                        }
                    }
                    GameSetup.selectedMark(selected: game.timeControlMode == .movetime)
                }
                
                HStack {
                    Text("Infinite")
                        .foregroundColor(game.analysicOnFlyMode ? Color.gray : Color.black)
                    Spacer()
                    GameSetup.selectedMark(selected: game.timeControlMode == .infinite)
                }
                .onTapGesture {
                    self.game.timeControlMode = .infinite
                }
                
            }
            .disabled(game.analysicOnFlyMode)
            
            Section(header: Text("Strength")) {
                if game.maxcores > 1 {
                    Stepper(value: $game.cores, in: 1...game.maxcores) {
                        HStack {
                            Text("Threads")
                            Spacer()
                            Text("\(game.cores) / \(game.maxcores)")
                        }
                    }
                }
                
//                if game.getEngineIdNumb() == stockfish {
//                    VStack {
//                        HStack {
//                            Text("Evaluation")
//                            Spacer()
//                        }
//                        Picker(selection: $game.evalNNUEMode, label: Text("Evaluation")) {
//                            ForEach (EvaluationMode.allCases, id:\.self) { em in
//                                Text(em.getName()).tag(em.rawValue)
//                            }
//                        }.pickerStyle(SegmentedPickerStyle())
//                    }
//                    
//                    Stepper(value: $game.skillLevel, in: 0...20) {
//                        HStack {
//                            Text("Skill level")
//                            Spacer()
//                            Text("\(game.skillLevel)")
//                        }
//                    }
//                }
                
                Toggle("Use opening books", isOn: $game.bookMode)
                
                if game.canBenchmark() {
                    NavigationLink(
                        destination: Benchmark(isNavigationBarHidden: self.$isNavigationBarHidden).environmentObject(game) ) {
                            Text("Benchmark")
                        }
                        .disabled(self.game.expectingBestmove)
                }
            }
            
            if game.engineData.allEngines.count > 1 {
                Section(header: Text("Engines (\(game.engineData.allEngines.count))")) {
                    ForEach (0 ..< game.engineData.allEngines.count) { index in
                        engineView(index)
                    }
                }
            }
        }
        .navigationBarTitle("Level", displayMode: .inline)
        .onAppear {
            oldEngineIdx = game.engineIdx
            game.engineChanged = false
            nodeString = "\(game.timer_nodes)"
            //      self.isNavigationBarHidden = false
        }
        .onDisappear() {
            self.game.write()
            game.engineChanged = oldEngineIdx != game.engineIdx
        }
    }
    
    func turnView(turn: Turn) -> some View {
        HStack {
            Text("\(turn.getName())")
                .foregroundColor(game.analysicOnFlyMode ? Color.gray : Color.black)
            Spacer()
            
            GameSetup.selectedMark(selected: turn == self.game.turn)
        }
        .onTapGesture {
            self.game.turn = turn
        }
    }
    
    static func selectedMark(selected: Bool) -> some View {
        HStack {
            if selected {
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
        }
    }
    
    func engineView(_ index: Int) -> some View {
        VStack {
            let enginInfo = self.game.engineData.allEngines[index]
            HStack {
                Text("\(index + 1). \(enginInfo.name)")
                Spacer()
                GameSetup.selectedMark(selected: index == self.game.engineIdx)
            }
            HStack {
                Text("ver \(enginInfo.version), by: \(enginInfo.author), elo: \(enginInfo.elo)")
                    .foregroundColor(Color.gray)
                    .font(.system(size: 11))
                Spacer()
            }
        }.onTapGesture {
            self.game.engineIdx = index
            self.game.write()
        }
    }
}

