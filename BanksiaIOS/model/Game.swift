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

enum VarEnum {
  case turn, analysicOnFly, engine
}

final class Game: ObservableObject {
  @Published var chessBoard = ChessBoard()
  @Published var displayingChessBoard = ChessBoard()
  
  @Published var timeControlMode = TimeControlMode.standard
  
  @Published var timer_standard_time = 5 /// unit: minute
  @Published var timer_standard_moves = 40
  @Published var timer_standard_inc = 1
  
  @Published var timer_depth = 12
  @Published var timer_nodes = 1000
  @Published var timer_movetime = 30 /// unit: second
  
  @Published var maxcores = 1
  @Published var cores = 2
  @Published var skillLevel = 20
  
  @Published var evalNNUEMode = 2
  @Published var bookMode = true
  
  @Published var expectingBestmove = false
  
  @Published var benchMode = false
  @Published var benchInfo = ""
  @Published var benchComputing = ""
  
  @Published var uciCmd_cores = 1
  @Published var uciCmd_nnuefile = ""
  
  @Published var engineIdx = 0 {
    didSet {
      if engineIdx != oldValue {
        if expectingBestmove {
          engine_cmd(Int32(engineData.allEngines[oldValue].idNumb), "stop")
          expectingBestmove = false
        }
        engine_clearAllMessages(Int32(engineIdx));
        variantChanged?(.engine)
      }
    }
  }
  
  @Published var geoBoardFrame = CGRect.zero

  var variantChanged: ((_ varEnum: VarEnum) -> Void)?
  let engineData = EngineData()
  
  @Published var turn = Turn.human_white {
    didSet {
      if turn != oldValue {
        variantChanged?(.turn)
      }
    }
  }
  
  @Published var analysicOnFlyMode = false {
    didSet {
      if analysicOnFlyMode != oldValue {
        variantChanged?(.analysicOnFly)
      }
    }
    
  }
  
  /// timeleft for standard clock, unit: millisecond
  var timeLeft = [ 0, 0 ]
  
  let book = BookPolyglot()
  
