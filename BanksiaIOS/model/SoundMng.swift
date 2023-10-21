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
import AVFoundation

enum Sound: Int, CaseIterable {
    case move, wrong, check, capture, win, alert1, alert2
    
    private static let allNames = [ "move", "error", "check", "cap", "win", "alert1", "alert2" ]
    
    func getName() -> String {
        return Sound.allNames[self.rawValue]
    }
}

class SoundMng {
    private var players = [AVAudioPlayer]()
    static private let synthesizer = AVSpeechSynthesizer()
    
    init() {
        loadSounds()
    }
    
    private func loadSounds() {
        for sound in Sound.allCases {
            let soundURL = Bundle.main.url(forResource: sound.getName(), withExtension: "wav")!
            if let player = try? AVAudioPlayer(contentsOf: soundURL) {
                player.prepareToPlay()
                players.append(player)
            }
        }
    }
    
    func playSound(sound: Sound){
        players[sound.rawValue].play()
    }
    
    func pos2SpeakString(pos: Int) -> String {
        let col = pos % 8, row = pos / 8
        return "\(columnNames[col]) \(8 - row)"
    }
    
    func speak(move: MoveFull, sanString: String, speechRate: Int, speechName: String) {
        var str = ""
        if move == MoveFull.illegalMoveFull {
            str = "wrong move"
        } else
        if sanString.isEmpty {
            if move.piece.type == Piece.KING && abs(move.from - move.dest) == 2 {
                str = "\(move.from < move.dest ? "short" : "long") castling"
            } else {
                
                let pieceName = PieceTypeStd(rawValue: move.piece.type)?.getName() ?? ""
                
                str = "\(pieceName) moves"
                str += " from \(pos2SpeakString(pos: move.from))"
                str += " to \(pos2SpeakString(pos: move.dest))"
                if move.promotion > PieceTypeStd.king.rawValue {
                    let promotion = PieceTypeStd(rawValue: move.promotion)?.getName() ?? ""
                    str += " and promote to \(promotion)"
                }
            }
        } else
        if sanString == "O-O" {
            str = "short castling"
        } else
        if sanString == "O-O-O" {
            str = "long castling"
        } else {
            var promoting = false
            str = "pawn"
            for c in sanString {
                let s = String(c)
                if s >= "A" && s <= "Z" {
                    let piece = PieceTypeStd.charactor2PieceType(str: s)
                    if promoting {
                        str += " promoted to \(piece.getName())"
                        break
                    }
                    
                    str = piece.getName()
                    continue
                }
                
                switch s {
                case "=":
                    promoting = true
                case "+":
                    str += " check"
                case "#":
                    str += " checkmate"
                default:
                    str += " \(s)"
                }
            }
        }
        print(str, ", sanString:", sanString)
        SoundMng.speak(string: str, voiceName: speechName, rate: speechRate)
    }
    
    static func speak(string: String, voiceName: String, rate: Int) {
        let utterance = AVSpeechUtterance(string: string)
        //    utterance.voice = AVSpeechSynthesisVoice(language: "en-GB")
        utterance.voice = AVSpeechSynthesisVoice(identifier: voiceName)
        utterance.rate = Float(rate) / 10.0
        synthesizer.speak(utterance)
    }
    
    func playSound(move: MoveFull, sanString: String, soundMode: SoundMode, speechRate: Int, speechName: String) {
        switch soundMode {
        case SoundMode.simple:
            playSound(sound: move == MoveFull.illegalMoveFull ? .wrong : .move)
        case SoundMode.speech:
            speak(move: move, sanString: sanString, speechRate: speechRate, speechName: speechName)
        default:
            break
        }
    }
}
