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

final class EngineOutput: ObservableObject {
    var hasEngineOutput = false
    var pvString = ""
    var scoreString = ""
    var nodeString = ""
    var npsString = ""
    var timeString = ""
    var nnueString = ""

    var depthString = ""
    var depth = 0
    var score = 0
    var mating = false
    var nodes: UInt64 = 0
    var nps: UInt64 = 0
    
    var arrowFrom = -1
    var arrowDest = -1

    func reset() {
        hasEngineOutput = false
        pvString = ""
        scoreString = ""
        depthString = ""
        nodeString = ""
        npsString = ""
        timeString = ""
        nnueString = ""
        
        depth = 0
        score = 0
        mating = false
        nodes = 0
        nps = 0
        arrowFrom = -1
        arrowDest = -1

    }
    
    func parseStringInfor(_ str: String) -> Bool {
        if (!str.starts(with: "info ")) {
            return false
        }
        let arr = str.split(separator: " ")
        var i = 0
        while i < arr.count {
            let w = arr[i]
            if w == "pv" {
                i += 1
                var pv = ""
                if i < arr.count {
                    let move = ChessBoard.moveFromCoordiateString(String(arr[i]))
                    arrowFrom = move.from
                    arrowDest = move.dest
                    while i < arr.count {
                        pv += " " + String(arr[i])
                        i += 1
                    }
                    self.pvString = "pv:\(pv)"
                } else {
                    arrowFrom = -1
                    arrowDest = -1
                    self.pvString = ""
                }
                break
            }
            
            switch w {
            case "depth":
                i += 1
                let s = String(arr[i])
                depthString = "depth: " + s
                depth = Int(s) ?? 0
                
            case "seldepth":
                i += 1
                if !depthString.isEmpty {
                    depthString += "/" + String(arr[i])
                }
                
            case "score":
                scoreString = "score: "
                mating = false
                i += 1
                var s = String(arr[i])
                if s == "cp" {
                    i += 1
                } else if s == "mate" {
                    i += 1
                    scoreString = "M"
                    mating = true
                }
                s = String(arr[i])
                score = Int(s) ?? 0
                scoreString += s + "; "
                
            case "nodes":
                i += 1
                let s = String(arr[i])
                nodes = UInt64(s) ?? 0
                nodeString = "nodes: \(s); "
                
            case "nps":
                i += 1
                let s = String(arr[i])
                nps = UInt64(s) ?? 0
                npsString = "nps: \(s); "
                
            case "time":
                i += 1
                timeString = "time: " + String(arr[i]) + "; "
                
            case "nnuehits":
                i += 1
                nnueString = "nnue: " + String(arr[i]) + "; "
                
            default:
                break
            }
            i += 1
        }
        
        hasEngineOutput = true
        return true
        
    }
    
    func displayString() -> String {
        return "\(depthString)\(depthString.isEmpty ? "" : "; ")\(scoreString)\(nodeString)\(timeString)\(npsString)\(nnueString)"
    }
}

