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

enum Level : Int, CaseIterable {
    case g2, g2_1, g5, g5_2, g15, g15_5, m1s, m2s, m5s, m10s, m30s, d10, d12, d15, d20, d30
    
    private static let allNames = ["Game in 2m", "Game in 2m+1s",
                                   "Game in 5m", "Game in 5m+2s",
                                   "Game in 15m", "Game in 15m+5s",
                                   "1 s/move", "2 s/move",
                                   "5 s/move", "10 s/move", "30 s/move",
                                   "depth 10", "depth 12", "depth 15", "depth 20", "depth 30"
    ]
    
    func getName() -> String {
        return Level.allNames[self.rawValue]
    }
}

struct WatchGameSetup: View {
    @EnvironmentObject var game: Game
    @EnvironmentObject var engineData: EngineData
    
    @State var level = Level.g2
    @State var showBenchmark = false
    @State private var extraInfo = ""
    
    var body: some View {
        ScrollView {
            Section(header: Text("Side")) {
                turnView(turn: Turn.human_white)
                turnView(turn: Turn.human_black)
                turnView(turn: Turn.human_both)
            }
            Section(header: Text("Level")) {
                VStack {
                    levelView(level: .g2)
                    levelView(level: .g2_1)
                    levelView(level: .g5)
                    levelView(level: .g5_2)
                    levelView(level: .g15)
                    levelView(level: .g15_5)
                }
                VStack {
                    levelView(level: .m1s)
                    levelView(level: .m2s)
                    levelView(level: .m5s)
                    levelView(level: .m10s)
                    levelView(level: .m30s)
                }
                VStack {
                    levelView(level: .d10)
                    levelView(level: .d12)
                    levelView(level: .d15)
                    levelView(level: .d20)
                    levelView(level: .d30)
                }
                
                Section(header: Text("Others")) {
                    Button(game.bookMode ? "Opening Book: Yes" : "Opening Book: No", action: {
                        game.bookMode.toggle()
                    })
                    
                    if game.maxcores > 1 || game.canBenchmark() {
                        Text("Threads")
                        HStack {
                            Spacer()
                            
                            Button("-", action: {
                                game.cores = max(1, game.cores - 1)
                            })
                            Text("\(game.cores) / \(game.maxcores)")
                            Button("+", action: {
                                game.cores = min(game.maxcores, game.cores + 1)
                            })
                        }                        
                    }
                }
            }
            
        }
        .onAppear {
            calLevel()
        }
        .onDisappear() {
            self.game.write()
        }
    }
    
    func setupLevel(level: Level) {
        self.level = level
        
        switch level {
        case .g2:
            game.timeControlMode = .standard
            game.timer_standard_moves = 0
            game.timer_standard_time = 2
            game.timer_standard_inc = 0
            
        case .g2_1:
            game.timeControlMode = .standard
            game.timer_standard_moves = 0
            game.timer_standard_time = 2
            game.timer_standard_inc = 1
        case .g5:
            game.timeControlMode = .standard
            game.timer_standard_moves = 0
            game.timer_standard_time = 5
            game.timer_standard_inc = 0
            
        case .g5_2:
            game.timeControlMode = .standard
            game.timer_standard_moves = 0
            game.timer_standard_time = 5
            game.timer_standard_inc = 2
            
        case .g15:
            game.timeControlMode = .standard
            game.timer_standard_moves = 0
            game.timer_standard_time = 15
            game.timer_standard_inc = 0
            
        case .g15_5:
            game.timeControlMode = .standard
            game.timer_standard_moves = 0
            game.timer_standard_time = 15
            game.timer_standard_inc = 5
            
        case .m1s:
            game.timeControlMode = .movetime
            game.timer_movetime = 1
            
        case .m2s:
            game.timeControlMode = .movetime
            game.timer_movetime = 2
        case .m5s:
            game.timeControlMode = .movetime
            game.timer_movetime = 5
            
        case .m10s:
            game.timeControlMode = .movetime
            game.timer_movetime = 10
            
        case .m30s:
            game.timeControlMode = .movetime
            game.timer_movetime = 30
            
        case .d10:
            game.timeControlMode = .depth
            game.timer_depth = 10
            
        case .d12:
            game.timeControlMode = .depth
            game.timer_depth = 12
        case .d15:
            game.timeControlMode = .depth
            game.timer_depth = 12
            
        case .d20:
            game.timeControlMode = .depth
            game.timer_depth = 20
            
        case .d30:
            game.timeControlMode = .depth
            game.timer_depth = 30
            
        }
    }
    
    func calLevel() {
        switch game.timeControlMode {
        case TimeControlMode.standard:
            game.timer_standard_moves = 0
            if [2, 5, 15].contains(game.timer_standard_time) {
                if game.timer_standard_time == 2 {
                    level = game.timer_standard_inc == 0 ? .g2 : .g2_1
                } else
                if game.timer_standard_time == 5 {
                    level = game.timer_standard_inc == 0 ? .g5 : .g5_2
                } else {
                    level = game.timer_standard_inc == 0 ? .g15 : .g15_5
                }
                return
            }
        case .depth:
            switch game.timer_depth {
            case 12:
                level = .d12
            case 15:
                level = .d15
            case 20:
                level = .d20
            case 30:
                level = .d30
                
            default:
                level = .d10
            }
            return
            
        case .movetime:
            switch game.timer_movetime {
            case 1:
                level = .m1s
            case 2:
                level = .m2s
            case 5:
                level = .m5s
            case 10:
                level = .m10s
            default:
                level = .m30s
            }
            return
            
        default:
            break
        }
        
        /// Something wrong, reset
        level = .g5_2
        game.timer_standard_time = 5
        game.timer_standard_moves = 0
        game.timer_standard_inc = 2
        
    }
    
    func levelView(level: Level) -> some View {
        HStack {
            Text("\(level.getName())")
                .foregroundColor(Color.white)
            Spacer()
            
            WatchGameSetup.selectedMark(selected: level == self.level)
        }
        .onTapGesture {
            setupLevel(level: level)
        }
    }
    
    
    func turnView(turn: Turn) -> some View {
        HStack {
            Text("\(turn.getName())")
                .foregroundColor(game.analysicOnFlyMode ? Color.gray : Color.white)
            Spacer()
            
            WatchGameSetup.selectedMark(selected: turn == self.game.turn)
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
                    .frame(width: 15, height: 15)
                    .foregroundColor(.green)
                    .shadow(radius: 1)
            } else {
                Rectangle()
                    .fill(Color.black.opacity(0))
                    .frame(width: 15, height: 15)
            }
        }
    }
}

struct WatchGameSetup_Previews: PreviewProvider {
    static var previews: some View {
        WatchGameSetup()
    }
}
