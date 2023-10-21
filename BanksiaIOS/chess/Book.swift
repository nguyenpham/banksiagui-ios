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


struct BookMove {
    //    int bookIdNumber;
    var key: [UInt64] = [ 0, 0]
    var active = true, flipped = true
    var comment = ""
    var hist = Hist()
    
    // for obk
    var win: Int = 0, draw: Int = 0, lost: Int = 0
}

struct BookPolyglotItem {
    var key: UInt64 = 0
    var move: UInt16 = 0
    var weight: UInt16 = 0
    var learn: UInt32 = 0
    
    func getMove(board: ChessBoard) -> Move {
        let df = move & 0x7, dr = move >> 3 & 0x7;
        var dest = Int((7 - dr) * 8 + df)
        
        let f = Int(move >> 6 & 0x7), r = Int(move >> 9 & 0x7)
        let from = (7 - r) * 8 + f;
        
        // castling moves
        if (dr == r && (from == 4 || from == 60)
            && (df == 0 || df == 7)
            && board.getPiece(Int(from)).type == Piece.KING) {
            dest = dest == 0 ? 2 : dest == 7 ? 6 : dest == 56 ? 58 : 62;
        }
        
        let p = Int(move >> 12 & 0x3)
        let promotion: Int = p == 0 ? Piece.EMPTY : (6 - p)
        return Move(from: from, dest: dest, promotion: promotion)
    }
    
    //      static u16 fromMove(const Hist& hist);
    
    mutating func convertToLittleEndian() {
        let k0 =
        (key >> 56) |
        ((key << 40) & 0x00FF000000000000) |
        ((key << 24) & 0x0000FF0000000000) |
        ((key << 8) & 0x000000FF00000000)
        
        let k1 =
        ((key >> 8) & 0x00000000FF000000) |
        ((key >> 24) & 0x0000000000FF0000) |
        ((key >> 40) & 0x000000000000FF00) |
        (key << 56)
        
        key = k0 | k1
        move = (move >> 8) | (move << 8)
        weight = (weight >> 8) | (weight << 8)
        
        learn = (learn >> 24) | ((learn<<8) & 0x00FF0000) | ((learn>>8) & 0x0000FF00) | (learn << 24)
    }
}


class BookPolyglot
{
    var itemCnt: Int64 = 0, memallocCnt: Int64 = 0
    var items = [BookPolyglotItem]()
    var maxPly = 20
    var top100 = 0
    
    init() {
        if let url = Bundle.main.url(forResource: "fruit", withExtension: "bin") {
            loadData(url: url)
        }
    }
    
    func loadData(url: URL) {
        guard let data = try? Data(contentsOf: url) else {
            return
        }
        
        items = Array<BookPolyglotItem>(repeating: BookPolyglotItem(), count: data.count/MemoryLayout<BookPolyglotItem>.size)
        _ = items.withUnsafeMutableBytes { data.copyBytes(to: $0) }
        
        print("data.count", data.count, "items count", items.count)
        
        for i in (0 ..< items.count) {
            items[i].convertToLittleEndian()
        }
    }
    
    func binarySearch(key: UInt64) -> Int {
        var first = 0, last = items.count - 1
        
        while first <= last {
            var middle: Int = (first + last) / 2
            if items[middle].key == key {
                while middle > 0 && items[middle - 1].key == key {
                    middle -= 1
                }
                return middle
            }
            
            if (items[middle].key > key)  {
                last = middle - 1
            }
            else {
                first = middle + 1
            }
        }
        return -1;
    }
    
    func search(key: UInt64) -> [BookPolyglotItem]  {
        var vec = [BookPolyglotItem]()
        
        var k = binarySearch(key: key)
        if k >= 0 {
            while(k < items.count && items[k].key == key) {
                vec.append(items[k])
                k += 1
            }
        }
        
        return vec
    }
    
    func probe(board: ChessBoard, forBestMove: Bool) -> Move {
        
        //    var bestMove = Move.illegalMove
        
        if items.isEmpty {
            return Move.illegalMove
        }
        
        let key = board.hashKey
        let vec = search(key: key)
        if !vec.isEmpty {
            
            var cb = ChessBoard()
            cb.setFen(fen: board.getFen())
            
            
            for m in vec {
                let move = m.getMove(board: board)
                print("move", move.from, move.dest)
            }
            
            // check later
            let k = Int(vec.count * top100 / 100)
            let idx = k == 0 ? 0 : Int.random(in: 0 ..< k)
            
            let move = vec[idx].getMove(board: board)
            if cb.checkMake(from: move.from, dest: move.dest, promotion: move.promotion) {
                return move
            }
            
            //      var bookMoveVec = [BookMove]()
            
            
            //moveList.push_back(board.histList.back());
            //              else {
            //                  auto bestScore = -1;
            //
            //                  for(auto && item : vec) {
            //                      auto m = item.getMove(board);
            //                      if (!cb->checkMake(m.from, m.dest, m.promotion)) {
            //                          break;
            //                      }
            //                      BookMove bm;
            //                      bm.bookIdNumber = idNumber;
            //                      bm.active = true;
            //                      bm.key[0] = key;
            //                      bm.hist = cb->histList.back();
            //                      bm.hist.move.score = item.weight;
            //                      bm.rank = item.weight;
            //                      bm.winrate = item.learn;
            //                      bookMoveVec.push_back(bm);
            //                      if (item.weight > bestScore) {
            //                          bestScore = item.weight;
            //                          bestMove = m;
            //                      }
            //                      cb->takeBack();
            //                  }
            //                game->probeMap[this] = bookMoveVec;
        }
        return Move.illegalMove
    }
    
}


