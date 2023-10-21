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

let lowerCase = NSCharacterSet.lowercaseLetters
let upperCase = NSCharacterSet.uppercaseLetters

let columnNames = [ "a", "b", "c", "d", "e", "f", "g", "h", "i" ]

struct ChessBoard {
    /// General
    var variant = ChessVariant.standard
    var startFen = ""
    var side = Side.white
    var pieces = [Piece]()
    var histList = [Hist]()
    
    var status = 0
    var result = Result()
    
    var timeLeft:[Int] = [ 60 * 20, 60 * 20 ]
    var moveTimeLeft:[Int] = [ 60 * 3, 60 * 3 ]
    
    var drawOfferingAt: Int = -100
    
    var hashKey: UInt64 = 0
    
    
    var repetitionThreatHold = 1;
    var quietCnt = 0, fullMoveCnt = 1;
    
    /// Chess
    var enpassant: Int = 0
    var castleRights: [Int] = [ 0, 0 ]
    var castleRights_column_king = 4, castleRights_column_rook_left = 0, castleRights_column_rook_right = 7
    
    static let CastleRight_long: Int  = (1 << 0)
    static let CastleRight_short: Int = (1 << 1)
    static let CastleRight_mask: Int  = (1 << 0) | (1 << 1)
    
    let originalFen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq"
    
    init() {
        variant = ChessVariant.standard
        pieces += [Piece](repeating: Piece.emptyPiece, count: 64)
        
        setFen(fen: "")
    }
    
    func getBoardSize() -> Int {
        return 64
    }
    
    /// General
    mutating func reset() {
        for p in pieces {
            p.setEmpty();
        }
        
        histList = [Hist]()
        quietCnt = 0;
        hashKey = 0
        enpassant = 0
        castleRights[0] = 0
        castleRights[1] = 0
        side = Side.white
        //      result.result = ResultType::noresult;
    }
    
    func isPositionValid(pos: Int) -> Bool {
        return pos >= 0 && pos < pieces.count
    }
    
    func isValid(move: Move) -> Bool {
        return move.isValid() && isPositionValid(pos: move.from) && isPositionValid(pos: move.dest);
    }
    
    func size() -> Int {
        return pieces.count
    }
    
    mutating func setPiece(pos: Int, piece: Piece) {
        pieces[pos] = piece;
    }
    
    func getPiece(_ pos: Int) -> Piece {
        return Piece(piece: pieces[pos])
    }
    
    func getAllTPieces(flip: Bool, aniFrom: Int, legalSet: Set<Int>) -> [TPiece] {
        var arr = [TPiece]()
        for i in 0 ..< size() {
            let p = pieces[i]
            let tp = TPiece(piece: p, pos: i, flip: flip, hasAni: i == aniFrom, showLegalMark: legalSet.contains(i))
            arr.append(tp)
        }
        //    assert(!arr.isEmpty)
        return arr
    }
    
    func isEmpty(pos: Int) -> Bool {
        return pieces[pos].isEmpty()
    }
    
    func isPiece(pos: Int, type: Int,  side: Side) -> Bool {
        let p = pieces[pos];
        return p.type == type && p.side == side;
    }
    
    func setEmpty(pos: Int) {
        pieces[pos].setEmpty();
    }
    
    mutating func reset(fen: String?) {
        drawOfferingAt = -100
    }
    
    mutating func clone(fromBoard: ChessBoard) -> Bool {
        startFen = fromBoard.startFen
        side = fromBoard.side
        
        pieces.removeAll(keepingCapacity: true)
        for p in fromBoard.pieces {
            pieces.append(Piece(piece: p))
        }
        histList.removeAll(keepingCapacity: true)
        for hist in fromBoard.histList {
            histList.append(Hist(hist: hist))
        }
        
        status = fromBoard.status
        result = fromBoard.result
        timeLeft = fromBoard.timeLeft
        moveTimeLeft = fromBoard.moveTimeLeft
        drawOfferingAt = fromBoard.drawOfferingAt
        hashKey = fromBoard.hashKey
        repetitionThreatHold = fromBoard.repetitionThreatHold
        quietCnt = fromBoard.quietCnt
        fullMoveCnt = fromBoard.fullMoveCnt
        enpassant = fromBoard.enpassant
        castleRights = fromBoard.castleRights
        castleRights_column_king = fromBoard.castleRights_column_king
        castleRights_column_rook_left = fromBoard.castleRights_column_rook_left
        castleRights_column_rook_right = fromBoard.castleRights_column_rook_right
        
        return true;
    }
    
    func isValid() -> Bool {
        if (side == Side.none || pieces.count != getBoardSize()) {
            return false;
        }
        
        //    if castleRights[0] + castleRights[1] > 0 && !isValidCastleRights() {
        //        return false;
        //    }
        
        var pieceCout = [[0, 0, 0, 0, 0, 0, 0], [ 0, 0, 0, 0, 0, 0, 0]]
        
        for (i, piece) in pieces.enumerated() {
            if piece.isEmpty() {
                continue;
            }
            
            pieceCout[piece.side.rawValue][piece.type] += 1
            if piece.type == PieceTypeStd.pawn.rawValue && (i < 8 || i >= 56) {
                return false
            }
        }
        
        if enpassant > 0 {
            let row = getRank(enpassant)
            if row != 2 && row != 5 {
                return false;
            }
            let pawnPos = row == 2 ? (enpassant + 8) : (enpassant - 8);
            if !isPiece(pos: pawnPos, type: PieceTypeStd.pawn.rawValue, side: row == 2 ? Side.black : Side.white) {
                return false
            }
        }
        
        /// Check number
        let b =
        pieceCout[0][PieceTypeStd.king.rawValue] == 1 && pieceCout[1][PieceTypeStd.king.rawValue] == 1 &&     // king
        pieceCout[0][PieceTypeStd.queen.rawValue] <= 9 && pieceCout[1][PieceTypeStd.queen.rawValue] <= 9 &&     // queen
        pieceCout[0][PieceTypeStd.rook.rawValue] <= 10 && pieceCout[1][PieceTypeStd.rook.rawValue] <= 10 &&     // rook
        pieceCout[0][PieceTypeStd.bishop.rawValue] <= 10 && pieceCout[1][PieceTypeStd.bishop.rawValue] <= 10 &&     // bishop
        pieceCout[0][PieceTypeStd.knight.rawValue] <= 10 && pieceCout[1][PieceTypeStd.knight.rawValue] <= 10 &&     // knight
        pieceCout[0][PieceTypeStd.pawn.rawValue] <= 8 && pieceCout[1][PieceTypeStd.pawn.rawValue] <= 8 &&       // pawn
        pieceCout[0][2]+pieceCout[0][3]+pieceCout[0][4]+pieceCout[0][5] + pieceCout[0][6] <= 15 &&
        pieceCout[1][2]+pieceCout[1][3]+pieceCout[1][4]+pieceCout[1][5] + pieceCout[1][6] <= 15;
        
        if !b {
            return false
        }
        
        /// A King can't capture  other King
        let kingB = findKing(side: Side.black)
        let kingW = findKing(side: Side.white)
        
        if !isPositionValid(pos: kingW) || !isPositionValid(pos: kingB) {
            return false
        }
        
        let d = abs(kingW - kingB)
        if d == 1 || d == 8 ||
            ((d == 7 || d == 9) && abs(kingW % 8 - kingB % 8) == 1 && abs(kingW / 8 - kingB / 8) == 1) {
            return false
        }
        
        return true
    }
    
    func toString() -> String {
        return "fen=[\(startFen)], #move=\(histList.count)";
    }
    
    func isOrigin() -> Bool {
        return startFen.isEmpty
    }
    
    func createFullMove(_ from: Int, _ dest: Int, _ promotion: Int) -> MoveFull {
        let move = MoveFull(from: from, dest: dest, promotion: promotion)
        if (isPositionValid(pos: from)) {
            move.piece = getPiece(from);
        }
        return move
    }
    
    func findKing(side: Side) -> Int {
        for pos in 0 ..< pieces.count {
            if (isPiece(pos: pos, type: Piece.KING, side: side)) {
                return pos;
            }
        }
        assert(false)
        return getNoPos()
    }
    
    func getNoPos() -> Int {
        return 64
    }
    
    /// Chess
    mutating func setFenCastleRights_clear() {
        castleRights[0] = 0
        castleRights[1] = 0
    }
    
    mutating func clearCastleRights(_ rookPos: Int, _ rookSide: Side) {
        let col = rookPos % 8
        if ((col != castleRights_column_rook_left && col != castleRights_column_rook_right)
            || (rookPos > 7 && rookPos < 56)) {
            return
        }
        let pos = col + (rookSide == Side.white ? 56 : 0)
        if pos != rookPos {
            return;
        }
        let sd = rookSide.rawValue
        if col == castleRights_column_rook_left {
            castleRights[sd] &= ~ChessBoard.CastleRight_long
        } else if col == castleRights_column_rook_right {
            castleRights[sd] &= ~ChessBoard.CastleRight_short
        }
    }
    
    mutating func setFenCastleRights(_ str: String) {
        for ch in str {
            switch (ch) {
            case "K":
                if pieces[60].isPiece(type: PieceTypeStd.king.rawValue, side: Side.white)
                    && pieces[63].isPiece(type: PieceTypeStd.rook.rawValue, side: Side.white) {
                    castleRights[Side.white.rawValue] |= ChessBoard.CastleRight_short
                }
            case "Q":
                if pieces[60].isPiece(type: PieceTypeStd.king.rawValue, side: Side.white)
                    && pieces[56].isPiece(type: PieceTypeStd.rook.rawValue, side: Side.white) {
                    castleRights[Side.white.rawValue] |= ChessBoard.CastleRight_long
                }
            case "k":
                if pieces[4].isPiece(type: PieceTypeStd.king.rawValue, side: Side.black)
                    && pieces[7].isPiece(type: PieceTypeStd.rook.rawValue, side: Side.black) {
                    castleRights[Side.black.rawValue] |= ChessBoard.CastleRight_short
                }
            case "q":
                if pieces[4].isPiece(type: PieceTypeStd.king.rawValue, side: Side.black)
                    && pieces[0].isPiece(type: PieceTypeStd.rook.rawValue, side: Side.black) {
                    castleRights[Side.black.rawValue] |= ChessBoard.CastleRight_long
                }
            default:
                break
            }
        }
    }
    
    mutating func setFen(fen: String) {
        reset()
        
        startFen = fen
        var str = fen
        if (fen.isEmpty) {
            str = originalFen
        } else {
            if fen == originalFen {
                startFen = ""
            }
        }
        
        side = Side.none
        enpassant = -1
        setFenCastleRights_clear();
        
        let vec = str.components(separatedBy: " ")
        let thefen: String = vec[0]
        
        var pos: Int = 0
        for i in 0 ..< 64 {
            let ch: String = thefen[i]
            
            if (ch == "/") {
                continue
            }
            
            if (ch >= "0" && ch <= "8") {
                if let num = Int(ch) {
                    pos = pos + num
                }
                continue
            }
            
            var side = Side.black
            if (ch >= "A" && ch < "Z") {
                side = Side.white
            }
            
            let pieceType = PieceTypeStd.charactor2PieceType(str: ch)
            
            if (pieceType != PieceTypeStd.empty) {
                setPiece(pos: pos, piece: Piece(type: pieceType.rawValue, side: side))
            }
            pos = pos + 1
        }
        
        /// side
        if (vec.count >= 2) {
            let str = vec[1]
            side = str.count > 0 && str[0] == "w" ? Side.white : Side.black
        }
        
        /// castle rights
        if (vec.count >= 3 && vec[2] != "-") {
            let str = vec[2]
            setFenCastleRights(str)
        }
        
        /// enpassant
        if (vec.count >= 4 && vec[3].count >= 2) {
            // enpassant
            let str = vec[3]
            let pos = coordinateStringToPos(str)
            if (isPositionValid(pos: pos)) {
                enpassant = pos
            }
        }
        
        // Half move
        quietCnt = 0;
        if (vec.count >= 5) {
            // half move
            let str = vec[4]
            let k: Int = Int(str) ?? -1
            if (k >= 0 && k <= 50) {
                quietCnt = k * 2
                if side == Side.black {
                    quietCnt = quietCnt + 1
                }
            }
        }
        
        // full move
        if (vec.count >= 6) {
            let str = vec[5]
            let k = Int(str) ?? -1
            if (k >= 1 && k <= 2000) {
                fullMoveCnt = k * 2
            }
        }
        
        //      checkEnpassant();
        hashKey = initHashKey()
        assert(isValid())
    }
    
    
    func posToCoordinateString(_ pos: Int) -> String {
        let row = pos / 8, col = pos % 8
        return columnNames[col] + String(8 - row)
    }
    
