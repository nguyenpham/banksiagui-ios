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


class Result {
    var result: ResultType
    var reason: ReasonType
    var comment: String
    
    
    init(result: ResultType = ResultType.noresult, reason: ReasonType = ReasonType.noreason, comment: String = "") {
        self.result = result
        self.reason = reason
        self.comment = comment
    }
    
    func reset() {
        result = ResultType.noresult
        reason = ReasonType.noreason
        comment = ""
    }
    
    func isNone() -> Bool {
        return result == ResultType.noresult
    }
    
    func reasonString() -> String {
        return Result.reasonStrings[reason.rawValue]
    }
    
    func toShortString() -> String {
        return Result.resultType2String(type: result, shortFrom: true)
    }
    
    func toString() -> String {
        var str = toShortString();
        if (reason != ReasonType.noreason) {
            str += " (" + reasonString() + ")";
        }
        return str;
    }
    
    static let resultStrings: [String] = [
        "*", "1-0", "1/2-1/2", "0-1"
    ]
    static let resultStrings_short: [String] = [
        "*", "1-0", "0.5", "0-1"
    ]
    static let reasonStrings: [String] = [
        "*", "mate", "stalemate", "repetition", "resign", "fifty moves", "insufficient material",
        "illegal move", "timeout",
        "adjudication by lengths", "adjudication by egtb", "adjudication by engines' scores", "adjudication by human",
        "perpetual chase",
        "both perpetual chases", "extra comment", "crash", "abort",
    ]
    
    static func resultType2String(type: ResultType,  shortFrom: Bool) -> String {
        var t = type.rawValue
        if t < 0 || t > 3 {
            t = 0
        }
        return shortFrom ? Result.resultStrings_short[t] : Result.resultStrings[t];
    }
    
}
