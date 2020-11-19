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

extension ContentView {
  func createViews(size: CGSize) -> some View {
    ScrollView([.horizontal, .vertical]) {
      VStack(spacing: 0) {
        HStack {
          if self.game.chessBoard.side == Side.white {
            Image("ballgreen")
              .resizable()
              .aspectRatio(contentMode: .fit)
              .frame(width: 5, height: 5)
              .padding(0)
          }

          createClocks(side: Side.white, timeLeft: whiteTimer)
          createClocks(side: Side.black, timeLeft: blackTimer)

          if self.game.chessBoard.side == Side.black {
            Image("ballgreen")
              .resizable()
              .aspectRatio(contentMode: .fit)
              .frame(width: 5, height: 5)
              .padding(0)
          }
          
          Spacer()
        }
        .frame(width: nil, height: 24)
        .padding(.leading, 6)
        
        ZStack {
          createViews_Board(cellWidth: size.width * (flipSize ? 2 : 1) / 8.25)
          if !flipSize {
            Image("e")
              .resizable()
              .aspectRatio(contentMode: .fill)
              .frame(width: size.width, height: size.width * 8 / 8.25)
              .onTapGesture {
                flipSize.toggle()
//                  scrollView.scrollTo(0)
              }
          }
        }

        createMenu(size: size)
      }
    }
    .onAppear {
      self.initData()
      //      self.geoSize = size
      self.cellWidth = size.width / 8.25
    }
  }
  
  func createClocks(side: Side, timeLeft: Int) -> some View {
    var playerName = ""
    if game.isHumanTurn(side: side) {
      playerName = side.getShortName()
    } else {
      playerName = game.getEngineDisplayName()
    }
    
    var timeLeftString = ""
    if !game.analysicOnFlyMode && (game.timeControlMode == .standard || game.timeControlMode == .movetime) {
      timeLeftString = clockString(side: side, second: timeLeft)
    }

    let font = Font.system(size: 12)

    return HStack(spacing: 0) {
      Text("\(playerName)")
        .lineLimit(1)
        .foregroundColor(Color.yellow)
        .font(font)
        .allowsTightening(true)
      Text(timeLeftString)
        .lineLimit(1)
        .foregroundColor(Color.white)
        .font(font)
        .allowsTightening(true)
    }
  }

  func createMenu(size: CGSize) -> some View {
    let boardPageHeight = 8 * size.width * (flipSize ? 2 : 1) / 8.25 + 24
    return VStack(spacing: 6) {
      Rectangle()
        .opacity(0.0)
        .frame(width: nil, height: max(6, size.height - boardPageHeight + 20))

      Button(action: {
        self.flipSize.toggle()
      }) {
        Text(self.flipSize ? "Normal board" : "Large board")
          .foregroundColor(Color.white)
          .frame(width: size.width * 0.8, height: nil, alignment: .center)
      }

      Button(action: {
        alwaysLargeBoardMode.toggle()
      }) {
        Text(alwaysLargeBoardMode ? "Always large board" : "Auto large board")
          .foregroundColor(Color.white)
          .frame(width: size.width * 0.8, height: nil, alignment: .center)
      }

      Button(action: {
        self.newGame()
      }) {
        Text("New game")
          .foregroundColor(Color.white)
          .frame(width: size.width * 0.8, height: nil, alignment: .center)
      }
      Button(action: { self.takeBack() }) {
        Text("Take back")
          .foregroundColor(Color.white)
          .frame(width: size.width * 0.8, height: nil, alignment: .center)
      }
      Button(action: { self.flip.toggle() }) {
        Text("Flip")
          .foregroundColor(Color.white)
          .frame(width: size.width * 0.8, height: nil, alignment: .center)
      }
    }
  }
  
}
