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
import AVFoundation

struct SoundNameView: View {
    @EnvironmentObject private var userData: UserData
    
    var body: some View {
        List (AVSpeechSynthesisVoice.speechVoices(), id:\.self) { voice in
            HStack {
                Text("\(voice.name)")
                Spacer()
                
                if voice.name == self.userData.speechName {
                    Image(systemName: "checkmark")
                        .resizable()
                        .frame(width: 20, height: 20)
                        .foregroundColor(.green)
                        .shadow(radius: 1)
                } else {
                    Rectangle()
                        .fill(Color.white)
                        .frame(width: 20, height: 20)
                }
            }.onTapGesture {
                self.userData.speechName = voice.name
            }
        }
    }
}

struct SoundNameView_Previews: PreviewProvider {
    static var previews: some View {
        SoundNameView()
    }
}