    func coordinateStringToPos(_ str: String) -> Int {
        if str.length >= 2 {
            let colChr = str[0], rowChr = str[1]
            if colChr >= "a" && colChr <= "h" && rowChr >= "1" && rowChr <= "8",
               let row = Int(rowChr) {
                let idx = columnNames.firstIndex(of: colChr)
                let col = columnNames.distance(from: columnNames.startIndex, to: idx!)
                return (8 - row) * 8 + col
            }
        }
        return -1
    }
    
    func moveFromCoordiateString(_ moveString: String) -> Move {
        let from = coordinateStringToPos(moveString)
        let dest = coordinateStringToPos(moveString.substring(fromIndex: 2))
        
        var promotion = PieceTypeStd.empty.rawValue
        if moveString.length > 4 {
            var ch = moveString[4]
            if ch == "=" && moveString.length > 5 {
                ch = moveString[5]
            }
            let t = PieceTypeStd.charactor2PieceType(str: ch).rawValue
            if t > PieceTypeStd.king.rawValue && t <= PieceTypeStd.pawn.rawValue {
                promotion = t
            }
        }
        
        return Move(from: from, dest: dest, promotion: promotion)
    }
    
    func moveFromString_castling(_ str: String, _ side: Side) -> Move {
        assert(str == "O-O" || str == "O-O+" || str == "0-0" || str == "O-O-O" || str == "O-O-O+" || str == "0-0-0")
        
        let from = side == Side.black ? 4 : 60
        let dest = from + (str.count < 5 ? 2 : -2)
        return Move(from: from, dest: dest, promotion: Piece.EMPTY)
    }
    
    mutating func moveFromString_san(_ str: String) -> Move {
        if str.count < 2 {
            return Move.illegalMove
        }
        
        if str == "O-O" || str == "O-O+" || str == "0-0" || str == "O-O-O" || str == "O-O-O+" || str == "0-0-0" {
            return moveFromString_castling(str, side)
        }
        
        var s = ""
        let characters = Array(str)
        
        for ch in characters {
            if ch != "+" && ch != "x" && ch != "*" && ch != "#" && ch != "-" && ch != "!" && ch != "?" {
                if ch < " " || ch == "." {
                    s += " "
                } else {
                    s += String(ch)
                }
            }
        }
        
        var from = -1, dest = -1, fromCol = -1, fromRow = -1
        var pieceType = PieceTypeStd.pawn.rawValue
        var promotion = Piece.EMPTY
        
        //      auto p = s.find("=")
        if s.contains("=") {
            let vec = s.split(separator: "=")
            
            if vec.count != 2 || vec[0].isEmpty || vec[1].isEmpty {
                return Move.illegalMove /// something wrong
            }
            
            promotion = PieceTypeStd.charactor2PieceType(str: String(vec[1])).rawValue
            if promotion <= PieceTypeStd.king.rawValue || promotion >= PieceTypeStd.pawn.rawValue {
                return Move.illegalMove
            }
            
            s = String(vec[0])
            if s.count < 2 || promotion == Piece.EMPTY {
                return Move.illegalMove
            }
        }
        
        if s.count < 2 {
            return Move.illegalMove
        }
        
        //    let destString = s.substr(s.count - 2, 2)
        
        let tldEndIndex = s.endIndex
        let tldStartIndex = s.index(tldEndIndex, offsetBy: -2)
        let range = Range(uncheckedBounds: (lower: tldStartIndex, upper: tldEndIndex))
        let destString = String(s[range])
        
        dest = coordinateStringToPos(destString)
        
        if !isPositionValid(pos: dest) {
            return Move.illegalMove
        }
        
        if s.count > 2 {
            var k = 0
            //      let arr = Array(s)
            
            let ch = String(s[0])
            if ch >= "A" && ch <= "Z" {
                k += 1
                pieceType = PieceTypeStd.charactor2PieceType(str: ch).rawValue
                
                if (pieceType == EMPTY) {
                    return Move.illegalMove
                }
            }
            
            let left = s.count - k - 2
            if (left > 0) {
                let tldStartIndex2 = s.index(s.startIndex, offsetBy: k)
                let tldEndIndex2 = s.index(s.startIndex, offsetBy: k + left)
                let range2 = Range(uncheckedBounds: (lower: tldStartIndex2, upper: tldEndIndex2))
                s = String(s[range2])
                
                if (left == 2) {
                    from = coordinateStringToPos(s)
                } else {
                    let ch = s[0]
                    if ch >= "0" && ch <= "9" {
                        fromRow = 8 - (Int(ch) ?? 0)
                    } else if ch >= "a" && ch <= "z" {
                        let idx = columnNames.firstIndex(of: ch)
                        fromCol = columnNames.distance(from: columnNames.startIndex, to: idx!)
                    }
                }
            }
        }
        
        if from < 0 {
            var moveList = [MoveFull]()
            gen(moves: &moveList, side: side)
            
            var goodMoves = [MoveFull]()
            for m in moveList {
                if (m.dest != dest || m.promotion != promotion ||
                    getPiece(m.from).type != pieceType) {
                    continue;
                }
                
                if ((fromRow < 0 && fromCol < 0) ||
                    (fromRow >= 0 && getRank(m.from) == fromRow) ||
                    (fromCol >= 0 && getColumn(m.from) == fromCol)) {
                    goodMoves.append(m)
                    from = m.from
                }
            }
            
            if goodMoves.count > 1 {
                for m in goodMoves {
                    let move = createFullMove(m.from, dest, promotion);
                    var hist = Hist()
                    make(move: move, hist: &hist)
                    let incheck = isIncheck(side)
                    takeBack(hist: hist);
                    if (!incheck) {
                        from = m.from;
                        break;
                    }
                }
            }
        }
        return Move(from: from, dest: dest, promotion: promotion);
    }
    
    func moveString_coordinate(move: Move) -> String {
        var str = posToCoordinateString(move.from) + posToCoordinateString(move.dest)
        if move.promotion != EMPTY, let promotionType = PieceTypeStd(rawValue: move.promotion) {
            str += promotionType.getCharactor().uppercased()
        }
        return str
    }
    
    func toString(_ piece: Piece) -> String {
        let pieceType = PieceTypeStd(rawValue: piece.type)
        var s = pieceType?.getCharactor()
        if piece.side == Side.white {
            s = s?.uppercased()
        }
        return s!
    }
    
    func printBoard() {
        //      stringStream << getFen() << std::endl;
        for i in (0 ..< 64) {
            let piece = getPiece(i)
            
            print(toString(piece), terminator: " ")
            
            if (i > 0 && i % 8 == 7) {
                let row = 8 - i / 8
                print(row)
            }
        }
        
        print("a b c d e f g h  ", side.getName())
        print("key: ", hashKey)
        //
        //      return stringStream.str();
    }
    
    func getColumn(_ pos: Int) -> Int {
        return pos % 8
    }
    
    func getRank(_ pos: Int) -> Int  {
        return pos / 8
    }
    
    func getFenCastleRights() -> String {
        var s = ""
        if (castleRights[0] + castleRights[1] != 0) {
            if ((castleRights[Side.white.rawValue] & ChessBoard.CastleRight_short) != 0) {
                s += "K";
            }
            if ((castleRights[Side.white.rawValue] & ChessBoard.CastleRight_long) != 0) {
                s += "Q";
            }
            if ((castleRights[Side.black.rawValue] & ChessBoard.CastleRight_short) != 0) {
                s += "k";
            }
            if ((castleRights[Side.black.rawValue] & ChessBoard.CastleRight_long) != 0) {
                s += "q";
            }
        } else {
            s = "-";
        }
        
        return s;
    }
    
    func getFen() -> String {
        let k = max(fullMoveCnt, (histList.count + 1) / 2)
        return getFen(halfCount: quietCnt / 2, fullMoveCount: k);
    }
    
    func getFen(halfCount: Int, fullMoveCount: Int) -> String {
        var s = ""
        var e = 0
        
        for i in (0 ..< 64) {
            let piece = getPiece(i)
            if piece.isEmpty() {
                e += 1;
            } else {
                if e != 0 {
                    s += String(e)
                    e = 0
                }
                s += toString(piece)
            }
            
            if i % 8 == 7 {
                if e != 0 {
                    s += String(e)
                }
                if i < 63 {
                    s += "/"
                }
                e = 0
            }
        }
        
        s += (side == Side.white ? " w " : " b ")
        + getFenCastleRights() + " ";
        
        if (enpassant > 0) {
            s += posToCoordinateString(enpassant)
        } else {
            s +=  "-"
        }
        
        s +=  " \(halfCount) \(fullMoveCount)"
        
        return s
    }
    
    
    func isalpha(_ ch: String) -> Bool {
        return (ch >= "a" && ch <= "z") || (ch >= "A" && ch <= "Z")
    }
    
    func isalnum(_ ch: String) -> Bool {
        return (ch >= "0" && ch <= "9") || isalpha(ch)
    }
    
