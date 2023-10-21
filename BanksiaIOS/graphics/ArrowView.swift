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

struct ArrowView: View {
    let cellWidth: CGFloat
    let from: Int
    let dest: Int
    let flip: Bool
    
    ///
    let color = Color.blue
    
    var body: some View {
        create().fill(color)
    }
    
    func posToPoint(pos: Int) -> CGPoint {
        let p = flip ? 63 - pos : pos
        return CGPoint(x: cellWidth * (CGFloat(p % 8) + 0.5), y: cellWidth * (CGFloat(p / 8) + 0.5))
    }
    
    func distance(_ a: CGPoint, _ b: CGPoint) -> CGFloat {
        let xDist = a.x - b.x
        let yDist = a.y - b.y
        return CGFloat(sqrt(xDist * xDist + yDist * yDist))
    }
    
    func create() -> Path {
        let sourcePos = posToPoint(pos: from)
        let targetPos = posToPoint(pos: dest)
        
        let len = distance(targetPos, sourcePos) /// - cellWidth / 3;
        let w = cellWidth / 20;
        
        var path = Path()
        
        /// rear side
        let x0: CGFloat = 0
        let y0: CGFloat = 0
        
        /// end of the arrow
        let p0 = CGPoint(x: x0, y: y0 + w)
        path.move(to: p0)
        
        path.addLine(to: CGPoint(x: x0, y: y0 - w))
        
        /// header of the arrow
        let tipX = x0 + len
        let x1 = tipX - cellWidth / 4;
        
        path.addLine(to: CGPoint(x: x1, y: y0 - w))
        
        path.addLine(to: CGPoint(x: x1, y: y0 - 2 * w))
        
        path.addLine(to: CGPoint(x: tipX, y: y0))
        
        path.addLine(to: CGPoint(x: x1, y: y0 + 2 * w))
        
        path.addLine(to: CGPoint(x: x1, y: y0 + w))
        
        path.closeSubpath()
        
        /// CGFloat(Double.pi)
        let angle: CGFloat = from % 8 != dest % 8 ? atan2(targetPos.y - sourcePos.y, targetPos.x - sourcePos.x) :
        targetPos.y > sourcePos.y ? CGFloat(Double.pi) / 2 : -CGFloat(Double.pi) / 2
        
        path = path.applying(CGAffineTransform(rotationAngle: angle))
            .applying(CGAffineTransform(translationX: sourcePos.x, y: sourcePos.y))
        
        return path
    }
    
}

struct ArrowView_Previews: PreviewProvider {
    static var previews: some View {
        ArrowView(cellWidth: 40, from: 6, dest: 21, flip: false)
    }
}