  let savedPGNKey = "savedPGN"
  
//  /// For debugging
//  let savedPGN =
//    """
//[Event "BSG online game"]
//[Site "Lichess"]
//[Date "2020.06.27"]
//[White "hm::BanksiaGUI 1707"]
//[Black "Stockfish 230619"]
//[Result "0-1"]
//[TimeControl "60+2"]
//[Time "21:22:11"]
//[Termination "mate"]
//[ECO "B23"]
//[Opening "Sicilian"]
//
//1. e4 c5 {-0.22/18 8.6 2142365}  2. Nc3 Nc6 {-0.11/18 4.3 1038429; B23: Sicilian, closed, 2...Nc6}
//3. Nf3 d6 {-0.05/19 5.3 1354783}  4. Bc4 e6 {+0.18/18 2.1 591321}
//5. d4 cxd4 {+0.29/16 1.1 303232}  6. Nxd4 Be7 {+0.27/17 1.8 487214}
//7. Be3 Nf6 {+0.25/18 3.4 895182}  8. O-O O-O {+0.00/18 6.1 1660921}
//9. a3 a6 {+0.12/17 4.8 1286506}  10. Be2 d5 {+0.42/15 0.8 233414}
//11. exd5 exd5 {+0.22/18 3.5 897260}  12. h3 Bd6 {+0.45/17 3.3 802609}
//13. Re1 Re8 {+0.37/17 5.0 1195308}  14. Qd2 Qc7 {+0.35/16 2.8 698471}
//15. Bf3 Be6 {+0.23/17 3.7 855584}  16. Rad1 Rad8 {+0.16/17 2.3 636794}
//17. Re2 Bh2+ {+0.13/17 8.7 2432363}  18. Kh1 Be5 {+0.16/16 0.9 258937}
//19. Qd3 Qd7 {+0.65/16 1.6 442148}  20. Nxe6 fxe6 {+0.83/17 1.1 314628}
//21. Na4 Bb8 {+1.15/17 2.3 649302}  22. Qb3 Na5 {+1.30/17 1.3 375380}
//23. Qb4 Nc4 {+1.37/19 4.5 1324732}  24. Nc5 Qc7 {+1.82/17 2.6 794270}
//25. g3 Qf7 {+1.27/18 2.7 833258}  26. Nxb7 Rc8 {+1.31/16 1.5 427030}
//27. Bd2 Ne4 {+2.75/18 1.2 371619}  28. Bxe4 dxe4 {+2.92/18 2.1 646963}
//29. Kg1 Ba7 {+3.24/22 9.4 3116240}  30. Bf4 Qh5 {+2.35/20 2.7 793107}
//31. Qe1 e3 {+2.82/20 1.3 475658}  32. Bxe3 Bxe3 {+3.11/21 1.6 609663}
//33. b3 Qxh3 {+2.68/23 9.5 3381370}  34. Rxe3 Nxe3 {+2.91/22 1.2 461371}
//35. Qxe3 Rxc2 {+2.99/23 3.6 1132670}  36. Rc1 Rxc1+ {+2.96/20 1.8 570267}
//37. Qxc1 Rf8 {+2.98/21 1.6 582370}  38. Nc5 Qf5 {+3.08/21 1.4 368035}
//39. Qe3 Qd5 {+3.04/21 1.4 410882}  40. Qxe6+ Qxe6 {+3.38/22 3.3 1030613}
//41. Nxe6 Rc8 {+3.34/21 2.2 744291}  42. Kg2 Rc6 {+3.88/19 1.7 737933}
//43. Nd4 Rc3 {+3.98/20 1.7 721474}  44. b4 Rxa3 {+4.19/19 1.9 689459}
//45. Ne6 Kf7 {+4.41/18 1.1 396151}  46. Nc7 Ke7 {+5.06/19 1.3 519353}
//47. Nd5+ Kd6 {+5.14/18 1.1 477604}  48. Ne3 Ke6 {+5.66/19 1.9 801603}
//49. Nc2 Ra2 {+6.16/18 1.6 746805}  50. Nd4+ Ke5 {+6.36/19 2.1 1019090}
//51. Nc6+ Kd5 {+6.84/17 1.5 716524}  52. Ne7+ Kc4 {+7.07/18 1.9 940849}
//53. Nc6 Kb5 {+7.40/19 1.6 960193}  54. Ne5 Kxb4 {+7.72/18 1.1 680275}
//55. Nc6+ Kc5 {+9.01/19 1.7 1000795}  56. Nd8 a5 {+11.41/18 1.5 725776}
//57. Ne6+ Kc4 {+13.24/19 1.1 591587}  58. Nxg7 a4 {+14.49/17 1.2 475215}
//59. Nf5 a3 {+15.35/16 1.1 558296}  60. Nd6+ Kb4 {+17.23/18 1.3 612086}
//61. Kh3 Rxf2 {+148.93/21 2.6 1253255}  62. Ne4 Rb2 {+148.99/21 0.8 325766}
//63. Nf6 a2 {+149.02/21 1.2 438426}  64. Nd5+ Kb3 {M+12/26 1.3 432917}
//65. Kg4 Rf2 {M+10/31 1.8 463483}  66. Kg5 a1=Q {M+6/39 1.4 283382}
//67. Nf4 Qe5+ {M+5/45 1.3 618592}  68. Kg4 h5+ {M+3/245 1.0 836605}
//69. Kh4 Qf5 {M+2/245 0.1 19242}  70. Nxh5 Rh2# {M+1/245 0.1 10050}
// 0-1
//"""
  
