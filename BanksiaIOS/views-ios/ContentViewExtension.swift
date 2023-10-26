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
import MessageUI
import MobileCoreServices // << for UTI types


extension ContentView {
    
    func editingUpdateFen() {
        editingFenString = game.chessBoard.getFen(halfCount: 0, fullMoveCount: 0)
    }
    
    func createViews(size: CGSize) -> some View {
        NavigationView {
            VStack(spacing: 0) {
                /// Clock
                if !flipSize && !editing && !creatingImage {
                    HStack {
                        Spacer()
                        if game.analysicOnFlyMode {
                            Text("Anaysing...")
                                .foregroundColor(((self.timeCouter >> 2) & 1) == 0  ? Color.red : Color.black)
                        } else {
                            createClock(side: Side.white, timeLeft: whiteTimer)
                            create_result()
                            createClock(side: Side.black, timeLeft: blackTimer)
                        }
                        Spacer()
                    }
                    .background(Color.white)
                }
                
                /// Board & pieces
                create_BoardView(size: size)
                
                /// Moves, controllers
                if !flipSize {
                    if editing {
                        EditView(editingFenString: $editingFenString, editing: $editing)
                            .environmentObject(game)
                    } else {
                        createViews_NoEditing()
                    }
                    
#if SHOW_ADS
                    BannerVC(hasAd: $hasAd)
                        .frame(width: 320, height: hasAd ? 50 : 1, alignment: .center)
                    //        .frame(width: kGADAdSizeBanner.size.width, height: kGADAdSizeBanner.size.height)
#endif
                }
            }
            .navigationBarTitle("Board", displayMode: .inline)
            .navigationBarHidden(self.isNavigationBarHidden)
            .onAppear {
                self.cellWidth = size.width / 8
                self.isNavigationBarHidden = true
            }
            .onReceive(self.moveModel.clickIndex.receive(on: RunLoop.main)) { value in
                showMove(atIdx: value + 1)
            }
        }
        .navigationViewStyle(StackNavigationViewStyle())
    }
    
    func create_result() -> some View {
        HStack {
            Spacer()
            if !game.chessBoard.result.isNone() {
                Text(game.chessBoard.result.toShortString())
                    .foregroundColor(Color.red)
                Spacer()
            }
        }
    }
    
    func create_BoardView(size: CGSize) -> some View {
        ZStack {
            if flipSize && !editing {
                ScrollView(.horizontal) {
                    createViews_Board(cellWidth: size.height / 8)
                }
                .frame(width: size.width, height: size.height)
            } else {
                createViews_Board(cellWidth: size.width / (editing && UIDevice.current.userInterfaceIdiom == .pad ? 9 : 8))
            }
            if !editing && !game.chessBoard.result.isNone() && game.chessBoard.histList.count == game.displayingChessBoard.histList.count{
                gameOverView(size: size)
            }
        }
    }
    
    func createClock(side: Side, timeLeft: Int) -> some View {
        let w: CGFloat = 12
        let str = createClockString(side: side, timeLeft: timeLeft)
        let borderColor = self.game.chessBoard.side == side && ((self.timeCouter >> 2) & 1) == 0  ? Color.green : Color.gray
        
        return HStack {
            if side == Side.white {
                Rectangle()
                    .fill(Color.white)
                    .frame(width: w, height: w)
                    .border(borderColor, width: 2)
            }
            
            Text("\(str)")
                .lineLimit(1)
                .fixedSize()
                .foregroundColor(str.contains("-") && !game.analysicOnFlyMode ? Color.red : Color.black)
            
            if side == Side.black {
                Rectangle()
                    .fill(Color.black)
                    .frame(width: w, height: w)
                    .border(borderColor, width: 2)
            }
        }
    }
    
    func createViews_NoEditing() -> some View {
        VStack(spacing: 0) {
            
            /// info
            if userData.showAnalysis && engineOutput.hasEngineOutput {
                ScrollView(.horizontal) {
                    Text(engineOutput.displayString())
                        .font(.system(size: UIDevice.current.userInterfaceIdiom == .phone ? 12 : 18))
                        .frame(width: nil, height: nil, alignment: .topLeading)
                        .lineLimit(1)
                        .padding(2)
                }
                .background(Color(red: 0.95, green: 0.95, blue: 0.95))
                /// pv
                ScrollView(.horizontal) {
                    Text(engineOutput.pvString)
                        .font(.system(size: UIDevice.current.userInterfaceIdiom == .phone ? 12 : 18))
                        .frame(width: nil, height: nil, alignment: .topLeading)
                        .lineLimit(1)
                        .padding(2)
                }
                .background(Color(red: 0.95, green: 0.95, blue: 0.95))
            }
            
            MoveView(chessBoard: game.chessBoard, displayingAt: game.displayingChessBoard.histList.count, viewModel: moveModel)
                .environmentObject(userData)
                .frame(minWidth: 0, maxWidth: .infinity, minHeight: 0, maxHeight: .infinity, alignment: .topLeading)
                .gesture(DragGesture(minimumDistance: 3.0, coordinateSpace: .local)
                    .onEnded({ value in
                        if value.translation.width < 0 && value.translation.height > -30 && value.translation.height < 30 {
                            showMove(menuTask: .prev) /// left
                        }
                        else if value.translation.width > 0 && value.translation.height > -30 && value.translation.height < 30 {
                            showMove(menuTask: .next) /// right
                        }
                        else if value.translation.height < 0 && value.translation.width < 100 && value.translation.width > -100 {
                            showMove(menuTask: .begin) /// up
                        }
                        else if value.translation.height > 0 && value.translation.width < 100 && value.translation.width > -100 {
                            showMove(menuTask: .end) /// down
                        }
                    }))
            HStack {
                Button("Game", action: {
                    showingPopup_menu.toggle()
                })
                
                Spacer()
                
                NavigationLink(
                    destination: GameSetup(isNavigationBarHidden: self.$isNavigationBarHidden)
                        .environmentObject(game) ) {
                            Text("Level")
                        }
                
                Spacer()
                
                Button("Move", action: {
                    showingPopup_move.toggle()
                })
                
                Spacer()
                
                NavigationLink(
                    destination: OptionView(isNavigationBarHidden: self.$isNavigationBarHidden).environmentObject(userData) ) {
                        Text("Options")
                    }
            }.padding(6)
        }
    }
    