    mutating func fromMoveList(str: String) -> Bool {
        enum State {
            case none, move, comment, evalsym, variant, counter
        }
        
        var st = State.none
        
        var level = 0
        var ok = true;
        
        var moveStringVec = [String]()
        var commentMap = [Int: String]()
        var eSymMap = [Int: String]()
        
        var moveString = "", comment = "", esym = ""
        
        let characters = Array(str)
        
        var i = 0
        var prevch = ""
        while i < characters.count && ok {
            let ch = String(characters[i])
            i += 1
            switch st {
            case State.none:
                if isalpha(ch) {
                    moveString = ch
                    st = State.move
                    //          if ch == "O" {
                    //            print("IAMHERE")
                    //          }
                } else if (ch == "!" || ch == "?") {
                    esym = ch
                    st = State.evalsym;
                } else if (ch == "{") {
                    comment = ""
                    st = State.comment
                } else if (ch == "(") {
                    st = State.variant;
                    level += 1
                } else if ch >= "0" && ch <= "9" {
                    st = State.counter
                }
                break;
                
            case State.move:
                if isalnum(ch) || ch == "=" || ch == "+" || (ch == "-" && (prevch == "O" || prevch == "0")) { // O-O
                    moveString += ch;
                } else {
                    // 17. Qd4 gxf1=Q+
                    if (moveString.count < 2 || moveString.count > 8) {
                        ok = false;
                        break
                    }
                    
                    moveStringVec.append(moveString)
                    moveString = ""
                    
                    i -= 1
                    st = State.none
                }
                break
                
            case State.evalsym:
                if (ch == "!" || ch == "?") {
                    esym += ch
                    break
                }
                eSymMap[moveStringVec.count] = esym
                esym = ""
                
                i -= 1
                st = State.none
                break
                
                
            case State.comment:
                if (ch == "}") {
                    if !comment.isEmpty {
                        let k = moveStringVec.count
                        if let s = commentMap[k] {
                            comment = s + " " + comment
                        }
                        
                        commentMap[k] = comment
                        comment = ""
                    }
                    st = State.none
                    break
                }
                comment += ch
                break
                
            case State.variant:
                if (ch == "(") {
                    level += 1
                } else
                if (ch == ")") {
                    level -= 1
                    if level == 0 {
                        st = State.none
                    }
                }
                break
                
            case State.counter:
                if isalnum(ch) {
                    break
                }
                if ch != "." && ch != ")" {
                    i -= 1
                }
                
                st = State.none
                break
            }
            
            prevch = ch
        }
        
        if (!moveString.isEmpty) {
            moveStringVec.append(moveString)
        }
        
        if (!esym.isEmpty) {
            eSymMap[moveStringVec.count] = esym
        }
        
        if (!comment.isEmpty) {
            commentMap[moveStringVec.count] = comment
        }
        
        for (idx, ss) in moveStringVec.enumerated() {
            
            var move = moveFromString_san(ss)
            if move == Move.illegalMove {
                move = moveFromCoordiateString(ss)
                
                if (move == Move.illegalMove) {
                    return false;
                }
            }
            
            if (!checkMake(from: move.from, dest: move.dest, promotion: move.promotion)) {
                return false;
            }
            
            /// Parse comment before making move for parsing pv
            var parsedComment = false
            var tmphist = Hist()
            if let s = commentMap[idx + 1] {
                //histList.back().comment = it->second;
                parseComment(comment: s, hist: &tmphist)
                parsedComment = true;
            }
            
            assert(!histList.isEmpty)
            if parsedComment {
                let lastHist = histList[histList.count - 1]
                lastHist.comment = tmphist.comment
                lastHist.es = tmphist.es
                histList[histList.count - 1] = lastHist
            }
            
            //      {
            //        auto it = eSymMap.find(i);
            //        if (it != eSymMap.end()) {
            //          auto esym = string2MoveEvaluationSymbol(it->second);
            //          if (esym != MoveEvaluationSymbol::none && !histList.empty()) {
            //            histList.back().mes = esym;
            //          }
            //        }
            //
            //      }
        }
        return true
    }
    
    func parseComment(comment: String, hist: inout Hist) {
        var commentVec = comment.split(separator: " ")
        
        // WDL
        for (i, ss) in commentVec.enumerated() {
            if ss.components(separatedBy: "/").count != 2 {
                continue;
            }
            
            let vec = ss.split(separator: "/");
            if vec.count != 3 {
                continue;
            }
            
            let s1 = vec[0]
            if !s1[s1.startIndex].isNumber {
                continue;
            }
            let s2 = vec[1]
            let s3 = vec[2]
            
            hist.es.stats_win = Int(s1) ?? 0
            hist.es.stats_draw = Int(s2) ?? 0
            hist.es.stats_loss = Int(s3) ?? 0
            
            commentVec.remove(at: i)
            break;
        }
        
        if commentVec.count >= 2 && commentVec[1][commentVec[1].startIndex].isNumber && commentVec[0].contains("/") {
            let ss = commentVec[0]
            let ch = ss[ss.startIndex]
            if ch.isNumber || ch == "M" || ch == "-" || ch == "+" {
                let arr = ss.split(separator: "/")
                let s0 = arr[0]
                let s1 = arr[1]
                
                let arr0 = s0.split(separator: "M")
                
                if arr0.count == 2 && arr0[0].count <= 1 && !arr0[1].isEmpty {
                    let score = Int(arr0[1]) ?? 0
                    hist.es.score = score
                    hist.es.mating = true
                } else {
                    let score = Double(s0) ?? 0.0
                    hist.es.score = Int(score * 100);
                }
                
                hist.es.depth = Int(s1) ?? 0
                hist.es.elapsedInMillisecond = Int(commentVec[1]) ?? 0
                
                //              assert(histList.back().es.score == hist.es.score);
                
                if commentVec.count <= 2 {
                    commentVec.removeAll()
                    return;
                }
                
                commentVec.remove(at: 0) /// delete score/depth
                commentVec.remove(at: 0) /// delete time
                
                let str = commentVec[0]
                let ch = str[str.startIndex]
                if ch.isNumber {
                    hist.es.nodes = UInt64(str) ?? 0
                    commentVec.remove(at: 0) /// delete nodes
                }
                
                if commentVec.count >= 2 && commentVec[0] == ";" {
                    commentVec.remove(at: 0) ///
                } else {
                    commentVec.removeAll()
                }
            }
        }
        var str = ""
        for ss in commentVec {
            if !str.isEmpty {
                str += " ";
            }
            str += ss
        }
        
        hist.comment = str
    }
    
    func toMoveListString(itemPerLine: Int) -> String {
        let moveCounter = true
        
        var str = ""
        var c = 0//, k = 0
        for i in 0 ..< histList.count {
            let hist = histList[i]
            //      if (i & 1) == 0 { //&& hist.move.piece.side == Side.black {
            //        k += 1 // counter should be from event number
            //      }
            
            if c != 0 {
                str += " "
            }
            if moveCounter && (i & 1) == 0 {
                str += "\(1 + i / 2). ";
            }
            
            str += hist.sanString;
            //          switch (notation) {
            //          case Notation.san:
            //                  break;
            //
            //          case Notation.algebraic_coordinate:
            //              default:
            //  //                stringStream << toString(hist.move);
            //
            //                  if (isChessFamily(variant)) {
            //                      str += ChessBoard::moveString_coordinate(hist.move);
            //                  }
            //                  break;
            //}
            
            // Comment
            var haveComment = false;
            let s = hist.computingString()
            if !s.isEmpty {
                haveComment = true;
                str += " {" + s;
            }
            
            if !hist.comment.isEmpty && moveCounter {
                str += haveComment ? "; " : " {"
                haveComment = true;
                str += hist.comment
            }
            
            if (haveComment) {
                str += "} "
            }
            
            c += 1
            if (itemPerLine > 0 && c >= itemPerLine) {
                c = 0
                str += "\n"
            }
        }
        
        return str
    }
    
    func gen_addMove(_ moveList:inout [MoveFull], _ from: Int, _ dest: Int)
    {
        let toSide = getPiece(dest).side
        let movingPiece = getPiece(from)
        let fromSide = movingPiece.side
        
        if fromSide != toSide {
            moveList.append(MoveFull(piece: movingPiece, from: from, dest: dest))
        }
    }
    
    func gen_addPawnMove(_ moveList:inout [MoveFull], _ from: Int, _ dest: Int)
    {
        let toSide = getPiece(dest).side
        let movingPiece = getPiece(from)
        let fromSide = movingPiece.side;
        
        assert(movingPiece.type == PieceTypeStd.pawn.rawValue);
        if (fromSide != toSide) {
            if (dest >= 8 && dest < 56) {
                moveList.append(MoveFull(piece: movingPiece, from: from, dest: dest));
            } else {
                moveList.append(MoveFull(piece: movingPiece, from: from, dest: dest, promotion: PieceTypeStd.queen.rawValue))
                moveList.append(MoveFull(piece: movingPiece, from: from, dest: dest, promotion: PieceTypeStd.rook.rawValue))
                moveList.append(MoveFull(piece: movingPiece, from: from, dest: dest, promotion: PieceTypeStd.bishop.rawValue))
                moveList.append(MoveFull(piece: movingPiece, from: from, dest: dest, promotion: PieceTypeStd.knight.rawValue))
            }
        }
    }
    
    func isIncheck(_ beingAttackedSide: Side) -> Bool {
        let kingPos = findKing(side: beingAttackedSide)
        assert(isPositionValid(pos: kingPos))
        let attackerSide = beingAttackedSide.getXSide()
        return beAttacked(kingPos, attackerSide)
    }
    
    
    func gen(moves:inout [MoveFull], side: Side)  {
        for pos in (0 ..< 64) {
            let piece = pieces[pos]
            
            if (piece.side != side) {
                continue;
            }
            
            switch (piece.type) {
            case PieceTypeStd.king.rawValue:
                genBishop(&moves, side, pos, true)
                genRook(&moves, side, pos, true)
                gen_castling(&moves, pos)
                break;
                
            case PieceTypeStd.queen.rawValue:
                genBishop(&moves, side, pos, false)
                genRook(&moves, side, pos, false)
                break;
                
            case PieceTypeStd.bishop.rawValue:
                genBishop(&moves, side, pos, false)
                break;
                
            case PieceTypeStd.rook.rawValue: /// both queen and rook here
                genRook(&moves, side, pos, false)
                break;
                
            case PieceTypeStd.knight.rawValue:
                genKnight(&moves, side, pos)
                break;
                
            case PieceTypeStd.pawn.rawValue:
                genPawn(&moves, side, pos)
                break;
                
            default:
                break;
            }
        }
    }
    
    func genKnight(_ moves:inout [MoveFull], _ side: Side, _ pos: Int)
    {
        let col = getColumn(pos)
        var y = pos - 6;
        
        if (y >= 0 && col < 6) {
            gen_addMove(&moves, pos, y)
        }
        y = pos - 10;
        if (y >= 0 && col > 1) {
            gen_addMove(&moves, pos, y)
        }
        y = pos - 15;
        if (y >= 0 && col < 7) {
            gen_addMove(&moves, pos, y)
        }
        y = pos - 17;
        if (y >= 0 && col > 0) {
            gen_addMove(&moves, pos, y)
        }
        y = pos + 6;
        if (y < 64 && col > 1) {
            gen_addMove(&moves, pos, y)
        }
        y = pos + 10;
        if (y < 64 && col < 6) {
            gen_addMove(&moves, pos, y)
        }
        y = pos + 15;
        if (y < 64 && col > 0) {
            gen_addMove(&moves, pos, y)
        }
        y = pos + 17;
        if (y < 64 && col < 7) {
            gen_addMove(&moves, pos, y)
        }
    }
    
    func genRook(_ moves:inout [MoveFull], _ side: Side, _ pos: Int, _ oneStep: Bool) {
        let col = getColumn(pos);
        for y in stride(from: pos - 1, through: pos - col, by: -1) { /* go left */
            gen_addMove(&moves, pos, y)
            if (oneStep || !isEmpty(pos: y)) {
                break;
            }
        }
        
        for y in (pos + 1 ..< pos - col + 8) { /* go right */
            gen_addMove(&moves, pos, y);
            if (oneStep || !isEmpty(pos: y)) {
                break;
            }
        }
        
        for y in stride(from: pos - 8, through: 0, by: -8) { /* go up */
            gen_addMove(&moves, pos, y);
            if (oneStep || !isEmpty(pos: y)) {
                break;
            }
        }
        
        for y in stride(from: pos + 8, through: 63, by: +8) { /* go down */
            gen_addMove(&moves, pos, y);
            if (oneStep || !isEmpty(pos: y)) {
                break;
            }
        }
    }
    