  init() {
    read()

    #if !LC0ONLY
    if let path = Bundle.main.path(forResource: network_nnue, ofType: nil) {
      setStockfishNNUEPath(path);
    }
    #endif

    #if !STOCKFISHONLY
    if let path = Bundle.main.path(forResource: network_lc0, ofType: nil) {
      setLc0NetPath(path);
    }
    #endif

    initCurrentEngine()
    
    let processInfo = ProcessInfo()
    print(processInfo.activeProcessorCount)
    maxcores = processInfo.activeProcessorCount
        
    let savedPGN = UserDefaults.standard.string(forKey: savedPGNKey) ?? ""
    
    var needNewGame = true
    if !savedPGN.isEmpty {
      var tmpBoard = ChessBoard()
      if parsePGN(chessBoard: &tmpBoard, string: savedPGN) {
        _ = chessBoard.clone(fromBoard: tmpBoard)
        newGameComplete()
        needNewGame = false
      }
    }
    
    if needNewGame {
      newGame()
    }
  }
  
  func read() {
    let defaults = UserDefaults.standard
    if defaults.string(forKey: "gamewritten") == nil {
      return
    }
    
    turn = Turn(rawValue: defaults.integer(forKey: "turn")) ?? Turn.human_white
    
    timeControlMode = TimeControlMode(rawValue: defaults.integer(forKey: "timer")) ?? TimeControlMode.standard
    timer_standard_time = max(1, defaults.integer(forKey: "timer_standard_time"))
    timer_standard_moves = max(0, defaults.integer(forKey: "timer_standard_moves"))
    timer_standard_inc = max(0, defaults.integer(forKey: "timer_standard_inc"))
    
    timer_depth = max(1, defaults.integer(forKey: "timer_depth"))
    timer_nodes = max(1, defaults.integer(forKey: "timer_nodes"))
    timer_movetime = max(1, defaults.integer(forKey: "timer_movetime"))
    
    skillLevel = defaults.integer(forKey: "skills")
    evalNNUEMode = defaults.integer(forKey: "evalMode")
    bookMode = defaults.bool(forKey: "bookMode")
    
    engineIdx = min(engineData.allEngines.count - 1, max(0, defaults.integer(forKey: "engineIdx")))
    print("engineIdx", engineIdx, "engineData.allEngines.count", engineData.allEngines.count, engineData.allEngines[0].idNumb)
    
    #if STOCKFISHONLY
    print("STOCKFISHONLY")
    #endif

    #if LC0ONLY
    print("LC0ONLY")
    #endif
  }
  
  func write() {
    let defaults = UserDefaults.standard
    defaults.set(turn.rawValue, forKey: "turn")
    defaults.set(timeControlMode.rawValue, forKey: "timer")
    
    defaults.set(timer_standard_time, forKey: "timer_standard_time")
    defaults.set(timer_standard_moves, forKey: "timer_standard_moves")
    defaults.set(timer_standard_inc, forKey: "timer_standard_inc")
    
    defaults.set(timer_depth, forKey: "timer_depth")
    defaults.set(timer_nodes, forKey: "timer_nodes")
    defaults.set(timer_movetime, forKey: "timer_movetime")
    
    defaults.set(skillLevel, forKey: "skills")
    defaults.set(evalNNUEMode, forKey: "evalMode")
    defaults.set(bookMode, forKey: "bookMode")
    defaults.set(engineIdx, forKey: "engineIdx")
    
    defaults.set("gamewritten", forKey: "gamewritten")
  }
  
  func getEngineDisplayName() -> String {
    let engineInfo = engineData.allEngines[engineIdx]
    #if os(watchOS)
    return engineInfo.shortName
    #else
    return engineInfo.name
    #endif
  }
  
  func newGame(fen: String = "") {
    chessBoard.setFen(fen: fen)
    chessBoard.result.reset()
    newGameComplete()
    
    initCurrentEngine()
    sendToEngine(cmd: "ucinewgame")
  }
  
  func newGameComplete() {
    _ = displayingChessBoard.clone(fromBoard: chessBoard)
    setupClocksBeforeThinking(newGame: true)
  }
  
  func sendUciStop() {
    sendToEngine(cmd: "stop")
  }
  
  /// msec left
  func getTimeLeft(side: Side) -> Int {
    return self.timeLeft[side.rawValue]
  }
  
