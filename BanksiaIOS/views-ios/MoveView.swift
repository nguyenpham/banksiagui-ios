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
import UIKit
import SwiftUI
import Combine
import WebKit

class MoveModel: ObservableObject {
  var clickIndex = PassthroughSubject<Int, Never>()
}

struct MoveView: UIViewRepresentable { //}, WebViewHandlerDelegate {
  let chessBoard: ChessBoard
  let displayingAt: Int
  @ObservedObject var viewModel: MoveModel
  @EnvironmentObject private var userData: UserData
  @State var contentOffset = CGPoint.zero


  /// user-scalable = yes
  let htmlhead =
"""
<html><head>
<meta name="viewport" content="width=device-width, shrink-to-fit=NO">
<style>
a:link { text-decoration: none; }
.numb { font-family: monospace; }
.m { color: black; font-family: monospace;}
.l { color: #333333; font-family: monospace;}
.cur { color: white; background-color: black; font-weight: bold; font-family: monospace; }
.evaluation { color: red;}
.comment { color: darkgreen;}
</style></head>
<body>
"""
  
  let htmlend = "</body></html>"

  func updateMoveList() -> String {
    var str = htmlhead
    for (i, hist) in chessBoard.histList.enumerated() {
      let haveCounter = (i & 1) == 0
      
      if (haveCounter) {
        if i == 0 {
          str += "&nbsp;"
        }
        str += "<a class='\(i + 1 <= displayingAt ? "m" : "l")'>\(i / 2 + 1).</a>"
      }
      
      var comment = ""
      if userData.showAnalysisMove {
        comment = hist.computingString()
      }
      
      if !hist.comment.isEmpty && userData.showAnalysisMove {
        if !comment.isEmpty {
          comment += "; "
        }
        comment = hist.comment
      }
      
      if !comment.isEmpty {
        comment = " <span class='comment'> {\(comment)}</span> \n"
      }
      
      //        if (hist.mes != banksia::MoveEvaluationSymbol::none) {
      //            auto es = banksia::moveEvaluationSymbol2String(hist.mes);
      //            if (!es.empty()) {
      //                evaluate = QString(" <span class='evaluation'>%1</span> \n").arg(es.c_str());
      //            }
      //        }
      
      let moveString = hist.sanString
      let s = "<a class='\(i + 1 < displayingAt ? "m" : i + 1 == displayingAt ? "cur" : "l")' href='\(i)'> \(moveString) </a>\n\(comment)"
      
      str += s + " "; // + "&nbsp;";
    }
    
    str += htmlend
    return str
  }
  
  func makeCoordinator() -> Coordinator {
    Coordinator(self)
  }
  
  func makeUIView(context: Context) -> WKWebView {
    let webView = WKWebView()
    webView.navigationDelegate = context.coordinator
    webView.allowsBackForwardNavigationGestures = true
    webView.scrollView.isScrollEnabled = true
    return webView
  }
  
  func updateUIView(_ webView: WKWebView, context: Context) {
    DispatchQueue.main.async {
//      webView.scrollView.isScrollEnabled = false
      self.contentOffset = webView.scrollView.contentOffset
//      webView.loadHTMLString(updateMoveList(), baseURL: nil)
    }
    webView.loadHTMLString(updateMoveList(), baseURL: nil)
  }
  
  class Coordinator : NSObject, WKNavigationDelegate {
    var parent: MoveView
    
    init(_ moveView: MoveView) {
      self.parent = moveView
    }
    
    func webView(_ webView: WKWebView, decidePolicyFor navigationAction: WKNavigationAction, decisionHandler: @escaping (WKNavigationActionPolicy) -> Void) {
      decisionHandler(.allow)
      if let url = navigationAction.request.url?.absoluteString, let num = Int(url) {
        parent.viewModel.clickIndex.send(num)
      }
    }
    
    func webView(_ webView: WKWebView, didFinish: WKNavigation!) {
//      webView.scrollView.isScrollEnabled = true
//      DispatchQueue.main.async {
        webView.scrollView.setContentOffset(self.parent.contentOffset, animated: true)
//      }
    }
  }
}