    func genBishop(_ moves:inout [MoveFull], _ side: Side, _ pos: Int, _ oneStep: Bool)
    {
        for y in stride(from: pos - 9, through: 0, by: -9) {        /* go left up */
            if getColumn(y) == 7 {
                break
            }
            gen_addMove(&moves, pos, y);
            if (oneStep || !isEmpty(pos: y)) {
                break;
            }
        }
        
        for y in stride(from: pos - 7, through: 0, by: -7) {        /* go right up */
            if getColumn(y) == 0 {
                break
            }
            gen_addMove(&moves, pos, y);
            if (oneStep || !isEmpty(pos: y)) {
                break;
            }
        }
        
        for y in stride(from: pos + 9, through: 63, by: +9) {        /* go right down */
            if getColumn(y) == 0 {
                break
            }
            gen_addMove(&moves, pos, y);
            if (oneStep || !isEmpty(pos: y)) {
                break;
            }
        }
        
        for y in stride(from: pos + 7, through: 63, by: +7) {        /* go right down */
            if getColumn(y) == 7 {
                break
            }
            gen_addMove(&moves, pos, y)
            if (oneStep || !isEmpty(pos: y)) {
                break;
            }
        }
    }
    
    func genPawn(_ moves:inout [MoveFull], _ side: Side, _ pos: Int)
    {
        let col = pos % 8
        
        if (side == Side.black) {
            if (isEmpty(pos: pos + 8)) {
                gen_addPawnMove(&moves, pos, pos + 8)
            }
            if (pos < 16 && isEmpty(pos: pos + 8) && isEmpty(pos: pos + 16)) {
                gen_addMove(&moves, pos, pos + 16)
            }
            
            if (col > 0 && (getPiece(pos + 7).side == Side.white || (pos + 7 == enpassant && getPiece(pos + 7).side == Side.none))) {
                gen_addPawnMove(&moves, pos, pos + 7)
            }
            if (col < 7 && (getPiece(pos + 9).side == Side.white || (pos + 9 == enpassant && getPiece(pos + 9).side == Side.none))) {
                gen_addPawnMove(&moves, pos, pos + 9)
            }
        } else {
            if (isEmpty(pos: pos - 8)) {
                gen_addPawnMove(&moves, pos, pos - 8)
            }
            if (pos >= 48 && isEmpty(pos: pos - 8) && isEmpty(pos: pos - 16)) {
                gen_addMove(&moves, pos, pos - 16)
            }
            
            if (col < 7 && (getPiece(pos - 7).side == Side.black || (pos - 7 == enpassant && getPiece(pos - 7).side == Side.none))) {
                gen_addPawnMove(&moves, pos, pos - 7)
            }
            if (col > 0 && (getPiece(pos - 9).side == Side.black || (pos - 9 == enpassant && getPiece(pos - 9).side == Side.none))) {
                gen_addPawnMove(&moves, pos, pos - 9)
            }
        }
    }
    
    func gen_castling(_ moves:inout [MoveFull], _ kingPos: Int)
    {
        if ((kingPos != 4 || castleRights[Side.black.rawValue] == 0) && (kingPos != 60 || castleRights[Side.white.rawValue] == 0)) {
            return;
        }
        
        if (kingPos == 4) {
            if ((castleRights[Side.black.rawValue] & ChessBoard.CastleRight_long) != 0 &&
                pieces[1].isEmpty() && pieces[2].isEmpty() && pieces[3].isEmpty() &&
                !beAttacked(2, Side.white) && !beAttacked(3, Side.white) && !beAttacked(4, Side.white)) {
                gen_addMove(&moves, 4, 2)
            }
            if ((castleRights[Side.black.rawValue] & ChessBoard.CastleRight_short) != 0 &&
                pieces[5].isEmpty() && pieces[6].isEmpty() &&
                !beAttacked(4, Side.white) && !beAttacked(5, Side.white) && !beAttacked(6, Side.white)) {
                gen_addMove(&moves, 4, 6)
            }
        } else {
            if ((castleRights[Side.white.rawValue] & ChessBoard.CastleRight_long) != 0 &&
                pieces[57].isEmpty() && pieces[58].isEmpty() && pieces[59].isEmpty() &&
                !beAttacked(58, Side.black) && !beAttacked(59, Side.black) && !beAttacked(60, Side.black)) {
                gen_addMove(&moves, 60, 58)
            }
            if ((castleRights[Side.white.rawValue] & ChessBoard.CastleRight_short) != 0 &&
                pieces[61].isEmpty() && pieces[62].isEmpty() &&
                !beAttacked(60, Side.black) && !beAttacked(61, Side.black) && !beAttacked(62, Side.black)) {
                gen_addMove(&moves, 60, 62)
            }
        }
    }
    
    mutating func genLegalOnly(moveList: inout [MoveFull], attackerSide: Side) {
        gen(moves: &moveList, side: attackerSide)
        
        var moves = [MoveFull]()
        var hist = Hist()
        for move in moveList {
            make(move: move, hist: &hist)
            if !isIncheck(attackerSide) {
                moves.append(move)
            }
            takeBack(hist: hist)
        }
        moveList = moves
    }
    
    func beAttacked(_ pos: Int, _ attackerSide: Side) -> Bool {
        let row = getRank(pos), col = getColumn(pos);
        /* Check attacking of Knight */
        if (col > 0 && row > 1 && isPiece(pos: pos - 17, type: PieceTypeStd.knight.rawValue, side: attackerSide)) {
            return true
        }
        if (col < 7 && row > 1 && isPiece(pos: pos - 15, type: PieceTypeStd.knight.rawValue, side: attackerSide)) {
            return true
        }
        if (col > 1 && row > 0 && isPiece(pos: pos - 10, type: PieceTypeStd.knight.rawValue, side: attackerSide)) {
            return true
        }
        if (col < 6 && row > 0 && isPiece(pos: pos - 6, type: PieceTypeStd.knight.rawValue, side: attackerSide)) {
            return true
        }
        if (col > 1 && row < 7 && isPiece(pos: pos + 6, type: PieceTypeStd.knight.rawValue, side: attackerSide)) {
            return true
        }
        if (col < 6 && row < 7 && isPiece(pos: pos + 10, type: PieceTypeStd.knight.rawValue, side: attackerSide)) {
            return true
        }
        if (col > 0 && row < 6 && isPiece(pos: pos + 15, type: PieceTypeStd.knight.rawValue, side: attackerSide)) {
            return true
        }
        if (col < 7 && row < 6 && isPiece(pos: pos + 17, type: PieceTypeStd.knight.rawValue, side: attackerSide)) {
            return true
        }
        
        /* Check horizontal and vertical lines for attacking of Queen, Rook, King */
        /* go down */
        for y in stride(from: pos + 8, through: 63, by: +8) {
            let piece = getPiece(y)
            if (!piece.isEmpty()) {
                if (piece.side == attackerSide) {
                    if (piece.type == PieceTypeStd.queen.rawValue || piece.type == PieceTypeStd.rook.rawValue ||
                        (piece.type == PieceTypeStd.king.rawValue && y == pos + 8)) {
                        return true;
                    }
                }
                break;
            }
        }
        
        /* go up */
        for y in stride(from: pos - 8, through: 0, by: -8) {
            let piece = getPiece(y);
            if (!piece.isEmpty()) {
                if (piece.side == attackerSide) {
                    if (piece.type == PieceTypeStd.queen.rawValue || piece.type == PieceTypeStd.rook.rawValue ||
                        (piece.type == PieceTypeStd.king.rawValue && y == pos - 8)) {
                        return true;
                    }
                }
                break;
            }
        }
        
        /* go left */
        for y in stride(from: pos - 1, through: pos - col, by: -1) {
            let piece = getPiece(y);
            if (!piece.isEmpty()) {
                if (piece.side == attackerSide) {
                    if (piece.type == PieceTypeStd.queen.rawValue || piece.type == PieceTypeStd.rook.rawValue ||
                        (piece.type == PieceTypeStd.king.rawValue && y == pos - 1)) {
                        return true;
                    }
                }
                break;
            }
        }
        
        /* go right */
        for y in pos + 1 ..< pos - col + 8 {
            let piece = getPiece(y);
            if (!piece.isEmpty()) {
                if (piece.side == attackerSide) {
                    if (piece.type == PieceTypeStd.queen.rawValue || piece.type == PieceTypeStd.rook.rawValue ||
                        (piece.type == PieceTypeStd.king.rawValue && y == pos + 1)) {
                        return true;
                    }
                }
                break;
            }
        }
        
        /* Check diagonal lines for attacking of Queen, Bishop, King, Pawn */
        /* go right down */
        for y in stride(from: pos + 9, through: 63, by: +9) {
            if getColumn(y) == 0 {
                break
            }
            let piece = getPiece(y);
            if (!piece.isEmpty()) {
                if (piece.side == attackerSide) {
                    if (piece.type == PieceTypeStd.queen.rawValue || piece.type == PieceTypeStd.bishop.rawValue ||
                        (y == pos + 9 && (piece.type == PieceTypeStd.king.rawValue || (piece.type == PieceTypeStd.pawn.rawValue && piece.side == Side.white)))) {
                        return true;
                    }
                }
                break;
            }
        }
        
        /* go left down */
        for y in stride(from: pos + 7, through: 63, by: +7) {
            if getColumn(y) == 7 {
                break
            }
            let piece = getPiece(y);
            if (!piece.isEmpty()) {
                if (piece.side == attackerSide) {
                    if (piece.type == PieceTypeStd.queen.rawValue || piece.type == PieceTypeStd.bishop.rawValue ||
                        (y == pos + 7 && (piece.type == PieceTypeStd.king.rawValue || (piece.type == PieceTypeStd.pawn.rawValue && piece.side == Side.white)))) {
                        return true;
                    }
                }
                break;
            }
        }
        
        /* go left up */
        for y in stride(from: pos - 9, through: 0, by: -9) {
            if getColumn(y) == 7 {
                break
            }
            let piece = getPiece(y);
            if (!piece.isEmpty()) {
                if (piece.side == attackerSide) {
                    if (piece.type == PieceTypeStd.queen.rawValue || piece.type == PieceTypeStd.bishop.rawValue ||
                        (y == pos - 9 && (piece.type == PieceTypeStd.king.rawValue || (piece.type == PieceTypeStd.pawn.rawValue && piece.side == Side.black)))) {
                        return true;
                    }
                }
                break;
            }
        }
        
        /* go right up */
        for y in stride(from: pos - 7, through: 0, by: -7) {
            if getColumn(y) == 0 {
                break
            }
            let piece = getPiece(y);
            if (!piece.isEmpty()) {
                if (piece.side == attackerSide) {
                    if (piece.type == PieceTypeStd.queen.rawValue || piece.type == PieceTypeStd.bishop.rawValue ||
                        (y == pos - 7 && (piece.type == PieceTypeStd.king.rawValue || (piece.type == PieceTypeStd.pawn.rawValue && piece.side == Side.black)))) {
                        return true;
                    }
                }
                break;
            }
        }
        
        return false;
    }
    
