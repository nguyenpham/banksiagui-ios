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


struct ContentView: View {
    static let version = "1.1"
    
    @EnvironmentObject var userData: UserData
    @EnvironmentObject var game: Game
    @State var isNavigationBarHidden: Bool = true
    
    @State var cellWidth: CGFloat = 0
    @State var geoSize = CGSize(width: 0, height: 0)
    
    @State var engineOutput = EngineOutput()

    @State var flip = false
    
    @State var editing = false
    @State var editingFenString = ""
    
    @State var timeCouter = 0
    @State var whiteTimer = 1
    @State var blackTimer = 10
    
    @State var humanTurn = Side.white
    @State var selectedPos = -1
    
    @State var aniFrom = -1
    @State var aniDest = -1
    @State var aniPromotion = -1
    @State var aniTask = MoveTask.make
    
    @State var promotionFrom = -1
    @State var promotionDest = -1
    @State var showingPopup_promotion = false
    @State var showingPopup_copy = false
    @State var showingPopup_menu = false
    @State var showingPopup_move = false
    
    @State var copyMode = CopyMode.pgn.rawValue
    
    @State var geoBoardFrame = CGRect.zero
    @State var creatingImage = false
    
    @State var legalSet = Set<Int>()
    
    @State var startMoveTime: CFAbsoluteTime = 0
    
    @State var flipSize = false
    @State var hasAd = false
    
    
#if os(watchOS)
    @State var alwaysLargeBoardMode = false
#else
    @ObservedObject var moveModel = MoveModel()
    
    let mailComposeDelegate = MailDelegate()
    let messageComposeDelegate = MessageDelegate()
#endif
    
    static let menuHeight: CGFloat = 40
    
    let soundMng = SoundMng()
    let timer = Timer.publish(every: 0.2, on: .main, in: .common).autoconnect()
    
    enum ComputingTask: Int {
        case go, takeback, new
    }
    @State var computingTask = [ComputingTask]()
    
    var body: some View {
        let v = GeometryReader { geometry in
            ZStack {
#if os(watchOS)
                createViews(size: geometry.size)
                if showingPopup_promotion {
                    createPromotionPopup(size: geometry.size)
                }
#else
                ZStack {
                    createViews(size: geometry.size)
                    if showingPopup_promotion || showingPopup_copy || showingPopup_menu || showingPopup_move {
                        Rectangle().fill(Color.black.opacity(0.3))
                            .edgesIgnoringSafeArea(.all)
                    }
                }
#endif
            }
            .onAppear {
                initData()
                geoBoardFrame = geometry.frame(in: CoordinateSpace.global)
                geoSize = geometry.size
            }
            .onReceive(self.timer) { _ in
                while let s = game.getSearchMessage() {
                    processEngineOutput(s)
                }
                
                self.timeCouter += 1
                if (self.timeCouter % 5) != 0 {
                    return
                }
                
                _ = doComputingTask()
                
                if game.timeControlMode != .standard && game.timeControlMode != .movetime {
                    return
                }
                
                for side in [Side.white, Side.black] {
                    var timeLeft = game.getTimeLeft(side: side)
                    if game.chessBoard.result.isNone() && side == game.chessBoard.side && !(game.chessBoard.histList.isEmpty && game.isHumanTurn()) {
                        timeLeft -= Int((CFAbsoluteTimeGetCurrent() - startMoveTime) * 1000)
                    }
                    if side == .white {
                        self.whiteTimer = timeLeft / 1000
                    } else {
                        self.blackTimer = timeLeft / 1000
                    }
                }
            }
        }
        
#if os(watchOS)
        return v
#else
        return v
            .popup(isPresented: $showingPopup_promotion, type: .`default`, closeOnTap: false) {
                createPromotionPopup(size: geoSize)
            }
            .popup(isPresented: $showingPopup_copy, type: .`default`, closeOnTap: false) {
                createCopyPopup(size: geoSize)
            }
            .popup(isPresented: $showingPopup_menu, type: .floater(), position: .bottom, closeOnTap: false) {
                createMenuPopup(size: geoSize)
            }
            .popup(isPresented: $showingPopup_move, type: .floater(), position: .bottom, closeOnTap: false) {
                createMenuPopup_move(size: geoSize)
            }
#endif
        
    }
    
