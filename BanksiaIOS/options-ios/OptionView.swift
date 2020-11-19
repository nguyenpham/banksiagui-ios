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

struct OptionView: View {
  @Binding var isNavigationBarHidden: Bool
  @EnvironmentObject private var userData: UserData
  
  private let cellwidth: CGFloat = 20
  private let testString = "Rook e 4 O-O Pawn a 7 a 8 promoted to queen"
  
  var body: some View {
    ZStack {
      List {
        Section(header: Text("General")) {
          Toggle("Show coordinates", isOn: $userData.showCoordinates)
          Toggle("Show legal moves", isOn: $userData.showLegalMoves)
          Toggle("Show analysis", isOn: $userData.showAnalysis)
          Toggle("Show analysis arrows", isOn: $userData.showAnalysisArrows)
          Toggle("Show analysis in move list", isOn: $userData.showAnalysisMove)
          
          VStack {
            HStack {
              Text("Sound:")
              Spacer()
            }
            Picker(selection: $userData.soundMode, label: Text("Sound")) {
              ForEach (SoundMode.allCases, id:\.self) { em in
                Text(em.getName()).tag(em.rawValue)
              }
            }
            .pickerStyle(SegmentedPickerStyle())
            
            if userData.soundMode == SoundMode.speech.rawValue {
              Stepper(value: $userData.speechRate, in: 1...10) {
                HStack {
                  Spacer()
                  Text("Rate")
                  Text("\(userData.speechRate) / 10")
                }
              }
              
              VStack {
                NavigationLink(destination: SoundNameView().environmentObject(userData)) {
                  Spacer()
                  Text("Void name: \(self.userData.speechName)")
                }
              }
              
              HStack {
                Spacer()
                ScrollView(.horizontal) {
                  Text(testString)
                    .font(.system(size: 12))
                    .frame(width: nil, height: nil, alignment: .topLeading)
                    .lineLimit(1)
                    .padding(2)
                }
                .background(Color(red: 0.95, green: 0.95, blue: 0.95))
                
                Button("Test", action: {
                  SoundMng.speak(string: testString, voiceName: self.userData.speechName, rate: self.userData.speechRate)
                })
              }
            }
          }
        }
        
        Section(header: Text("Graphics")) {
          HStack {
            NavigationLink(destination: OptionPieceStyle().environmentObject(userData)) {
              Text("Piece set")
              Spacer()
              Text(self.userData.pieceStyle.getName())
              HStack {
                GPiece(piece: Piece(type: Piece.KING, side: Side.white),
                       pos: 0,
                       cellwidth: self.cellwidth,
                       pieceStyle: self.userData.pieceStyle,
                       flip: false,
                       editing: false,
                       humanSide: Side.none,
                       make: { (_, _, _, _, _) -> Bool in
                        return false },
                       tap: { pos in },
                       startDragging: { pos in },
                       isArrowEnd: false,
                       aniFrom: -1, aniDest: -1, aniPromotion: -1, aniTask: MoveTask.make,
                       selectedPos: -1, showLegalMark: false
                       
                )
                GPiece(piece: Piece(type: Piece.KING, side: Side.black),
                       pos: 0,
                       cellwidth: self.cellwidth,
                       pieceStyle: self.userData.pieceStyle,
                       flip: false,
                       editing: false,
                       humanSide: Side.none,
                       make: { (_, _, _, _, _) -> Bool in
                        return false },
                       tap: { pos in },
                       startDragging: { pos in },
                       isArrowEnd: false, aniFrom: -1, aniDest: -1, aniPromotion: -1, aniTask: MoveTask.make,
                       selectedPos: -1, showLegalMark: false
                )
              }
              .padding(0)
              .frame(width: self.cellwidth * 2, height: self.cellwidth)
            }
          }
          HStack {
            NavigationLink(destination: OptionColorScheme().environmentObject(userData)) {
              Text("Board color")
              Spacer()
              Text(self.userData.cellStyle.getName())
              ZStack {
                GBoard.createALine(x: self.cellwidth / 2,
                                   y: self.cellwidth / 2,
                                   cellwidth: self.cellwidth,
                                   cellStyle: self.userData.cellStyle,
                                   count: 2,
                                   firstBlack: false)
              }
              .padding(0)
              .frame(width: self.cellwidth * 2, height: self.cellwidth)
            }
          }
        }
        
        Section(header: Text("About")) {
          NavigationLink(destination: OptionUserName().environmentObject(userData)) {
            HStack {
              Text("User name")
              Spacer()
              Text(self.userData.userName)
            }
          }
          NavigationLink("About", destination: AboutView())
          NavigationLink("Help", destination: HelpView())
        }
      }
    }
    .navigationBarTitle("Options", displayMode: .inline)
    .onAppear {
      self.isNavigationBarHidden = false
    }
    .onDisappear() {
      self.userData.write()
      self.isNavigationBarHidden = true
    }
  }
}

//struct OptionView_Previews: PreviewProvider {
//    static var previews: some View {
//      let userData = UserData()
//      let isNavigationBarHidden = false
//      return OptionView(isNavigationBarHidden: $isNavigationBarHidden)
//        .environmentObject(userData)
//    }
//}