  func uciPosition() -> String {
    var cmd = "position "
    if analysicOnFlyMode {
      if displayingChessBoard.startFen.isEmpty {
        cmd += "startpos"
      } else {
        cmd += "fen \(displayingChessBoard.startFen)"
      }

      if !displayingChessBoard.histList.isEmpty {
        cmd += " moves"
        for hist in displayingChessBoard.histList {
          cmd += " \(displayingChessBoard.moveString_coordinate(move: hist.move))"
        }
      }
    } else {
      if chessBoard.startFen.isEmpty {
        cmd += "startpos"
      } else {
        cmd += "fen \(chessBoard.startFen)"
      }
      
      if !chessBoard.histList.isEmpty {
        cmd += " moves"
        for hist in chessBoard.histList {
          cmd += " \(chessBoard.moveString_coordinate(move: hist.move))"
        }
      }
    }
    return cmd
  }
  
  func uciGo() -> String {
    var cmd = "go "
    if analysicOnFlyMode {
      
    }
    switch timeControlMode {
    case .standard:
      cmd += "wtime \(timeLeft[Side.white.rawValue]) btime \(timeLeft[Side.black.rawValue]) winc \(timer_standard_inc * 1000) binc \(timer_standard_inc * 1000)"
      
    case .depth:
      cmd += "depth \(timer_depth)"
    case .nodes:
      cmd += "nodes \(timer_nodes)"
    case .movetime:
      cmd += "movetime \(timer_movetime)"
    case .infinite:
      cmd += "infinite"
    }
    return cmd
  }
  
  func uciBenchmark(core: Int) -> String {
    var cmd = "bench"
    if core > 1 {
      cmd += " 16 \(core)"
    }
    return cmd
  }
  
  func isHumanTurn() -> Bool {
    return isHumanTurn(side: chessBoard.side)
  }
  
  func isHumanTurn(side: Side) -> Bool {
    return turn == Turn.human_both ||
      (turn == Turn.human_white && side == Side.white) ||
      (turn == Turn.human_black && side == Side.black)
  }
  
  func setupClocksBeforeThinking(newGame: Bool) {
    let sd = chessBoard.side.rawValue
    if timeControlMode == .movetime {
      timeLeft[sd] = timer_movetime * 1000
      
      if chessBoard.histList.isEmpty {
        timeLeft[1 - sd] = timeLeft[sd]
      }
      
    } else
    if (timeControlMode == .standard) {
      if newGame {
        /// convert from minute to milisecond
        timeLeft[0] = timer_standard_time * 60 * 1000
        timeLeft[1] = timeLeft[0]
        return
      }
      
      if chessBoard.histList.count > 1 { /// ignored for 0, 1 (first move)
        timeLeft[sd] += timer_standard_inc * 1000
      }
      
      // Reset times after n moves
      if timer_standard_moves > 0 {
        let fullCnt = chessBoard.histList.count / 2 /// cnt from 0
        if fullCnt > 1 && fullCnt % timer_standard_moves == 0 {
          timeLeft[sd] += timer_standard_time * 60 * 1000
        }
      }
    }
  }
  
  func udateClockAfterMove(moveElapseInMillisecond: Int) {
    assert(moveElapseInMillisecond > 0 && chessBoard.histList.count > 0)
    
    if timeControlMode != .standard {
      return;
    }
    
    /// Ignore time for human for the first move
    if chessBoard.histList.count <= 1 && (turn == Turn.human_both || turn == Turn.human_white) {
      return;
    }
    
    timeLeft[1 - chessBoard.side.rawValue] -= moveElapseInMillisecond
  }
  
  func probeBook() -> Move {
    return book.probe(board: chessBoard, forBestMove: true)
  }
  
  func savePGNGame() {
    let pgnString = getGamePGN()
    UserDefaults.standard.set(pgnString, forKey: savedPGNKey)
  }
  
  func getGameTitle() -> String {
    let whiteName = isHumanTurn(side: .white) ? "White" : getEngineDisplayName()
    let blackName = isHumanTurn(side: .black) ? "Black" : getEngineDisplayName()
    let r = chessBoard.result.isNone() ? "vs" : chessBoard.result.toShortString()
    return "\(whiteName) \(r) \(blackName)"
  }
  
