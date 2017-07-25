//
//  ViewController.swift
//  LivingRoomLamp
//
//  Created by Johannes Schriewer on 23.07.17.
//  Copyright © 2017 Johannes Schriewer. All rights reserved.
//

import UIKit

class ViewController: UIViewController, UIGestureRecognizerDelegate {
    @IBOutlet weak var colorWheel: UIImageView!
    @IBOutlet weak var lowRing: UISlider!
    @IBOutlet weak var highRing: UISlider!
    @IBOutlet weak var brightness: UISlider!
    @IBOutlet weak var mode: UISegmentedControl!
    
    var tapRecognizer: UITapGestureRecognizer!
    var conn: NSURLConnection!
    var sendTimer: Timer?
    
    override func viewDidLoad() {
        super.viewDidLoad()
        tapRecognizer = UITapGestureRecognizer(target: self, action: #selector(self.colorChanged(_:)))
        tapRecognizer.delegate = self
        colorWheel.addGestureRecognizer(tapRecognizer)
        self.conn = NSURLConnection()
    }

    private func sendUpdate(_ sender: UIView) {
        if let timer = self.sendTimer {
            timer.invalidate()
        }
        
        self.sendTimer = Timer.scheduledTimer(withTimeInterval: 0.1, repeats: false) { timer in
            print("sending parameters")
            
            // prepare json data
            var json = [String: Any]()
            
            switch sender {
            case self.lowRing:
                json["lowPower"] = round((sender as! UISlider).value * 100) / 100.0
                break
            case self.highRing:
                json["highPower"] = round((sender as! UISlider).value * 100) / 100.0
                break
            case self.brightness:
                json["brightness"] = round((sender as! UISlider).value * 100) / 100.0
                break
            case self.mode:
                json["mode"] = ["white", "cinema", "moodlight"][(sender as! UISegmentedControl).selectedSegmentIndex]
                break
            default:
                print("What?")
            }
            
            let jsonData = try? JSONSerialization.data(withJSONObject: json)
            
            // create post request
            let url = URL(string: "http://192.168.2.102/parameters")!
//            let url = URL(string: "http://httpbin.org/post")!
            var request = URLRequest(url: url)
            request.httpMethod = "POST"
            request.httpShouldUsePipelining = false
            request.timeoutInterval = 5
            request.setValue("application/json", forHTTPHeaderField: "Content-Type")
            request.setValue("close", forHTTPHeaderField: "Connection")
            
            // insert json data to the request
            request.httpBody = jsonData
            
            let task = URLSession.shared.dataTask(with: request) { data, response, error in
                guard let data = data, error == nil else {
                    print(error?.localizedDescription ?? "No data")
                    return
                }
                let responseJSON = try? JSONSerialization.jsonObject(with: data, options: [])
                if let responseJSON = responseJSON as? [String: Any] {
                    print(responseJSON)
                }
            }
            
            task.resume()
        }
    }
    
    private func sendColorUpdate(_ hue: Double, _ saturation: Double) {
        self.sendTimer = Timer.scheduledTimer(withTimeInterval: 0.1, repeats: false) { timer in
            print("sending parameters")
            
            // prepare json data
            var json = [String: Any]()
            json["hue"] = round(hue * 100) / 100.0
            json["saturation"] = round(saturation * 100) / 100.0
            
            let jsonData = try? JSONSerialization.data(withJSONObject: json)
            
            // create post request
            let url = URL(string: "http://192.168.2.102/parameters")!
            //            let url = URL(string: "http://httpbin.org/post")!
            var request = URLRequest(url: url)
            request.httpMethod = "POST"
            request.httpShouldUsePipelining = false
            request.timeoutInterval = 5
            request.setValue("application/json", forHTTPHeaderField: "Content-Type")
            request.setValue("close", forHTTPHeaderField: "Connection")
            
            // insert json data to the request
            request.httpBody = jsonData
            
            let task = URLSession.shared.dataTask(with: request) { data, response, error in
                guard let data = data, error == nil else {
                    print(error?.localizedDescription ?? "No data")
                    return
                }
                let responseJSON = try? JSONSerialization.jsonObject(with: data, options: [])
                if let responseJSON = responseJSON as? [String: Any] {
                    print(responseJSON)
                }
            }
            
            task.resume()
        }
    }
    
    func colorChanged(_ recognizer: UITapGestureRecognizer? = nil) {
        guard let recognizer = recognizer else {
            return
        }
        
        var point = recognizer.location(ofTouch: 0, in: colorWheel)
        point.x = point.x - colorWheel.bounds.width / 2.0
        point.y = point.y - colorWheel.bounds.height / 2.0
        
        var deg: Double = atan2(Double(point.y), Double(point.x)) / Double.pi * 180.0
        deg -= 270
        if deg < -360 {
            deg += 720.0
        }
        if deg < 0 {
            deg += 360.0
        }
        
        var dist: Double = abs(sqrt(pow(Double(point.x), 2) + pow(Double(point.y), 2)))
        if dist > 130 {
            dist = 130
        }
        self.sendColorUpdate(deg/360.0, dist / 130.0)
        print("tapped, deg \(deg), distance \(dist)")
    }
    
    @IBAction func modeChanged(_ sender: UISegmentedControl) {
        print("mode is now \(sender.selectedSegmentIndex)")
        if sender.selectedSegmentIndex == 2 {
            if brightness.value > 1.0 {
                brightness.value = 1.0
                self.brightnessChanged(brightness)
            }
            brightness.maximumValue = 1.0
        } else {
            brightness.maximumValue = 2.0
        }
        self.sendUpdate(mode)
    }

    @IBAction func brightnessChanged(_ sender: UISlider) {
        print("brightness: \(sender.value)")
        self.sendUpdate(brightness)
    }

    @IBAction func lowRingChanged(_ sender: UISlider) {
        print("low: \(sender.value)")
        self.sendUpdate(lowRing)
    }
    
    @IBAction func highRingChanged(_ sender: UISlider) {
        print("high: \(sender.value)")
        self.sendUpdate(highRing)
    }
}

