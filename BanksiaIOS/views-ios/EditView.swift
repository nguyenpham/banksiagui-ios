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
import MobileCoreServices // << for UTI types

struct EditView: View {
    @Binding var editingFenString: String
    @Binding var editing: Bool
    
    @EnvironmentObject var game: Game
    
    @State private var editingCastleWhiteOO = true
    @State private var editingCastleWhiteOOO = true
    @State private var editingCastleBlackOO = true
    @State private var editingCastleBlackOOO = true
    @State private var editingWhiteTurn = true
    @State private var showingAlert = false
    
    var body: some View {
        VStack(spacing: 0) {
            VStack {
                Divider()
                HStack(spacing: 10) {
                    createSideButton()
                    
                    Text("White:")
                    Button("O-O") {
                        editingCastleWhiteOO.toggle()
                        editingUpdateFen()
                    }
                    .background(self.editingCastleWhiteOO ? Color.green : Color.white)
                    
                    Button("O-O-O") {
                        editingCastleWhiteOOO.toggle()
                        editingUpdateFen()
                    }
                    .background(self.editingCastleWhiteOOO ? Color.green : Color.white)
                    
                    Text("Black:")
                    Button("O-O") {
                        editingCastleBlackOO.toggle()
                        editingUpdateFen()
                    }
                    .background(self.editingCastleBlackOO ? Color.green : Color.white)
                    
                    Button("O-O-O") {
                        editingCastleBlackOOO.toggle()
                        editingUpdateFen()
                    }
                    .background(self.editingCastleBlackOOO ? Color.green : Color.white)
                }
                
                Divider()
                HStack {
                    createCopyButton()
                    createFENView()
                }
                
                //        if UIDevice.current.userInterfaceIdiom == .phone {
                //          createFENView()
                //          Spacer()
                //          createCopyButton()
                //        } else {
                //          HStack {
                //            createCopyButton()
                //            createFENView()
                //          }
                //        }
            }
            .frame(minWidth: 0, maxWidth: .infinity, minHeight: 0, maxHeight: .infinity, alignment: .topLeading)
            
            HStack {
                Spacer()
                Button("Startpos", action: {
                    game.displayingChessBoard.setFen(fen: "")
                    editingUpdateFen()
                })
                Spacer()
                Button("Clear", action: {
                    game.displayingChessBoard.reset()
                    editingUpdateFen()
                })
                Spacer()
                Button("Cancel", action: {
                    _ = game.displayingChessBoard.clone(fromBoard: game.chessBoard)
                    editing = false
                })
                Spacer()
                Button("Done", action: {
                    if self.game.displayingChessBoard.isValid() {
                        game.newGame(fen: game.displayingChessBoard.getFen())
                        editing = false
                    } else {
                        showingAlert = true
                    }
                })
                .alert(isPresented: $showingAlert) {
                    Alert(title: Text("Error"), message: Text("Board invalid"))
                }
                Spacer()
            }
        }
        .frame(minWidth: 0, maxWidth: .infinity, minHeight: 0, maxHeight: .infinity, alignment: .topLeading)
        .onAppear() {
            self.editingUpdateFen(true)
        }
    }
    
    func createSideButton() -> some View {
        Button(action: {
            editingWhiteTurn.toggle()
            editingUpdateFen()
        }) {
            Text("\(editingWhiteTurn ? "White" : "Black") turn")
        }
    }
    
    func createCopyButton() -> some View {
        Button("Copy") {
            UIPasteboard.general.setValue(self.editingFenString,
                                          forPasteboardType: kUTTypePlainText as String)
        }
    }
    
    func createFENView() -> some View {
        ScrollView(.horizontal) {
            Text("FEN: \(self.editingFenString)")
                .textFieldStyle(RoundedBorderTextFieldStyle())
                .frame(width: nil, height: nil, alignment: .topLeading)
                .lineLimit(1)
        }
    }
    
    func editingUpdateFen(_ starting: Bool = false) {
        if starting {
            editingCastleWhiteOO = (game.displayingChessBoard.castleRights[Side.white.rawValue] & ChessBoard.CastleRight_short) != 0
            editingCastleWhiteOOO = (game.displayingChessBoard.castleRights[Side.white.rawValue] & ChessBoard.CastleRight_long) != 0
            editingCastleBlackOO = (game.displayingChessBoard.castleRights[Side.black.rawValue] & ChessBoard.CastleRight_short) != 0
            editingCastleBlackOOO = (game.displayingChessBoard.castleRights[Side.black.rawValue] & ChessBoard.CastleRight_long) != 0
            
            editingWhiteTurn = game.displayingChessBoard.side == Side.white
        } else {
            game.displayingChessBoard.castleRights[0] = 0
            game.displayingChessBoard.castleRights[1] = 0
            
            if editingCastleWhiteOO {
                game.displayingChessBoard.castleRights[Side.white.rawValue] |= ChessBoard.CastleRight_short
            }
            if editingCastleWhiteOOO {
                game.displayingChessBoard.castleRights[Side.white.rawValue] |= ChessBoard.CastleRight_long
            }
            if editingCastleBlackOO {
                game.displayingChessBoard.castleRights[Side.black.rawValue] |= ChessBoard.CastleRight_short
            }
            if editingCastleBlackOOO {
                game.displayingChessBoard.castleRights[Side.black.rawValue] |= ChessBoard.CastleRight_long
            }
            
            game.displayingChessBoard.side = editingWhiteTurn ? Side.white : Side.black
        }
        editingFenString = game.displayingChessBoard.getFen(halfCount: 0, fullMoveCount: 0)
    }
    
}

//struct EditView_Previews: PreviewProvider {
//    static var previews: some View {
//        EditView()
//    }
//}