    func createMenuPopup(size: CGSize) -> some View {
        VStack(spacing: 0) {
            MenuView(width: size.width * (UIDevice.current.userInterfaceIdiom == .phone ? 0.6 : 0.3), menuClick: { (menuTask) -> Void in
                showingPopup_menu = false
                showingPopup_move = false
                
                switch (menuTask) {
                case .new:
                    newGame()
                    
                case .edit:
                    editingUpdateFen()
                    editing = true
                    flip = false
                    
                case .copy:
                    showingPopup_copy = true
                    
                case .cancel:
                    break
                    
                case .paste:
                    if let string = UIPasteboard.general.string, !string.isEmpty {
                        game.paste(string: string)
                    }
                    break
                    
                case .flip:
                    flip.toggle()
                }
            })
        }
        .frame(width: size.width * (UIDevice.current.userInterfaceIdiom == .phone ? 0.6 : 0.3), height: CGFloat(MenuTask.allCases.count) * ContentView.menuHeight)
        .background(Color(red: 0x3d, green: 0x5a, blue: 0x80))
        .cornerRadius(10.0)
        .shadow(color: Color(.sRGBLinear, white: 0, opacity: 0.13), radius: 10.0)
    }
    
    func createMenuPopup_move(size: CGSize) -> some View {
        VStack(spacing: 0) {
            MenuMoveView(width: size.width * (UIDevice.current.userInterfaceIdiom == .phone ? 0.6 : 0.3), menuClick: { (menuTask) -> Void in
                showingPopup_menu = false
                showingPopup_move = false
                
                switch (menuTask) {
                case .begin:
                    showMove(menuTask: .begin)
                    break
                case .prev:
                    showMove(menuTask: .prev)
                    break
                case .next:
                    showMove(menuTask: .next)
                    break
                case .end:
                    showMove(menuTask: .end)
                    break
                case .takeback:
                    takeBack()
                case .cancel:
                    break
                }
            })
        }
        .frame(width: size.width * (UIDevice.current.userInterfaceIdiom == .phone ? 0.6 : 0.3), height: CGFloat(MenuMoveTask.allCases.count) * ContentView.menuHeight)
        .background(Color(red: 0x3d, green: 0x5a, blue: 0x80))
        .cornerRadius(10.0)
        .shadow(color: Color(.sRGBLinear, white: 0, opacity: 0.13), radius: 10.0)
    }
    
    func showMove(atIdx: Int) {
        var ok = false
        while game.displayingChessBoard.histList.count > atIdx {
            game.displayingChessBoard.takeBack()
            ok = true
        }
        while game.displayingChessBoard.histList.count < atIdx {
            let move = game.chessBoard.histList[game.displayingChessBoard.histList.count].move
            game.displayingChessBoard.make(move: move)
            ok = true
        }
        if ok {
            _ = analysicOnFly_true()
        }
    }
    
    func showMove(menuTask: MenuMoveTask) {
        switch menuTask {
        case .begin:
            var ok = false
            while !game.displayingChessBoard.histList.isEmpty {
                game.displayingChessBoard.takeBack()
                ok = true
            }
            if ok {
                _ = analysicOnFly_true()
            }
            
        case .prev:
            guard let move = game.displayingChessBoard.histList.last?.move
            else {
                break
            }
            aniFrom = move.dest
            aniDest = move.from
            aniPromotion = move.promotion
            aniTask = MoveTask.prev
            
        case .next:
            if game.displayingChessBoard.histList.count >= game.chessBoard.histList.count {
                break
            }
            
            let move = game.chessBoard.histList[game.displayingChessBoard.histList.count].move
            aniFrom = move.from
            aniDest = move.dest
            aniPromotion = move.promotion
            aniTask = MoveTask.next
            
        case .end:
            var ok = false
            while game.displayingChessBoard.histList.count < game.chessBoard.histList.count {
                let move = game.chessBoard.histList[game.displayingChessBoard.histList.count].move
                game.displayingChessBoard.make(move: move)
                ok = true
            }
            if ok {
                _ = analysicOnFly_true()
            }
        default:
            break
        }
    }
    