    mutating func make(move: MoveFull, hist: inout Hist) {
        hist.enpassant = enpassant
        hist.status = status
        hist.castleRights[0] = castleRights[0]
        hist.castleRights[1] = castleRights[1]
        hist.castled = 0
        hist.move = move
        hist.cap = getPiece(move.dest)
        hist.hashKey = hashKey
        hist.quietCnt = quietCnt
        
        hashKey ^= hashKeyEnpassant(enpassant)
        
        hashKey ^= xorHashKey(move.from)
        if (!hist.cap.isEmpty()) {
            hashKey ^= xorHashKey(move.dest)
        }
        
        let p = getPiece(move.from)
        setPiece(pos: move.dest, piece: p)
        pieces[move.from].setEmpty()
        
        hashKey ^= xorHashKey(move.dest)
        
        quietCnt += 1
        enpassant = -1;
        
        if ((castleRights[0] + castleRights[1]) != 0 && hist.cap.type == PieceTypeStd.rook.rawValue) {
            clearCastleRights(move.dest, hist.cap.side)
        }
        
        switch (p.type) {
        case PieceTypeStd.king.rawValue:
            castleRights[p.side.rawValue] &= ~(ChessBoard.CastleRight_long|ChessBoard.CastleRight_short)
            if (abs(move.from - move.dest) == 2) { // castle
                let rookPos = move.from + (move.from < move.dest ? 3 : -4);
                assert(pieces[rookPos].type == PieceTypeStd.rook.rawValue);
                let newRookPos = (move.from + move.dest) / 2;
                assert(pieces[newRookPos].isEmpty());
                
                hashKey ^= xorHashKey(rookPos);
                setPiece(pos: newRookPos, piece: getPiece(rookPos))
                setEmpty(pos: rookPos);
                hashKey ^= xorHashKey(newRookPos);
                quietCnt = 0;
                hist.castled = move.dest == 2 || move.dest == 56 + 2 ? ChessBoard.CastleRight_long : ChessBoard.CastleRight_short;
            }
            break;
            
        case PieceTypeStd.rook.rawValue:
            if (castleRights[0] + castleRights[1] != 0) {
                clearCastleRights(move.from, p.side);
            }
            break;
            
        case PieceTypeStd.pawn.rawValue:
            let d = abs(move.from - move.dest)
            quietCnt = 0
            
            if (d == 16) {
                enpassant = (move.from + move.dest) / 2
            } else if move.dest == hist.enpassant {
                let ep = move.dest + (p.side == Side.white ? +8 : -8);
                hist.cap = getPiece(ep)
                assert(!hist.cap.isEmpty())
                
                hashKey ^= xorHashKey(ep)
                pieces[ep].setEmpty();
            } else {
                if (move.promotion != EMPTY) {
                    hashKey ^= xorHashKey(move.dest);
                    pieces[move.dest].type = move.promotion;
                    hashKey ^= xorHashKey(move.dest);
                }
            }
            break;
        default:
            break;
        }
        
        if (!hist.cap.isEmpty()) {
            quietCnt = 0;
        }
        
        if (hist.castleRights[Side.white.rawValue] != castleRights[Side.white.rawValue]) {
            if ((hist.castleRights[Side.white.rawValue] & ChessBoard.CastleRight_short) != (castleRights[Side.white.rawValue] & ChessBoard.CastleRight_short)) {
                hashKey ^= polyglotRandom64[RandomCastle+0]
                quietCnt = 0;
            }
            if ((hist.castleRights[Side.white.rawValue] & ChessBoard.CastleRight_long) != (castleRights[Side.white.rawValue] & ChessBoard.CastleRight_long)) {
                hashKey ^= polyglotRandom64[RandomCastle+1]
                quietCnt = 0;
            }
        }
        if (hist.castleRights[Side.black.rawValue] != castleRights[Side.black.rawValue]) {
            if ((hist.castleRights[Side.black.rawValue] & ChessBoard.CastleRight_short) != (castleRights[Side.black.rawValue] & ChessBoard.CastleRight_short)) {
                hashKey ^= polyglotRandom64[RandomCastle+2]
                quietCnt = 0;
            }
            if ((hist.castleRights[Side.black.rawValue] & ChessBoard.CastleRight_long) != (castleRights[Side.black.rawValue] & ChessBoard.CastleRight_long)) {
                hashKey ^= polyglotRandom64[RandomCastle+3]
                quietCnt = 0;
            }
        }
        
        hashKey ^= hashKeyEnpassant(enpassant);
    }
    
    mutating func takeBack(hist: Hist) {
        let movep = getPiece(hist.move.dest)
        setPiece(pos: hist.move.from, piece: movep);
        
        var capPos = hist.move.dest
        
        if (movep.type == PieceTypeStd.pawn.rawValue && hist.enpassant == hist.move.dest) {
            capPos = hist.move.dest + (movep.side == Side.white ? +8 : -8);
            setEmpty(pos: hist.move.dest);
        }
        setPiece(pos: capPos, piece: hist.cap);
        
        if movep.type == PieceTypeStd.king.rawValue {
            if (abs(hist.move.from - hist.move.dest) == 2) {
                assert(hist.castled == ChessBoard.CastleRight_long || hist.castled == ChessBoard.CastleRight_short);
                let rookPos = hist.move.from + (hist.move.from < hist.move.dest ? 3 : -4);
                assert(isEmpty(pos: rookPos));
                let newRookPos = (hist.move.from + hist.move.dest) / 2;
                setPiece(pos: rookPos, piece: Piece(type: PieceTypeStd.rook.rawValue, side: hist.move.dest < 8 ? Side.black : Side.white));
                setEmpty(pos: newRookPos);
            }
        }
        
        if (hist.move.promotion != EMPTY) {
            setPiece(pos: hist.move.from, piece: Piece(type: PieceTypeStd.pawn.rawValue, side: hist.move.dest < 8 ? Side.white : Side.black));
        }
        
        status = hist.status;
        castleRights[0] = hist.castleRights[0];
        castleRights[1] = hist.castleRights[1];
        enpassant = hist.enpassant;
        quietCnt = hist.quietCnt;
        
        //    pieceList_takeback(hist);
        hashKey = hist.hashKey;
    }
    
    mutating func make(move: MoveFull) {
        var hist = Hist()
        make(move: move, hist: &hist);
        histList.append(hist)
        side = side.getXSide()
        
        hashKey ^= xorSideHashKey()
        assert(istHashKeyValid())
    }
    
    mutating func takeBack() {
        if let hist = histList.last {
            histList.remove(at: histList.count - 1)
            takeBack(hist: hist)
            side = side.getXSide()
            assert(istHashKeyValid());
        }
    }
    
    mutating func perft(depth: Int, ply: Int = 0) -> UInt64 {
        if depth == 0 {
            return 1
        }
        
        assert(istHashKeyValid())
        var nodes: UInt64 = 0
        
        var moveList = [MoveFull]()
        gen(moves: &moveList, side: side)
        
        let theSide = side
        for move in moveList {
            make(move: move)
            assert(theSide != side)
            if !isIncheck(theSide) {
                let n = perft(depth: depth - 1, ply: ply + 1)
                nodes += n;
                
                if (ply == 0) {
                    print(moveString_coordinate(move: move) + ":", n)
                }
            }
            takeBack()
            assert(theSide == side)
        }
        return nodes
    }
    
    mutating func rule() -> Result {
        let result = Result()
        
        // Mated or stalemate
        var haveLegalMove = false;
        var moveList = [MoveFull]()
        gen(moves: &moveList, side: side)
        
        var hist = Hist()
        for move in moveList {
            make(move: move, hist: &hist);
            if (!isIncheck(side)) {
                haveLegalMove = true;
            }
            takeBack(hist: hist)
            if (haveLegalMove) {
                break
            }
        }
        
        if (!haveLegalMove) {
            if (isIncheck(side)) {
                result.result = side == Side.white ? ResultType.loss : ResultType.win;
                result.reason = ReasonType.mate;
            } else {
                result.result = ResultType.draw;
                result.reason = ReasonType.stalemate;
            }
            return result;
        }
        
        /// draw by insufficientmaterial
        var pieceCnt = Array(repeating: Array(repeating: 0, count: 8), count: 2)
        var bishopColor = Array(repeating: Array(repeating: 0, count: 2), count: 2)
        
        var draw = true;
        for i in 0 ..< 64 {
            let p = pieces[i];
            if p.type <= Piece.KING {
                continue
            }
            if (p.type != PieceTypeStd.bishop.rawValue &&
                p.type != PieceTypeStd.knight.rawValue) {
                draw = false;
                break;
            }
            
            let sd = p.side.rawValue
            let t = p.type;
            pieceCnt[sd][t] += 1
            
            if (p.type == PieceTypeStd.bishop.rawValue) {
                let c = (getColumn(i) + getRank(i)) & 1;
                bishopColor[sd][c] += 1
                if (bishopColor[sd][0] != 0 && bishopColor[sd][1] != 0) || pieceCnt[sd][PieceTypeStd.knight.rawValue] != 0 { /// bishops in different colors or a knight + a bishop
                    draw = false;
                    break;
                }
            } else if pieceCnt[sd][t] > 0 {
                if (pieceCnt[sd][t] > 1 || pieceCnt[sd][PieceTypeStd.bishop.rawValue] != 0) { // more than 2 knights or a knight + a bishop
                    draw = false
                    break;
                }
            }
        }
        
        if (draw) {
            result.result = ResultType.draw;
            result.reason = ReasonType.insufficientmaterial;
            return result;
        }
        
        /// 50 moves
        if (quietCnt >= 50 * 2) {
            result.result = ResultType.draw;
            result.reason = ReasonType.fiftymoves;
            //        std::cout << "************* DRAW BY 50 MOVES *************" << std::endl;
            return result;
        }
        
        if (quietCnt >= 2 * 4) {
            var cnt = 0;
            var i = histList.count, k = i - quietCnt;
            i -= 2;
            while (i >= 0 && i >= k) {
                let hist = histList[i]
                if (hist.hashKey == hashKey) {
                    cnt += 1
                }
                i -= 2
            }
            if (cnt >= 2) { // 2 + 1 itself = 3 times
                result.result = ResultType.draw;
                result.reason = ReasonType.repetition;
                return result;
            }
        }
        
        return result;
    }
    
    // Check and make the move if it is legal
    mutating func checkMake(from: Int, dest: Int, promotion: Int) -> Bool {
        assert(promotion >= Piece.EMPTY)
        //    assert(istHashKeyValid())
        if !isPositionValid(pos: from) || !isPositionValid(pos: dest) || from == dest {
            return false
        }
        
        let piece = getPiece(from)
        var cap = getPiece(dest)
        
        
        /// Castling move by dropping King into Rook
        var _dest = dest
        if piece.side == cap.side && piece.type == Piece.KING && cap.type == PieceTypeStd.rook.rawValue
            && (from == 4 || from == 60) && from / 8 == dest / 8 {
            _dest = dest > from ? (from + 2) : (from - 2)
            cap = Piece.emptyPiece
        }
        
        if piece.isEmpty()
            || piece.side != side
            || piece.side == cap.side /// (piece.side == cap.side && (piece.type != Piece.KING || cap.type != PieceTypeStd.rook.rawValue))
            || !Move.isValidPromotion(promotion: promotion) {
            return false;
        }
        
        var moveList = [MoveFull]()
        gen(moves: &moveList, side: side);
        
        for move in moveList {
            assert(istHashKeyValid());
            if (move.from != from || move.dest != _dest || move.promotion != promotion) {
                continue;
            }
            
            let theSide = side;
            let fullmove = createFullMove(from, _dest, promotion)
            make(move: fullmove)
            assert(side != theSide)
            
            if (isIncheck(theSide)) {
                takeBack()
                return false;
            }
            
            _ = createStringForLastMove(&moveList)
            assert(isValid())
            return true;
        }
        
        return false;
    }
    