  func getGamePGN() -> String {
    let event = "BanksiaGUI game"
    let site = "iOS device"
    let round = 1
    let whiteName = isHumanTurn(side: .white) ? "White" : getEngineDisplayName()
    let blackName = isHumanTurn(side: .black) ? "Black" : getEngineDisplayName()
    let resultString = chessBoard.result.toShortString()
    
    let date = Date()
    let df = DateFormatter()
    df.dateFormat = "yyyy-MM-dd"
    let dateString = df.string(from: date)
    df.dateFormat = "HH:mm:ss"
    let timeString = df.string(from: date)
    
    
    var str =
      "[Event \"\(event)\"]\n" +
      "[Site \"\(site)\"]\n" +
      "[Date \"\(dateString)\"]\n" +
      "[Round \"\(round)\"]\n" +
      "[White \"\(whiteName)\"]\n" +
      "[Black \"\(blackName)\"]\n" +
      "[Result \"\(resultString)\"]\n" +
      
      "[TimeControl \"\(getTimeControlString())\"]\n" +
      "[Time \"\(timeString)\"]\n"
    
    
    //    auto str = chessBoard.result.reasonString()
    //    if (!str.empty()) {
    //        stringStream << "[Termination \"" << str << "\"]" << std::endl;
    //    }
    //
    //    if (board->variant != ChessVariant::standard) {
    //        stringStream << "[Variant \"" << chessVariant2String(board->variant) << "\"]" << std::endl;
    //    }
    
    if (chessBoard.variant == ChessVariant.chess960 || !chessBoard.startFen.isEmpty) {
      str += "[FEN \"\(chessBoard.startFen)\"]\n" +
        "[SetUp \"1\"]\n"
    }
    
    //    auto ecoVec = board->commentEcoString();
    //    if (ecoVec.size() > 1) {
    //        stringStream << "[ECO \"" << ecoVec.front() << "\"]" << std::endl;
    //        stringStream << "[Opening \"" << ecoVec.at(1) << "\"]" << std::endl;
    //    }
    
    // Move text
    str += "\n" + chessBoard.toMoveListString(itemPerLine: 8)
    
    if !chessBoard.result.isNone() {
      if chessBoard.histList.count % 8 != 0 {
        str += " ";
      }
      str += " \(resultString)";
    }
    
    str += "\n"
    return str
  }
  
  func getTimeControlString() -> String {
    var str = ""
    switch timeControlMode {
    case .standard:
      let s0 = timer_standard_moves > 0 ? "/\(timer_standard_moves)" : ""
      let s1 = timer_standard_inc > 0 ? "+\(timer_standard_inc)" : ""
      str = "\(timer_standard_time)\(s0)\(s1)"
      
    case .depth:
      str = "depth \(timer_depth)"
      
    case .infinite:
      str = "infinite"
      
    case .movetime:
      str = "movetime \(timer_movetime)"
      
    case .nodes:
      str = "node \(timer_nodes)"
    }
    return str
  }
  
  func paste(string: String) {
    var ok = string.contains("[Event")
    
    var str = string
    /// FEN only
    if !ok && string.contains("/") && string.contains("k") && string.contains("K") && string.contains(" ") {
      ok = true
      str = "[Event \"event\"]\n[Site \"site\"]\n[White \"w\"]\n[Black \"b\"]\n[FEN \"\(string)\"]\n"
    }
    print(str)
    if ok {
      var tmpBoard = ChessBoard()
      if parsePGN(chessBoard: &tmpBoard, string: str) {
        _ = chessBoard.clone(fromBoard: tmpBoard)
        newGameComplete()
      }
    }
    
  }
  