    func gameOverView(size: CGSize) -> some View {
        Text("Game Over \(game.chessBoard.result.toShortString())")
            .font(.title)
            .shadow(color: .black, radius: 3, x: 3, y: 3)
            .position(CGPoint(x: size.width / 2, y: size.width / 2))
            .foregroundColor(.red)
    }
    
    @State var initDone = false
    
    func initData() {
        if self.initDone {
            return
        }
        self.initDone = true
        
        game.variantChanged = variantChanged
        newGame_complete(autoGo: false)
    }
    
    func variantChanged(varEnum: VarEnum) {
        switch varEnum {
        case .turn:
            turnChanged(autoGo: true)
        case .analysicOnFly:
            print("main analysicOnFly", self.game.analysicOnFlyMode)
            
            if analysicOnFly_true() {
                return
            }
            
            if game.expectingBestmove {
                game.sendUciStop()
            }
            
        case .engine:
            break
        }
    }
    
    func analysicOnFly_true() -> Bool {
        if !self.game.analysicOnFlyMode {
            return false
        }
        
        computerGo()
        return true
    }
    
    
    func createViews_Board(cellWidth: CGFloat) -> some View {
        ZStack {
            GBoard(cellwidth: cellWidth,
                   cellStyle: userData.cellStyle,
                   showCoordinates: userData.showCoordinates,
                   flip: flip,
                   from: game.displayingChessBoard.histList.last?.move.from ?? -1,
                   dest: game.displayingChessBoard.histList.last?.move.dest ?? -1,
                   selectedPos: selectedPos
            )
            .frame(width: cellWidth * 8, height: cellWidth * (editing ? 10 : 8))
            
            GAllPieces(cellwidth: cellWidth,
                       chessBoard: $game.displayingChessBoard,
                       pieceStyle: userData.pieceStyle,
                       flip: flip,
                       editing: editing,
                       humanSide: humanTurn,
                       make: { (piece, from, dest, promotion, moveTask) -> Bool in return make(piece: piece, from: from, dest: dest, promotion: promotion, moveTask: moveTask) },
                       tap: { pos in tap(pos: pos) },
                       startDragging: { pos in genLegalMovesFrom(pos: pos) },
                       arrowFrom: userData.showAnalysisArrows ? engineOutput.arrowFrom : -1, arrowDest: engineOutput.arrowDest,
                       aniFrom: aniFrom, aniDest: aniDest, aniPromotion: aniPromotion, aniTask: aniTask,
                       selectedPos: selectedPos,
                       legalSet: legalSet
            )
            .frame(width: cellWidth * 8, height: cellWidth * (editing ? 10 : 8))
        }
        .gesture(LongPressGesture(minimumDuration: 1.0)
            .onEnded({ _ in
                if editing {
                    flipSize = false
                } else {
                    flipSize.toggle()
                }
            }))
    }
    
    func createClockString(side: Side, timeLeft: Int) -> String {
        var str = ""
        if game.isHumanTurn(side: side) {
#if os(watchOS)
            str += side.getShortName()
#else
            str += !userData.userName.isEmpty ? userData.userName : side.getName()
#endif
        } else {
            str += game.getEngineDisplayName()
        }
        
        if !game.analysicOnFlyMode && (game.timeControlMode == .standard || game.timeControlMode == .movetime) {
            str += ": " + clockString(side: side, second: timeLeft)
        }
        return str
    }
    
    func clockString(side: Side, second: Int) -> String {
        if game.timeControlMode != .standard && game.timeControlMode != .movetime {
            return ""
        }
        
        let spString = ":";
        
        let t = abs(second)
        let h = t / (60 * 60), r = t % (60 * 60), m = r / 60, s = r % 60;
        
        var str = second < 0 ? "-" : ""
        var mstring = "\(m)"
        if (h > 0) {
            str += "\(h)\(spString)"
            mstring = String("\(m)".paddingToLeft(upTo: 2, using: "0"))
        }
        
        let sstring = "\(s)".paddingToLeft(upTo: 2, using: "0")
        str += mstring + spString + sstring
        return str
    }
    