    mutating func createStringForLastMove(_ moveList:inout [MoveFull]) -> Bool {
        if (histList.isEmpty) {
            return false;
        }
        
        assert(istHashKeyValid());
        
        let hist = histList.last!
        
        let movePiece = hist.move.piece
        if (movePiece.isEmpty()) {
            return false; // something wrong
        }
        
        var str = ""
        
        // special cases - castling moves
        if movePiece.type == PieceTypeStd.king.rawValue && hist.castled > 0 {
            //        let col = hist.move.dest % 8;
            assert(hist.castled == ChessBoard.CastleRight_long || hist.castled == ChessBoard.CastleRight_short);
            str = hist.castled == ChessBoard.CastleRight_long ? "O-O-O" : "O-O";
        } else {
            var ambi = false, sameCol = false, sameRow = false;
            
            if movePiece.type != PieceTypeStd.king.rawValue {
                for m in moveList {
                    if (m.dest == hist.move.dest
                        && m.from != hist.move.from
                        && pieces[m.from].type == movePiece.type) {
                        ambi = true;
                        if (m.from / 8 == hist.move.from / 8) {
                            sameRow = true;
                        }
                        if (m.from % 8 == hist.move.from % 8) {
                            sameCol = true;
                        }
                    }
                }
            }
            
            if (movePiece.type != PieceTypeStd.pawn.rawValue) {
                //            str = char(pieceTypeName[static_cast<int>(movePiece.type)] - 'a' + 'A');
                if let pieceType = PieceTypeStd(rawValue: movePiece.type) {
                    str = pieceType.getCharactor().uppercased()
                }
            }
            if (ambi) {
                if (sameCol && sameRow) {
                    str += posToCoordinateString(hist.move.from)
                } else if (sameCol) {
                    str += "\(8 - hist.move.from / 8)"
                } else {
                    str += columnNames[hist.move.from % 8]
                }
            }
            
            if (!hist.cap.isEmpty()) {
                // When a pawn makes a capture, the file from which the pawn departed is used to
                // identify the pawn. For example, exd5
                if (str.isEmpty && movePiece.type == PieceTypeStd.pawn.rawValue) {
                    str += columnNames[hist.move.from % 8]
                }
                str += "x";
            }
            
            str += posToCoordinateString(hist.move.dest);
            
            // promotion
            if (hist.move.promotion != PieceTypeStd.empty.rawValue) {
                str += "=";
                //            str += char(pieceTypeName[static_cast<int>(hist.move.promotion)] - 'a' + 'A');
                if let promotionType = PieceTypeStd(rawValue: hist.move.promotion) {
                    str += promotionType.getCharactor().uppercased()
                }
            }
        }
        
        
        // incheck
        if (isIncheck(side)) {
            assert(istHashKeyValid());
            
            var moveList = [MoveFull]()
            genLegalOnly(moveList: &moveList, attackerSide: side)
            str += moveList.isEmpty ? "#" : "+";
        }
        
        //    hist.sanString = str
        histList[histList.count - 1].sanString = str
        return true;
    }
    
    func xorHashKey(_ pos: Int)  -> UInt64 {
        assert(isPositionValid(pos: pos))
        assert(!pieces[pos].isEmpty())
        
        let sd = pieces[pos].side.rawValue
        let kind_of_piece = (6 - pieces[pos].type) * 2 + sd; assert(kind_of_piece >= 0 && kind_of_piece <= 11);
        
        let file = getColumn(pos), row = 7 - getRank(pos);
        let offset_piece = 64 * kind_of_piece + 8 * row + file;
        
        return polyglotRandom64[offset_piece]
    }
    
    func hashKeyEnpassant(_ enpassant: Int)  -> UInt64 {
        var key: UInt64 = 0
        
        if (enpassant > 0) {
            
            let col = getColumn(enpassant), row = getRank(enpassant);
            var ok = false
            if (row == 2) {
                ok = (col > 0 && isPiece(pos: enpassant + 7, type: PieceTypeStd.pawn.rawValue, side: Side.white))
                || (col < 7 && isPiece(pos: enpassant + 9, type: PieceTypeStd.pawn.rawValue, side: Side.white));
            } else {
                ok = (col > 0 && isPiece(pos: enpassant - 9, type: PieceTypeStd.pawn.rawValue, side: Side.black))
                || (col < 7 && isPiece(pos: enpassant - 7, type: PieceTypeStd.pawn.rawValue, side: Side.black));
            }
            if ok {
                key ^= polyglotRandom64[RandomEnPassant + col]
            }
        }
        
        return key;
    }
    
    let RandomCastle    = 768
    let RandomEnPassant = 772
    let RandomTurn      = 780
    
    func xorSideHashKey() -> UInt64 {
        return polyglotRandom64[RandomTurn]
    }
    
    func initHashKey() -> UInt64 {
        var key: UInt64 = 0
        for i in 0 ..< pieces.count {
            if (!pieces[i].isEmpty()) {
                key ^= xorHashKey(i);
            }
        }
        
        if (side == Side.white) {
            key ^= polyglotRandom64[RandomTurn]
        }
        
        if ((castleRights[0] | castleRights[1]) != 0) {
            if ((castleRights[Side.white.rawValue] & ChessBoard.CastleRight_short) != 0) {
                key ^= polyglotRandom64[RandomCastle + 0]
            }
            if ((castleRights[Side.white.rawValue] & ChessBoard.CastleRight_long) != 0) {
                key ^= polyglotRandom64[RandomCastle + 1]
            }
            if ((castleRights[Side.black.rawValue] & ChessBoard.CastleRight_short) != 0) {
                key ^= polyglotRandom64[RandomCastle + 2]
            }
            if ((castleRights[Side.black.rawValue] & ChessBoard.CastleRight_long) != 0) {
                key ^= polyglotRandom64[RandomCastle + 3]
            }
        }
        
        key ^= hashKeyEnpassant(enpassant);
        return key;
    }
    
    func istHashKeyValid() -> Bool {
        return hashKey == initHashKey()
    }
    
    // http://hardy.uhasselt.be/Toga/book_format.html
    
