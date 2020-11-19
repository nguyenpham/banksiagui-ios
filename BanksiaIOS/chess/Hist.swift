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

import Foundation

class EngineScore {
  let ScoreNull = 0xfffffff

  var score: Int
  var mating: Bool = false
  var depth = 0, selectiveDepth = 0
  var elapsedInMillisecond = 0
  var stats_win = 0, stats_draw = 0, stats_loss = 0, nnuehits = 0
  
  var nodes: UInt64 = 0
  var nps: UInt64 = 0
  //  var hashKey: UInt64 = 0
  //   var pv = "", fen = "", wdl = ""
  //    std::vector<HistBasic> pvhist;
  //    void reset();
  
  init() {
    score = ScoreNull
  }
  
  func hasWDL() -> Bool {
    return stats_win + stats_draw + stats_loss > 0
  }
  
  func hasNNUE()  -> Bool {
    return nnuehits > 0
  }
  
  //    void merge(const EngineScore& e);
  //    bool empty() const;
  //    std::string getPVString(Notation notation, bool isChess) const;
}

class Hist {
  var move = MoveFull.illegalMoveFull
  var cap: Piece = Piece.emptyPiece
  var enpassant: Int = -1, status: Int = -1, castled: Int = 0, quietCnt: Int = 0
  var castleRights: [Int] = [0, 0]
  var hashKey: UInt64 = 0
  
  var sanString: String = ""
  var comment: String = ""
  
  var es = EngineScore()

  init() {
  }
  
  init(hist: Hist) {
    move = hist.move
    cap = Piece(piece: hist.cap)
    enpassant = hist.enpassant
    status = hist.status
    castled = hist.castled
    quietCnt = hist.quietCnt
    castleRights = hist.castleRights
    hashKey = hist.hashKey
    sanString = hist.sanString
    comment = hist.comment
    es = hist.es
  }
  
  func set(move: MoveFull) {
    self.move = move;
  }
  
  func isValid() -> Bool {
    return move.isValid() && cap.isValid();
  }
  
  func computingString() -> String {
    var str = ""
    
    var cnt = 0;
    if (es.depth > 0) {
      if (es.mating) {
        str += "M\(es.score)"
        cnt += 1
      } else {
        let value = es.score
        //        if (scoreInWhiteView && move.piece.side == Side.black) {
        //          value = -value;
        //        }
        
        str += "\(Double(value)/100.0)"
        cnt += 1
      }
      
      str += "/\(es.depth) \(es.elapsedInMillisecond)"
      cnt += 1
      
      if (es.nodes > 0) {
        str += " \(es.nodes)"
        cnt += 0
      }
    }
    
    if (es.hasWDL()) {
      if (cnt > 0) {
        str += " "
      }
      str += "\(es.stats_win)/\(es.stats_draw)/\(es.stats_loss)"
      cnt += 1
    }
    
    return str
  }
}
