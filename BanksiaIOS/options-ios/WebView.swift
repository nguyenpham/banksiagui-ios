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
import WebKit

struct WebView: UIViewRepresentable {
    let text: String
    let isFile: Bool
    
    func makeUIView(context: Context) -> WKWebView {
        return WKWebView()
    }
    
    func updateUIView(_ uiView: WKWebView, context: Context) {
        if isFile {
            if let url = Bundle.main.url(forResource: text, withExtension: "html") { //, subdirectory: "data") {
                uiView.loadFileURL(url, allowingReadAccessTo: url)
            }
        } else {
            uiView.loadHTMLString(text, baseURL: nil)
        }
    }
}

struct Web_Previews: PreviewProvider {
    static var previews: some View {
        WebView(text: "<html><body>hey hellooo</body></html>", isFile: false)
    }
}