    let polyglotRandom64 : [UInt64] = [
        0x9D39247E33776D41, 0x2AF7398005AAA5C7, 0x44DB015024623547, 0x9C15F73E62A76AE2,
        0x75834465489C0C89, 0x3290AC3A203001BF, 0x0FBBAD1F61042279, 0xE83A908FF2FB60CA,
        0x0D7E765D58755C10, 0x1A083822CEAFE02D, 0x9605D5F0E25EC3B0, 0xD021FF5CD13A2ED5,
        0x40BDF15D4A672E32, 0x011355146FD56395, 0x5DB4832046F3D9E5, 0x239F8B2D7FF719CC,
        0x05D1A1AE85B49AA1, 0x679F848F6E8FC971, 0x7449BBFF801FED0B, 0x7D11CDB1C3B7ADF0,
        0x82C7709E781EB7CC, 0xF3218F1C9510786C, 0x331478F3AF51BBE6, 0x4BB38DE5E7219443,
        0xAA649C6EBCFD50FC, 0x8DBD98A352AFD40B, 0x87D2074B81D79217, 0x19F3C751D3E92AE1,
        0xB4AB30F062B19ABF, 0x7B0500AC42047AC4, 0xC9452CA81A09D85D, 0x24AA6C514DA27500,
        0x4C9F34427501B447, 0x14A68FD73C910841, 0xA71B9B83461CBD93, 0x03488B95B0F1850F,
        0x637B2B34FF93C040, 0x09D1BC9A3DD90A94, 0x3575668334A1DD3B, 0x735E2B97A4C45A23,
        0x18727070F1BD400B, 0x1FCBACD259BF02E7, 0xD310A7C2CE9B6555, 0xBF983FE0FE5D8244,
        0x9F74D14F7454A824, 0x51EBDC4AB9BA3035, 0x5C82C505DB9AB0FA, 0xFCF7FE8A3430B241,
        0x3253A729B9BA3DDE, 0x8C74C368081B3075, 0xB9BC6C87167C33E7, 0x7EF48F2B83024E20,
        0x11D505D4C351BD7F, 0x6568FCA92C76A243, 0x4DE0B0F40F32A7B8, 0x96D693460CC37E5D,
        0x42E240CB63689F2F, 0x6D2BDCDAE2919661, 0x42880B0236E4D951, 0x5F0F4A5898171BB6,
        0x39F890F579F92F88, 0x93C5B5F47356388B, 0x63DC359D8D231B78, 0xEC16CA8AEA98AD76,
        0x5355F900C2A82DC7, 0x07FB9F855A997142, 0x5093417AA8A7ED5E, 0x7BCBC38DA25A7F3C,
        0x19FC8A768CF4B6D4, 0x637A7780DECFC0D9, 0x8249A47AEE0E41F7, 0x79AD695501E7D1E8,
        0x14ACBAF4777D5776, 0xF145B6BECCDEA195, 0xDABF2AC8201752FC, 0x24C3C94DF9C8D3F6,
        0xBB6E2924F03912EA, 0x0CE26C0B95C980D9, 0xA49CD132BFBF7CC4, 0xE99D662AF4243939,
        0x27E6AD7891165C3F, 0x8535F040B9744FF1, 0x54B3F4FA5F40D873, 0x72B12C32127FED2B,
        0xEE954D3C7B411F47, 0x9A85AC909A24EAA1, 0x70AC4CD9F04F21F5, 0xF9B89D3E99A075C2,
        0x87B3E2B2B5C907B1, 0xA366E5B8C54F48B8, 0xAE4A9346CC3F7CF2, 0x1920C04D47267BBD,
        0x87BF02C6B49E2AE9, 0x092237AC237F3859, 0xFF07F64EF8ED14D0, 0x8DE8DCA9F03CC54E,
        0x9C1633264DB49C89, 0xB3F22C3D0B0B38ED, 0x390E5FB44D01144B, 0x5BFEA5B4712768E9,
        0x1E1032911FA78984, 0x9A74ACB964E78CB3, 0x4F80F7A035DAFB04, 0x6304D09A0B3738C4,
        0x2171E64683023A08, 0x5B9B63EB9CEFF80C, 0x506AACF489889342, 0x1881AFC9A3A701D6,
        0x6503080440750644, 0xDFD395339CDBF4A7, 0xEF927DBCF00C20F2, 0x7B32F7D1E03680EC,
        0xB9FD7620E7316243, 0x05A7E8A57DB91B77, 0xB5889C6E15630A75, 0x4A750A09CE9573F7,
        0xCF464CEC899A2F8A, 0xF538639CE705B824, 0x3C79A0FF5580EF7F, 0xEDE6C87F8477609D,
        0x799E81F05BC93F31, 0x86536B8CF3428A8C, 0x97D7374C60087B73, 0xA246637CFF328532,
        0x043FCAE60CC0EBA0, 0x920E449535DD359E, 0x70EB093B15B290CC, 0x73A1921916591CBD,
        0x56436C9FE1A1AA8D, 0xEFAC4B70633B8F81, 0xBB215798D45DF7AF, 0x45F20042F24F1768,
        0x930F80F4E8EB7462, 0xFF6712FFCFD75EA1, 0xAE623FD67468AA70, 0xDD2C5BC84BC8D8FC,
        0x7EED120D54CF2DD9, 0x22FE545401165F1C, 0xC91800E98FB99929, 0x808BD68E6AC10365,
        0xDEC468145B7605F6, 0x1BEDE3A3AEF53302, 0x43539603D6C55602, 0xAA969B5C691CCB7A,
        0xA87832D392EFEE56, 0x65942C7B3C7E11AE, 0xDED2D633CAD004F6, 0x21F08570F420E565,
        0xB415938D7DA94E3C, 0x91B859E59ECB6350, 0x10CFF333E0ED804A, 0x28AED140BE0BB7DD,
        0xC5CC1D89724FA456, 0x5648F680F11A2741, 0x2D255069F0B7DAB3, 0x9BC5A38EF729ABD4,
        0xEF2F054308F6A2BC, 0xAF2042F5CC5C2858, 0x480412BAB7F5BE2A, 0xAEF3AF4A563DFE43,
        0x19AFE59AE451497F, 0x52593803DFF1E840, 0xF4F076E65F2CE6F0, 0x11379625747D5AF3,
        0xBCE5D2248682C115, 0x9DA4243DE836994F, 0x066F70B33FE09017, 0x4DC4DE189B671A1C,
        0x51039AB7712457C3, 0xC07A3F80C31FB4B4, 0xB46EE9C5E64A6E7C, 0xB3819A42ABE61C87,
        0x21A007933A522A20, 0x2DF16F761598AA4F, 0x763C4A1371B368FD, 0xF793C46702E086A0,
        0xD7288E012AEB8D31, 0xDE336A2A4BC1C44B, 0x0BF692B38D079F23, 0x2C604A7A177326B3,
        0x4850E73E03EB6064, 0xCFC447F1E53C8E1B, 0xB05CA3F564268D99, 0x9AE182C8BC9474E8,
        0xA4FC4BD4FC5558CA, 0xE755178D58FC4E76, 0x69B97DB1A4C03DFE, 0xF9B5B7C4ACC67C96,
        0xFC6A82D64B8655FB, 0x9C684CB6C4D24417, 0x8EC97D2917456ED0, 0x6703DF9D2924E97E,
        0xC547F57E42A7444E, 0x78E37644E7CAD29E, 0xFE9A44E9362F05FA, 0x08BD35CC38336615,
        0x9315E5EB3A129ACE, 0x94061B871E04DF75, 0xDF1D9F9D784BA010, 0x3BBA57B68871B59D,
        0xD2B7ADEEDED1F73F, 0xF7A255D83BC373F8, 0xD7F4F2448C0CEB81, 0xD95BE88CD210FFA7,
        0x336F52F8FF4728E7, 0xA74049DAC312AC71, 0xA2F61BB6E437FDB5, 0x4F2A5CB07F6A35B3,
        0x87D380BDA5BF7859, 0x16B9F7E06C453A21, 0x7BA2484C8A0FD54E, 0xF3A678CAD9A2E38C,
        0x39B0BF7DDE437BA2, 0xFCAF55C1BF8A4424, 0x18FCF680573FA594, 0x4C0563B89F495AC3,
        0x40E087931A00930D, 0x8CFFA9412EB642C1, 0x68CA39053261169F, 0x7A1EE967D27579E2,
        0x9D1D60E5076F5B6F, 0x3810E399B6F65BA2, 0x32095B6D4AB5F9B1, 0x35CAB62109DD038A,
        0xA90B24499FCFAFB1, 0x77A225A07CC2C6BD, 0x513E5E634C70E331, 0x4361C0CA3F692F12,
        0xD941ACA44B20A45B, 0x528F7C8602C5807B, 0x52AB92BEB9613989, 0x9D1DFA2EFC557F73,
        0x722FF175F572C348, 0x1D1260A51107FE97, 0x7A249A57EC0C9BA2, 0x04208FE9E8F7F2D6,
        0x5A110C6058B920A0, 0x0CD9A497658A5698, 0x56FD23C8F9715A4C, 0x284C847B9D887AAE,
        0x04FEABFBBDB619CB, 0x742E1E651C60BA83, 0x9A9632E65904AD3C, 0x881B82A13B51B9E2,
        0x506E6744CD974924, 0xB0183DB56FFC6A79, 0x0ED9B915C66ED37E, 0x5E11E86D5873D484,
        0xF678647E3519AC6E, 0x1B85D488D0F20CC5, 0xDAB9FE6525D89021, 0x0D151D86ADB73615,
        0xA865A54EDCC0F019, 0x93C42566AEF98FFB, 0x99E7AFEABE000731, 0x48CBFF086DDF285A,
        0x7F9B6AF1EBF78BAF, 0x58627E1A149BBA21, 0x2CD16E2ABD791E33, 0xD363EFF5F0977996,
        0x0CE2A38C344A6EED, 0x1A804AADB9CFA741, 0x907F30421D78C5DE, 0x501F65EDB3034D07,
        0x37624AE5A48FA6E9, 0x957BAF61700CFF4E, 0x3A6C27934E31188A, 0xD49503536ABCA345,
        0x088E049589C432E0, 0xF943AEE7FEBF21B8, 0x6C3B8E3E336139D3, 0x364F6FFA464EE52E,
        0xD60F6DCEDC314222, 0x56963B0DCA418FC0, 0x16F50EDF91E513AF, 0xEF1955914B609F93,
        0x565601C0364E3228, 0xECB53939887E8175, 0xBAC7A9A18531294B, 0xB344C470397BBA52,
        0x65D34954DAF3CEBD, 0xB4B81B3FA97511E2, 0xB422061193D6F6A7, 0x071582401C38434D,
        0x7A13F18BBEDC4FF5, 0xBC4097B116C524D2, 0x59B97885E2F2EA28, 0x99170A5DC3115544,
        0x6F423357E7C6A9F9, 0x325928EE6E6F8794, 0xD0E4366228B03343, 0x565C31F7DE89EA27,
        0x30F5611484119414, 0xD873DB391292ED4F, 0x7BD94E1D8E17DEBC, 0xC7D9F16864A76E94,
        0x947AE053EE56E63C, 0xC8C93882F9475F5F, 0x3A9BF55BA91F81CA, 0xD9A11FBB3D9808E4,
        0x0FD22063EDC29FCA, 0xB3F256D8ACA0B0B9, 0xB03031A8B4516E84, 0x35DD37D5871448AF,
        0xE9F6082B05542E4E, 0xEBFAFA33D7254B59, 0x9255ABB50D532280, 0xB9AB4CE57F2D34F3,
        0x693501D628297551, 0xC62C58F97DD949BF, 0xCD454F8F19C5126A, 0xBBE83F4ECC2BDECB,
        0xDC842B7E2819E230, 0xBA89142E007503B8, 0xA3BC941D0A5061CB, 0xE9F6760E32CD8021,
        0x09C7E552BC76492F, 0x852F54934DA55CC9, 0x8107FCCF064FCF56, 0x098954D51FFF6580,
        0x23B70EDB1955C4BF, 0xC330DE426430F69D, 0x4715ED43E8A45C0A, 0xA8D7E4DAB780A08D,
        0x0572B974F03CE0BB, 0xB57D2E985E1419C7, 0xE8D9ECBE2CF3D73F, 0x2FE4B17170E59750,
        0x11317BA87905E790, 0x7FBF21EC8A1F45EC, 0x1725CABFCB045B00, 0x964E915CD5E2B207,
        0x3E2B8BCBF016D66D, 0xBE7444E39328A0AC, 0xF85B2B4FBCDE44B7, 0x49353FEA39BA63B1,
        0x1DD01AAFCD53486A, 0x1FCA8A92FD719F85, 0xFC7C95D827357AFA, 0x18A6A990C8B35EBD,
        0xCCCB7005C6B9C28D, 0x3BDBB92C43B17F26, 0xAA70B5B4F89695A2, 0xE94C39A54A98307F,
        0xB7A0B174CFF6F36E, 0xD4DBA84729AF48AD, 0x2E18BC1AD9704A68, 0x2DE0966DAF2F8B1C,
        0xB9C11D5B1E43A07E, 0x64972D68DEE33360, 0x94628D38D0C20584, 0xDBC0D2B6AB90A559,
        0xD2733C4335C6A72F, 0x7E75D99D94A70F4D, 0x6CED1983376FA72B, 0x97FCAACBF030BC24,
        0x7B77497B32503B12, 0x8547EDDFB81CCB94, 0x79999CDFF70902CB, 0xCFFE1939438E9B24,
        0x829626E3892D95D7, 0x92FAE24291F2B3F1, 0x63E22C147B9C3403, 0xC678B6D860284A1C,
        0x5873888850659AE7, 0x0981DCD296A8736D, 0x9F65789A6509A440, 0x9FF38FED72E9052F,
        0xE479EE5B9930578C, 0xE7F28ECD2D49EECD, 0x56C074A581EA17FE, 0x5544F7D774B14AEF,
        0x7B3F0195FC6F290F, 0x12153635B2C0CF57, 0x7F5126DBBA5E0CA7, 0x7A76956C3EAFB413,
        0x3D5774A11D31AB39, 0x8A1B083821F40CB4, 0x7B4A38E32537DF62, 0x950113646D1D6E03,
        0x4DA8979A0041E8A9, 0x3BC36E078F7515D7, 0x5D0A12F27AD310D1, 0x7F9D1A2E1EBE1327,
        0xDA3A361B1C5157B1, 0xDCDD7D20903D0C25, 0x36833336D068F707, 0xCE68341F79893389,
        0xAB9090168DD05F34, 0x43954B3252DC25E5, 0xB438C2B67F98E5E9, 0x10DCD78E3851A492,
        0xDBC27AB5447822BF, 0x9B3CDB65F82CA382, 0xB67B7896167B4C84, 0xBFCED1B0048EAC50,
        0xA9119B60369FFEBD, 0x1FFF7AC80904BF45, 0xAC12FB171817EEE7, 0xAF08DA9177DDA93D,
        0x1B0CAB936E65C744, 0xB559EB1D04E5E932, 0xC37B45B3F8D6F2BA, 0xC3A9DC228CAAC9E9,
        0xF3B8B6675A6507FF, 0x9FC477DE4ED681DA, 0x67378D8ECCEF96CB, 0x6DD856D94D259236,
        0xA319CE15B0B4DB31, 0x073973751F12DD5E, 0x8A8E849EB32781A5, 0xE1925C71285279F5,
        0x74C04BF1790C0EFE, 0x4DDA48153C94938A, 0x9D266D6A1CC0542C, 0x7440FB816508C4FE,
        0x13328503DF48229F, 0xD6BF7BAEE43CAC40, 0x4838D65F6EF6748F, 0x1E152328F3318DEA,
        0x8F8419A348F296BF, 0x72C8834A5957B511, 0xD7A023A73260B45C, 0x94EBC8ABCFB56DAE,
        0x9FC10D0F989993E0, 0xDE68A2355B93CAE6, 0xA44CFE79AE538BBE, 0x9D1D84FCCE371425,
        0x51D2B1AB2DDFB636, 0x2FD7E4B9E72CD38C, 0x65CA5B96B7552210, 0xDD69A0D8AB3B546D,
        0x604D51B25FBF70E2, 0x73AA8A564FB7AC9E, 0x1A8C1E992B941148, 0xAAC40A2703D9BEA0,
        0x764DBEAE7FA4F3A6, 0x1E99B96E70A9BE8B, 0x2C5E9DEB57EF4743, 0x3A938FEE32D29981,
        0x26E6DB8FFDF5ADFE, 0x469356C504EC9F9D, 0xC8763C5B08D1908C, 0x3F6C6AF859D80055,
        0x7F7CC39420A3A545, 0x9BFB227EBDF4C5CE, 0x89039D79D6FC5C5C, 0x8FE88B57305E2AB6,
        0xA09E8C8C35AB96DE, 0xFA7E393983325753, 0xD6B6D0ECC617C699, 0xDFEA21EA9E7557E3,
        0xB67C1FA481680AF8, 0xCA1E3785A9E724E5, 0x1CFC8BED0D681639, 0xD18D8549D140CAEA,
        0x4ED0FE7E9DC91335, 0xE4DBF0634473F5D2, 0x1761F93A44D5AEFE, 0x53898E4C3910DA55,
        0x734DE8181F6EC39A, 0x2680B122BAA28D97, 0x298AF231C85BAFAB, 0x7983EED3740847D5,
        0x66C1A2A1A60CD889, 0x9E17E49642A3E4C1, 0xEDB454E7BADC0805, 0x50B704CAB602C329,
        0x4CC317FB9CDDD023, 0x66B4835D9EAFEA22, 0x219B97E26FFC81BD, 0x261E4E4C0A333A9D,
        0x1FE2CCA76517DB90, 0xD7504DFA8816EDBB, 0xB9571FA04DC089C8, 0x1DDC0325259B27DE,
        0xCF3F4688801EB9AA, 0xF4F5D05C10CAB243, 0x38B6525C21A42B0E, 0x36F60E2BA4FA6800,
        0xEB3593803173E0CE, 0x9C4CD6257C5A3603, 0xAF0C317D32ADAA8A, 0x258E5A80C7204C4B,
        0x8B889D624D44885D, 0xF4D14597E660F855, 0xD4347F66EC8941C3, 0xE699ED85B0DFB40D,
        0x2472F6207C2D0484, 0xC2A1E7B5B459AEB5, 0xAB4F6451CC1D45EC, 0x63767572AE3D6174,
        0xA59E0BD101731A28, 0x116D0016CB948F09, 0x2CF9C8CA052F6E9F, 0x0B090A7560A968E3,
        0xABEEDDB2DDE06FF1, 0x58EFC10B06A2068D, 0xC6E57A78FBD986E0, 0x2EAB8CA63CE802D7,
        0x14A195640116F336, 0x7C0828DD624EC390, 0xD74BBE77E6116AC7, 0x804456AF10F5FB53,
        0xEBE9EA2ADF4321C7, 0x03219A39EE587A30, 0x49787FEF17AF9924, 0xA1E9300CD8520548,
        0x5B45E522E4B1B4EF, 0xB49C3B3995091A36, 0xD4490AD526F14431, 0x12A8F216AF9418C2,
        0x001F837CC7350524, 0x1877B51E57A764D5, 0xA2853B80F17F58EE, 0x993E1DE72D36D310,
        0xB3598080CE64A656, 0x252F59CF0D9F04BB, 0xD23C8E176D113600, 0x1BDA0492E7E4586E,
        0x21E0BD5026C619BF, 0x3B097ADAF088F94E, 0x8D14DEDB30BE846E, 0xF95CFFA23AF5F6F4,
        0x3871700761B3F743, 0xCA672B91E9E4FA16, 0x64C8E531BFF53B55, 0x241260ED4AD1E87D,
        0x106C09B972D2E822, 0x7FBA195410E5CA30, 0x7884D9BC6CB569D8, 0x0647DFEDCD894A29,
        0x63573FF03E224774, 0x4FC8E9560F91B123, 0x1DB956E450275779, 0xB8D91274B9E9D4FB,
        0xA2EBEE47E2FBFCE1, 0xD9F1F30CCD97FB09, 0xEFED53D75FD64E6B, 0x2E6D02C36017F67F,
        0xA9AA4D20DB084E9B, 0xB64BE8D8B25396C1, 0x70CB6AF7C2D5BCF0, 0x98F076A4F7A2322E,
        0xBF84470805E69B5F, 0x94C3251F06F90CF3, 0x3E003E616A6591E9, 0xB925A6CD0421AFF3,
        0x61BDD1307C66E300, 0xBF8D5108E27E0D48, 0x240AB57A8B888B20, 0xFC87614BAF287E07,
        0xEF02CDD06FFDB432, 0xA1082C0466DF6C0A, 0x8215E577001332C8, 0xD39BB9C3A48DB6CF,
        0x2738259634305C14, 0x61CF4F94C97DF93D, 0x1B6BACA2AE4E125B, 0x758F450C88572E0B,
        0x959F587D507A8359, 0xB063E962E045F54D, 0x60E8ED72C0DFF5D1, 0x7B64978555326F9F,
        0xFD080D236DA814BA, 0x8C90FD9B083F4558, 0x106F72FE81E2C590, 0x7976033A39F7D952,
        0xA4EC0132764CA04B, 0x733EA705FAE4FA77, 0xB4D8F77BC3E56167, 0x9E21F4F903B33FD9,
        0x9D765E419FB69F6D, 0xD30C088BA61EA5EF, 0x5D94337FBFAF7F5B, 0x1A4E4822EB4D7A59,
        0x6FFE73E81B637FB3, 0xDDF957BC36D8B9CA, 0x64D0E29EEA8838B3, 0x08DD9BDFD96B9F63,
        0x087E79E5A57D1D13, 0xE328E230E3E2B3FB, 0x1C2559E30F0946BE, 0x720BF5F26F4D2EAA,
        0xB0774D261CC609DB, 0x443F64EC5A371195, 0x4112CF68649A260E, 0xD813F2FAB7F5C5CA,
        0x660D3257380841EE, 0x59AC2C7873F910A3, 0xE846963877671A17, 0x93B633ABFA3469F8,
        0xC0C0F5A60EF4CDCF, 0xCAF21ECD4377B28C, 0x57277707199B8175, 0x506C11B9D90E8B1D,
        0xD83CC2687A19255F, 0x4A29C6465A314CD1, 0xED2DF21216235097, 0xB5635C95FF7296E2,
        0x22AF003AB672E811, 0x52E762596BF68235, 0x9AEBA33AC6ECC6B0, 0x944F6DE09134DFB6,
        0x6C47BEC883A7DE39, 0x6AD047C430A12104, 0xA5B1CFDBA0AB4067, 0x7C45D833AFF07862,
        0x5092EF950A16DA0B, 0x9338E69C052B8E7B, 0x455A4B4CFE30E3F5, 0x6B02E63195AD0CF8,
        0x6B17B224BAD6BF27, 0xD1E0CCD25BB9C169, 0xDE0C89A556B9AE70, 0x50065E535A213CF6,
        0x9C1169FA2777B874, 0x78EDEFD694AF1EED, 0x6DC93D9526A50E68, 0xEE97F453F06791ED,
        0x32AB0EDB696703D3, 0x3A6853C7E70757A7, 0x31865CED6120F37D, 0x67FEF95D92607890,
        0x1F2B1D1F15F6DC9C, 0xB69E38A8965C6B65, 0xAA9119FF184CCCF4, 0xF43C732873F24C13,
        0xFB4A3D794A9A80D2, 0x3550C2321FD6109C, 0x371F77E76BB8417E, 0x6BFA9AAE5EC05779,
        0xCD04F3FF001A4778, 0xE3273522064480CA, 0x9F91508BFFCFC14A, 0x049A7F41061A9E60,
        0xFCB6BE43A9F2FE9B, 0x08DE8A1C7797DA9B, 0x8F9887E6078735A1, 0xB5B4071DBFC73A66,
        0x230E343DFBA08D33, 0x43ED7F5A0FAE657D, 0x3A88A0FBBCB05C63, 0x21874B8B4D2DBC4F,
        0x1BDEA12E35F6A8C9, 0x53C065C6C8E63528, 0xE34A1D250E7A8D6B, 0xD6B04D3B7651DD7E,
        0x5E90277E7CB39E2D, 0x2C046F22062DC67D, 0xB10BB459132D0A26, 0x3FA9DDFB67E2F199,
        0x0E09B88E1914F7AF, 0x10E8B35AF3EEAB37, 0x9EEDECA8E272B933, 0xD4C718BC4AE8AE5F,
        0x81536D601170FC20, 0x91B534F885818A06, 0xEC8177F83F900978, 0x190E714FADA5156E,
        0xB592BF39B0364963, 0x89C350C893AE7DC1, 0xAC042E70F8B383F2, 0xB49B52E587A1EE60,
        0xFB152FE3FF26DA89, 0x3E666E6F69AE2C15, 0x3B544EBE544C19F9, 0xE805A1E290CF2456,
        0x24B33C9D7ED25117, 0xE74733427B72F0C1, 0x0A804D18B7097475, 0x57E3306D881EDB4F,
        0x4AE7D6A36EB5DBCB, 0x2D8D5432157064C8, 0xD1E649DE1E7F268B, 0x8A328A1CEDFE552C,
        0x07A3AEC79624C7DA, 0x84547DDC3E203C94, 0x990A98FD5071D263, 0x1A4FF12616EEFC89,
        0xF6F7FD1431714200, 0x30C05B1BA332F41C, 0x8D2636B81555A786, 0x46C9FEB55D120902,
        0xCCEC0A73B49C9921, 0x4E9D2827355FC492, 0x19EBB029435DCB0F, 0x4659D2B743848A2C,
        0x963EF2C96B33BE31, 0x74F85198B05A2E7D, 0x5A0F544DD2B1FB18, 0x03727073C2E134B1,
        0xC7F6AA2DE59AEA61, 0x352787BAA0D7C22F, 0x9853EAB63B5E0B35, 0xABBDCDD7ED5C0860,
        0xCF05DAF5AC8D77B0, 0x49CAD48CEBF4A71E, 0x7A4C10EC2158C4A6, 0xD9E92AA246BF719E,
        0x13AE978D09FE5557, 0x730499AF921549FF, 0x4E4B705B92903BA4, 0xFF577222C14F0A3A,
        0x55B6344CF97AAFAE, 0xB862225B055B6960, 0xCAC09AFBDDD2CDB4, 0xDAF8E9829FE96B5F,
        0xB5FDFC5D3132C498, 0x310CB380DB6F7503, 0xE87FBB46217A360E, 0x2102AE466EBB1148,
        0xF8549E1A3AA5E00D, 0x07A69AFDCC42261A, 0xC4C118BFE78FEAAE, 0xF9F4892ED96BD438,
        0x1AF3DBE25D8F45DA, 0xF5B4B0B0D2DEEEB4, 0x962ACEEFA82E1C84, 0x046E3ECAAF453CE9,
        0xF05D129681949A4C, 0x964781CE734B3C84, 0x9C2ED44081CE5FBD, 0x522E23F3925E319E,
        0x177E00F9FC32F791, 0x2BC60A63A6F3B3F2, 0x222BBFAE61725606, 0x486289DDCC3D6780,
        0x7DC7785B8EFDFC80, 0x8AF38731C02BA980, 0x1FAB64EA29A2DDF7, 0xE4D9429322CD065A,
        0x9DA058C67844F20C, 0x24C0E332B70019B0, 0x233003B5A6CFE6AD, 0xD586BD01C5C217F6,
        0x5E5637885F29BC2B, 0x7EBA726D8C94094B, 0x0A56A5F0BFE39272, 0xD79476A84EE20D06,
        0x9E4C1269BAA4BF37, 0x17EFEE45B0DEE640, 0x1D95B0A5FCF90BC6, 0x93CBE0B699C2585D,
        0x65FA4F227A2B6D79, 0xD5F9E858292504D5, 0xC2B5A03F71471A6F, 0x59300222B4561E00,
        0xCE2F8642CA0712DC, 0x7CA9723FBB2E8988, 0x2785338347F2BA08, 0xC61BB3A141E50E8C,
        0x150F361DAB9DEC26, 0x9F6A419D382595F4, 0x64A53DC924FE7AC9, 0x142DE49FFF7A7C3D,
        0x0C335248857FA9E7, 0x0A9C32D5EAE45305, 0xE6C42178C4BBB92E, 0x71F1CE2490D20B07,
        0xF1BCC3D275AFE51A, 0xE728E8C83C334074, 0x96FBF83A12884624, 0x81A1549FD6573DA5,
        0x5FA7867CAF35E149, 0x56986E2EF3ED091B, 0x917F1DD5F8886C61, 0xD20D8C88C8FFE65F,
        0x31D71DCE64B2C310, 0xF165B587DF898190, 0xA57E6339DD2CF3A0, 0x1EF6E6DBB1961EC9,
        0x70CC73D90BC26E24, 0xE21A6B35DF0C3AD7, 0x003A93D8B2806962, 0x1C99DED33CB890A1,
        0xCF3145DE0ADD4289, 0xD0E4427A5514FB72, 0x77C621CC9FB3A483, 0x67A34DAC4356550B,
        0xF8D626AAAF278509,
    ]
    
}

