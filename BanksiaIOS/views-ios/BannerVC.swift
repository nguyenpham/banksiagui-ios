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
import GoogleMobileAds
import UIKit

var hasAd = false

struct BannerVC: UIViewControllerRepresentable  {
  @Binding var hasAd: Bool

  func makeUIViewController(context: Context) -> UIViewController {
    let banner = GADBannerView(adSize: kGADAdSizeBanner)
    
    let viewController = UIViewController()
    banner.rootViewController = viewController
    //    banner.delegate = viewController
    banner.delegate = context.coordinator

    viewController.view.addSubview(banner)
    viewController.view.frame = CGRect(origin: .zero, size: kGADAdSizeBanner.size)
    if let bannerUnitID = Bundle.main.object(forInfoDictionaryKey: "GADBannerUnitID") as? String {
      banner.adUnitID = bannerUnitID
      banner.load(GADRequest())
    }
    return viewController
  }

  func updateUIViewController(_ uiViewController: UIViewController, context: Context) {}

  func makeCoordinator() -> BannerVC.Coordinator {
    Coordinator(self)
  }

  class Coordinator: NSObject, GADBannerViewDelegate {
    var parent: BannerVC

    init(_ bannerVC: BannerVC) {
      self.parent = bannerVC
    }

    func adViewDidReceiveAd(_ bannerView: GADBannerView){
      print("adViewDidReceiveAd")
      parent.hasAd = true
    }

    func adView(_ bannerView: GADBannerView, didFailToReceiveAdWithError error: GADRequestError) {
      print(error)
      parent.hasAd = false
    }
  }
}

//extension UIViewController: GADBannerViewDelegate {
//  public func adViewDidReceiveAd(_ bannerView: GADBannerView) {
//    print("ok ad")
//    hasAd = true
//  }
//
//  public func adView(_ bannerView: GADBannerView, didFailToReceiveAdWithError error: GADRequestError) {
//    print("fail ad")
//    print(error)
//  }
//}