    private let promotionPieceTypes = [PieceTypeStd.queen.rawValue, PieceTypeStd.rook.rawValue, PieceTypeStd.bishop.rawValue, PieceTypeStd.knight.rawValue]
    
    func createPromotionPopup(size: CGSize) -> some View {
        ZStack {
            VStack(spacing: 10) {
                Text("Promotion")
                HStack {
                    ForEach(promotionPieceTypes, id: \.self) { t in
                        Button(action: {
                            showingPopup_promotion = false
                            _ = checkMakeMove(from: promotionFrom,
                                              dest: promotionDest,
                                              promotion: t,
                                              byComputer: false)
                        }) {
                            GPiece(piece: Piece(type: t, side: game.chessBoard.side),
                                   pos: 0,
                                   cellwidth: size.width / 8,
                                   pieceStyle: userData.pieceStyle,
                                   flip: false,
                                   editing: false,
                                   humanSide: Side.none,
                                   make: { (_, _, _, _, _) -> Bool in return false },
                                   tap: { pos in },
                                   startDragging: { pos in },
                                   isArrowEnd: false, aniFrom: -1, aniDest: -1, aniPromotion: -1, aniTask: MoveTask.make, selectedPos: -1, showLegalMark: false
                            )
                            .disabled(true)
                        }
                        .buttonStyle(PlainButtonStyle())
                        .frame(width: size.width / 8, height: size.width / 8)
                    }
                }
            }
            //.padding(EdgeInsets(top: 70, leading: 20, bottom: 40, trailing: 20))
            .frame(width: size.width * 6 / 8, height: size.width * 3 / 8)
            .background(Color(red: 0x3d, green: 0x5a, blue: 0x80))
            .cornerRadius(10.0)
            .shadow(color: Color(.sRGBLinear, white: 0, opacity: 0.13), radius: 10.0)
        }
    }
    
    func clearAllMarks() {
        selectedPos = -1
        
        engineOutput.arrowFrom = -1
        engineOutput.arrowDest = -1
        
        aniFrom = -1
        aniDest = -1
        aniPromotion = -1
        
        promotionFrom = -1
        promotionDest = -1
    }
    
    func setupForNewMove(autoGo: Bool) {
        legalSet.removeAll()
        clearAllMarks()
        
        showingPopup_promotion = false
        
        turnChanged(autoGo: autoGo)
    }
    
    func turnChanged(autoGo: Bool) {
        game.setupClocksBeforeThinking(newGame: game.chessBoard.histList.isEmpty)
        startMoveTime = CFAbsoluteTimeGetCurrent()
        
        let isHumanTurn = game.analysicOnFlyMode || game.isHumanTurn()
        humanTurn = game.chessBoard.result.isNone() && isHumanTurn ? self.game.chessBoard.side : Side.none
        if autoGo && (!isHumanTurn || game.analysicOnFlyMode) {
            computerGo()
        }
    }
    
    func tap(pos: Int) {
        if !game.chessBoard.isPositionValid(pos: pos) {
            return
        }
        
        if game.chessBoard.histList.count != game.displayingChessBoard.histList.count || game.displayingChessBoard.side != humanTurn {
            return
        }
        let piece = game.displayingChessBoard.getPiece(pos)
        if game.displayingChessBoard.side == piece.side {
            selectPos(pos: pos)
            return
        }
        
        if selectedPos >= 0 {
            _ = make(piece: game.chessBoard.getPiece(selectedPos), from: selectedPos, dest: pos, promotion: Piece.EMPTY, moveTask: .make)
        }
    }
    
    func selectPos(pos: Int) {
        self.selectedPos = pos
        genLegalMovesFrom(pos: pos)
    }
    
    func playSound(move: MoveFull, sanString: String) {
        soundMng.playSound(move: move,
                           sanString: sanString,
                           soundMode: SoundMode(rawValue: userData.soundMode) ?? SoundMode.off,
                           speechRate: userData.speechRate,
                           speechName: userData.speechName)
    }
}

struct ContentView_Previews: PreviewProvider {
    static var previews: some View {
        ContentView()
    }
}