    func createCopyPopup(size: CGSize) -> some View {
        ZStack {
            VStack(spacing: 20) {
                Picker(selection: $copyMode, label: Text("Copy")) {
                    ForEach (CopyMode.allCases, id:\.self) { em in
                        Text(em.getName()).tag(em.rawValue)
                    }
                }
                .padding()
                .pickerStyle(SegmentedPickerStyle())
                
                HStack {
                    Spacer()
                    Button("Copy", action: {
                        showingPopup_copy = false
                        DispatchQueue.main.asyncAfter(deadline: .now() + 1.0) {
                            doCopyEmail(copy: true)
                        }
                    })
                    Spacer()
                    Button("Email", action: {
                        showingPopup_copy = false
                        creatingImage = copyMode == CopyMode.img.rawValue
                        DispatchQueue.main.asyncAfter(deadline: .now() + 1.0) {
                            doCopyEmail(copy: false)
                        }
                    })
                    Spacer()
                    Button("Cancel", action: {
                        showingPopup_copy = false
                    })
                    Spacer()
                }
            }
            //.padding(EdgeInsets(top: 70, leading: 20, bottom: 40, trailing: 20))
            .frame(width: size.width * 6 / 8, height: size.width * 3 / 8)
            .background(Color(red: 0x3d, green: 0x5a, blue: 0x80))
            .cornerRadius(10.0)
            .shadow(color: Color(.sRGBLinear, white: 0, opacity: 0.13), radius: 10.0)
        }
    }

    func createBoardImage() -> UIImage? {
        let frame = CGRect(origin: geoBoardFrame.origin, size: CGSize(width: geoBoardFrame.width, height: geoBoardFrame.width))
        let image = UIApplication.shared.windows.first?.rootViewController?.view
            .asImage(rect: frame)
        
        creatingImage = false
        return image
    }
    
    func doCopyEmail(copy: Bool) {
        if copy {
            switch copyMode {
            case CopyMode.fen.rawValue:
                UIPasteboard.general.string = game.chessBoard.getFen()
            case CopyMode.pgn.rawValue:
                UIPasteboard.general.string = game.getGamePGN()
            case CopyMode.img.rawValue:
                if let image = createBoardImage() {
                    UIPasteboard.general.image = image
                }
            default:
                break
            }
        } else {
            switch copyMode {
            case CopyMode.fen.rawValue:
                presentMailCompose(subject: game.getGameTitle(), body: game.chessBoard.getFen(), attach: nil)
            case CopyMode.pgn.rawValue:
                presentMailCompose(subject: game.getGameTitle(), body: game.getGamePGN(), attach: nil)
            case CopyMode.img.rawValue:
                if let image = createBoardImage() {
                    presentMailCompose(subject: game.getGameTitle(), body: "", attach: image.pngData())
                }
            default:
                break
            }
        }
    }
}

/// MARK: The mail part
extension ContentView {
    
    /// Delegate for view controller as `MFMailComposeViewControllerDelegate`
    class MailDelegate: NSObject, MFMailComposeViewControllerDelegate {
        
        func mailComposeController(_ controller: MFMailComposeViewController,
                                   didFinishWith result: MFMailComposeResult,
                                   error: Error?) {
            controller.dismiss(animated: true)
        }
        
    }

    /// Present an mail compose view controller modally in UIKit environment
    func presentMailCompose(subject: String, body: String, attach: Data?) {
        guard MFMailComposeViewController.canSendMail(),
              let vc = UIApplication.shared.windows.first?.rootViewController else {
            return
        }
        let composeVC = MFMailComposeViewController()
        composeVC.mailComposeDelegate = mailComposeDelegate
        
        composeVC.setSubject(subject)
        
        if !body.isEmpty {
            composeVC.setMessageBody(body, isHTML:false)
        }
        
        if let attach = attach {
            composeVC.addAttachmentData(attach, mimeType: "image/png", fileName: "banksiaboard.png")
        }
        
        vc.present(composeVC, animated: true)
    }
}

/// MARK: The message part
extension ContentView {
    
    /// Delegate for view controller as `MFMessageComposeViewControllerDelegate`
    class MessageDelegate: NSObject, MFMessageComposeViewControllerDelegate {
        func messageComposeViewController(_ controller: MFMessageComposeViewController, didFinishWith result: MessageComposeResult) {
            // Customize here
            controller.dismiss(animated: true)
        }
        
    }
    
    /// Present an message compose view controller modally in UIKit environment
    func presentMessageCompose() {
        guard MFMessageComposeViewController.canSendText(),
              let vc = UIApplication.shared.windows.first?.rootViewController else {
            return
        }
        
        let composeVC = MFMessageComposeViewController()
        composeVC.messageComposeDelegate = messageComposeDelegate
        
        vc.present(composeVC, animated: true)
    }
}