  func parsePGN(chessBoard: inout ChessBoard, string: String) -> Bool {
    let contentVec = string.split(separator: "\n")
    var itemMap = [String : String]()
    
    var moveText = ""
    for ss in contentVec {
      let s = String(ss).trimmingCharacters(in: .whitespacesAndNewlines)
      if s.starts(with: "[") {
        let arr = s.split(separator: "\"")
        if arr.count < 3 {
          continue
        }
        
        let str = String(arr[1])
        if str.isEmpty {
          continue
        }
        
        var theSet = CharacterSet.whitespacesAndNewlines
        theSet.insert("[")
        let key = String(arr[0]).trimmingCharacters(in: theSet).lowercased()
        if key.isEmpty {
          continue
        }
        
        itemMap[key] = str
        continue;
      }
      moveText += " " + s
    }
    
    let fen = itemMap["fen"] ?? ""
    
    //      var variant = chessVariant;
    //      auto p = itemMap.find("variant");
    //      if (p != itemMap.end()) {
    //          variant = string2ChessVariant(p->second);
    //      }
    //
    //    if (inputBoard && inputBoard->variant != variant) {
    //      std::cerr << "Input board is incorrect on variant";
    //      return;
    //    }
    //
    //    auto board = inputBoard ? inputBoard : Game::createBoard(variant);
    //
    //    if (!board) {
    //      std::cerr << "Variant is not supported";
    //      return;
    //    }
    
    chessBoard.setFen(fen: fen)
//    chessBoard.printBoard()
    if !chessBoard.isValid() {
      return false
    }
    
    _ = chessBoard.fromMoveList(str: moveText)
    
    chessBoard.result.result = ResultType.noresult
    if let s = itemMap["result"] {
      switch s {
      case "1-0":
        chessBoard.result.result = ResultType.win
      case "0-1":
        chessBoard.result.result = ResultType.loss
      case "1/2-1/2", "0.5-0.5":
        chessBoard.result.result = ResultType.draw
      default:
        break
      }
    }
    
    return chessBoard.isValid()
  }

  func checkMakeMove(from: Int, dest: Int, promotion: Int, byComputer: Bool, moveElapseInMillisecond: Int) -> Bool {
    if chessBoard.checkMake(from: from, dest: dest, promotion: promotion) {
      udateClockAfterMove(moveElapseInMillisecond: moveElapseInMillisecond)
      if displayingChessBoard.histList.count + 1 == chessBoard.histList.count {
        _ = displayingChessBoard.clone(fromBoard: chessBoard)
      }
      savePGNGame()
      return true
    }
    print("Not legal move", from, dest, promotion)
    return false
  }

  func takeBack(moveElapseInMillisecond: Int) {
    if chessBoard.histList.isEmpty {
      return
    }
    
    chessBoard.takeBack()
    if displayingChessBoard.histList.count > chessBoard.histList.count {
      _ = displayingChessBoard.clone(fromBoard: chessBoard)
    }
    
    udateClockAfterMove(moveElapseInMillisecond: moveElapseInMillisecond)
  }
  
  func initCurrentEngine() {
    engine_initialize(getEngineIdNumb(), Int32(self.cores), Int32(self.skillLevel), Int32(self.evalNNUEMode))
  }
  
  func computerGo() {
    self.expectingBestmove = true
    DispatchQueue.global(qos: .userInitiated).async {
      self.initCurrentEngine();
      self.sendToEngine(cmd: self.uciPosition())
      self.sendToEngine(cmd: self.uciGo())
    }
  }
  
  func benchmark(core: Int) {
    DispatchQueue.global().asyncAfter(deadline: .now() + 0.2, qos: .default) {
    // DispatchQueue.global(qos: .userInitiated).async {
      self.initCurrentEngine();
      self.sendToEngine(cmd: self.uciBenchmark(core: core))
    }
    self.expectingBestmove = true
  }
  
  func sendToEngine(cmd: String) {
    engine_cmd(getEngineIdNumb(), cmd)
  }
  
  func getSearchMessage() -> String? {
    if let ptr = engine_getSearchMessage(getEngineIdNumb()) {
      let s = String(cString: ptr)
      if !s.isEmpty {
        return s
      }
    }
    return nil
  }
  
  func getEngineIdNumb() -> Int32 {
    return Int32(engineData.allEngines[engineIdx].idNumb)
  }
  
  func canBenchmark() -> Bool {
    return engineData.allEngines[engineIdx].bench
  }
}

