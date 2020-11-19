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
import UIKit


extension ContentView {
  
  func processEngineOutput(_ str: String) {
    
    if game.benchMode {
      if str.starts(with: "Total") || str.starts(with: "Nodes") {
        if !game.benchInfo.isEmpty {
          game.benchInfo += "\n"
        }
        game.benchInfo += str
      } else if str.starts(with: "bench END") {
        game.benchMode = false;
      } else {
        if game.benchComputing.length > 32 * 1024 {
          game.benchComputing = game.benchComputing.substring(fromIndex: 8 * 1024)
        }
        game.benchComputing = str + "\n" + game.benchComputing
      }
      return
    }
    
    if (str.starts(with: "bestmove ")) {
      game.expectingBestmove = false

      if (computingTask.isEmpty || !doComputingTask()) && !game.analysicOnFlyMode {
        let arr = str.split(separator: " ")
        if arr.count > 1 {
          let move = game.chessBoard.moveFromCoordiateString(String(arr[1]))
          aniPromotion = move.promotion
          aniDest = move.dest
          aniFrom = move.from
          aniTask = MoveTask.make
        }
      }
      return
    }
    
    if (str.starts(with: "info ")) {
      let arr = str.split(separator: " ")
      var i = 0
      while i < arr.count {
        let w = arr[i]
        if w == "pv" {
          i += 1
          var pv = ""
          if i < arr.count {
            let move = game.chessBoard.moveFromCoordiateString(String(arr[i]))
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
    }
  }

  func genLegalMovesFrom(pos: Int) {
    if editing {
      return
    }

    var mset = Set<Int>()
    var board = ChessBoard()
    board.setFen(fen: game.chessBoard.getFen())
    var moves = [MoveFull]()
    board.genLegalOnly(moveList: &moves, attackerSide: board.side)
    for m in moves {
      if m.from == pos {
        mset.insert(m.dest)
      }
    }
    
    legalSet = mset
  }

  func make(piece: Piece, from: Int, dest: Int, promotion: Int, moveTask: MoveTask) -> Bool {
    if moveTask == .prev || moveTask == .next {
      if moveTask == .prev {
        game.displayingChessBoard.takeBack()
      } else {
        if game.displayingChessBoard.histList.count < game.chessBoard.histList.count {
          let move = game.chessBoard.histList[game.displayingChessBoard.histList.count].move
          game.displayingChessBoard.make(move: move)
        }
      }
      clearAllMarks()
      _ = analysicOnFly_true()
      return true
    }
    
    if from == dest || (self.selectedPos >= 0 && from >= 0 && dest < 0) {
      self.selectPos(pos: from)
      return false
    }
    
    #if !os(watchOS)
    if self.editing {
      if dest >= 0 && dest < 64 {
        self.game.displayingChessBoard.setPiece(pos: dest, piece: piece)
      }

      var r = false
      if from >= 0 && from < 64 { /// from inside the board
        self.game.displayingChessBoard.setPiece(pos: from, piece: Piece.emptyPiece)
        r = true
      }
      editingUpdateFen()
      return r
    }
    #endif
    
    /// Promotion
    if promotion <= Piece.EMPTY &&
        piece.type == PieceTypeStd.pawn.rawValue &&
        ((piece.side == Side.white && dest < 8) || (piece.side == Side.black && dest >= 56)) &&
        abs(from / 8 - dest / 8) == 1
    {
      promotionFrom = from
      promotionDest = dest
      showingPopup_promotion = true
      return true
    }
    return checkMakeMove(from: from, dest: dest, promotion: promotion, byComputer: false)
  }

  func checkMakeMove(from: Int, dest: Int, promotion: Int, byComputer: Bool) -> Bool {
    assert(promotion >= 0)
    let moveElapseInMillisecond = Int((CFAbsoluteTimeGetCurrent() - startMoveTime) * 1000)
    if game.checkMakeMove(from: from, dest: dest, promotion: promotion, byComputer: byComputer, moveElapseInMillisecond: moveElapseInMillisecond) {
//      game.chessBoard.printBoard()
      /// Store computing info
      if !depthString.isEmpty && (game.analysicOnFlyMode || !game.isHumanTurn(side: game.chessBoard.side.getXSide())) {
        game.chessBoard.histList.last?.es.depth = depth
        game.chessBoard.histList.last?.es.score = score
        game.chessBoard.histList.last?.es.mating = mating
        game.chessBoard.histList.last?.es.elapsedInMillisecond = moveElapseInMillisecond
        game.chessBoard.histList.last?.es.nodes = nodes
        game.chessBoard.histList.last?.es.nps = nps
        assert(game.chessBoard.histList.last?.es.elapsedInMillisecond == moveElapseInMillisecond)
      }

      setupForNewMove()
      playSound(move: game.chessBoard.histList.last?.move ?? MoveFull.illegalMoveFull, sanString:  game.chessBoard.histList.last?.sanString ?? "")
      return true
    }
    clearAllMarks()
    playSound(move: MoveFull.illegalMoveFull, sanString: "")
    return false
  }
  
  func newGame() {
    if game.expectingBestmove {
      return delayTask(.new)
    }
    computingTask.removeAll()
    game.expectingBestmove = false
    computingTask.removeAll()
    game.newGame()
    newGame_complete()
  }

  func newGame_complete() {
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

    setupForNewMove()
  }

  func computerGo() {
    if game.expectingBestmove {
      return delayTask(.go)
    }

    /// Opening book moves
    if game.bookMode && !game.analysicOnFlyMode {
      let move = game.probeBook()
      if move.isValid() && checkMakeMove(from: move.from, dest: move.dest, promotion: move.promotion, byComputer: true) {
        return
      }
    }

    game.computerGo()
  }
  
  func delayTask(_ task: ComputingTask) {
    DispatchQueue.global(qos: .default).async {
      self.computingTask.append(task)
      self.game.sendUciStop()
    }
  }

  func doComputingTask() -> Bool {
    if computingTask.isEmpty || game.expectingBestmove {
      return false
    }
    DispatchQueue.global(qos: .default).async {
      guard let task = computingTask.first, !game.expectingBestmove else {
        return
      }
      computingTask.remove(at: 0)
      
      switch task {
      case .takeback:
        takeBack()
      case .go:
        computerGo()
      case .new:
        newGame()
      }
    }
    return true
  }
  
  func takeBack() {
    if game.expectingBestmove {
      return delayTask(.takeback)
    }

    let moveElapseInMillisecond = Int((CFAbsoluteTimeGetCurrent() - startMoveTime) * 1000)
    game.takeBack(moveElapseInMillisecond: moveElapseInMillisecond)
    setupForNewMove()
    game.savePGNGame()
//    game.chessBoard.printBoard()
  }

}
