// -----------------------------------------------------------------------------
// This file is part of vAmiga
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

//
// Extensions to CGImage
//

public extension CGImage {
    
    static func bitmapInfo() -> CGBitmapInfo {
        
        let noAlpha = CGImageAlphaInfo.noneSkipLast.rawValue
        let bigEn32 = CGBitmapInfo.byteOrder32Big.rawValue
    
        return CGBitmapInfo(rawValue: noAlpha | bigEn32)
    }
    
    static func dataProvider(data: UnsafeMutableRawPointer, size: CGSize) -> CGDataProvider? {
        
        let dealloc: CGDataProviderReleaseDataCallback = {
            
            (info: UnsafeMutableRawPointer?, data: UnsafeRawPointer, size: Int) -> Void in
            
            // Core Foundation objects are memory managed, aren't they?
            return
        }
        
        return CGDataProvider(dataInfo: nil,
                              data: data,
                              size: 4 * Int(size.width) * Int(size.height),
                              releaseData: dealloc)
    }
    
    // Creates a CGImage from a raw data stream in 32 bit big endian format
    static func make(data: UnsafeMutableRawPointer, size: CGSize) -> CGImage? {
        
        let w = Int(size.width)
        let h = Int(size.height)
        
        return CGImage(width: w, height: h,
                       bitsPerComponent: 8,
                       bitsPerPixel: 32,
                       bytesPerRow: 4 * w,
                       space: CGColorSpaceCreateDeviceRGB(),
                       bitmapInfo: bitmapInfo(),
                       provider: dataProvider(data: data, size: size)!,
                       decode: nil,
                       shouldInterpolate: false,
                       intent: CGColorRenderingIntent.defaultIntent)
    }
    
    // Creates a CGImage from a MTLTexture
    static func make(texture: MTLTexture, rect: CGRect) -> CGImage? {
        
        // Compute texture cutout
        //   (x,y) : upper left corner
        //   (w,h) : width and height
        let x = Int(CGFloat(texture.width) * rect.minX)
        let y = Int(CGFloat(texture.height) * rect.minY)
        let w = Int(CGFloat(texture.width) * rect.width)
        let h = Int(CGFloat(texture.height) * rect.height)
        
        // Get texture data as a byte stream
        guard let data = malloc(4 * w * h) else { return nil; }
        texture.getBytes(data,
                         bytesPerRow: 4 * w,
                         from: MTLRegionMake2D(x, y, w, h),
                         mipmapLevel: 0)
        
        return make(data: data, size: CGSize.init(width: w, height: h))
    }
}

//
// Extensions to NSColor
//

extension NSColor {
    
    convenience init(r: Int, g: Int, b: Int, a: Int = 255) {
        
        self.init(red: CGFloat(r) / 255,
                  green: CGFloat(g) / 255,
                  blue: CGFloat(b) / 255,
                  alpha: CGFloat(a) / 255)
    }
    
    convenience init(r: Double, g: Double, b: Double, a: Double = 1.0) {

        self.init(red: CGFloat(r),
                  green: CGFloat(g),
                  blue: CGFloat(b),
                  alpha: CGFloat(a))
    }
    
    convenience init(_ rgb: (Double, Double, Double)) {
        
        self.init(r: rgb.0, g: rgb.1, b: rgb.2)
    }
    
    convenience init(rgba: UInt32) {
        
        let r = CGFloat(rgba & 0xFF)
        let g = CGFloat((rgba >> 8) & 0xFF)
        let b = CGFloat((rgba >> 16) & 0xFF)
        
        self.init(red: r / 255.0, green: g / 255.0, blue: b / 255.0, alpha: 1.0)
    }
    
    convenience init(amigaRGB: UInt16) {
        
        let r = CGFloat((amigaRGB >> 8) & 0xF)
        let g = CGFloat((amigaRGB >> 4) & 0xF)
        let b = CGFloat(amigaRGB & 0xF)
        
        self.init(red: r / 15.0, green: g / 15.0, blue: b / 15.0, alpha: 1.0)
    }
    
    func rgb() -> (Int, Int, Int) {
        
        let r = Int(redComponent * 255)
        let g = Int(greenComponent * 255)
        let b = Int(blueComponent * 255)
        
        return (r, g, b)
    }
    
    func rgba() -> (Int, Int, Int, Int) {
        
        let r = Int(redComponent * 255)
        let g = Int(greenComponent * 255)
        let b = Int(blueComponent * 255)
        
        return (r, g, b, 255)
    }
    
    func amigaRGB() -> UInt16 {
        
        let r = UInt16(redComponent * 15.0)
        let g = UInt16(greenComponent * 15.0)
        let b = UInt16(blueComponent * 15.0)
        
        return (r << 8) | (g << 4) | b
    }
    
    static var warningColor: NSColor {
        return NSColor(named: NSColor.Name("warningColor"))!
    }
}

//
// Extensions to NSColor
//

extension NSColorWell {
    
    func setColor(_ rgb: (Double, Double, Double) ) {
        
        color = NSColor.init(r: rgb.0, g: rgb.1, b: rgb.2)
    }
}

//
// Extensions to NSImage
//

public extension NSImage {
    
    convenience init(color: NSColor, size: NSSize) {
        
        self.init(size: size)
        lockFocus()
        color.drawSwatch(in: NSRect(origin: .zero, size: size))
        unlockFocus()
    }

    static func make(texture: MTLTexture, rect: CGRect) -> NSImage? {
        
        guard let cgImage = CGImage.make(texture: texture, rect: rect) else {
            track("Failed to create CGImage.")
            return nil
        }
        
        let size = NSSize(width: cgImage.width, height: cgImage.height)
        return NSImage(cgImage: cgImage, size: size)
    }

    static func make(data: UnsafeMutableRawPointer, rect: CGSize) -> NSImage? {
        
        guard let cgImage = CGImage.make(data: data, size: rect) else {
            track("Failed to create CGImage.")
            return nil
        }
        
        let size = NSSize(width: cgImage.width, height: cgImage.height)
        return NSImage(cgImage: cgImage, size: size)
    }

    func expand(toSize size: NSSize) -> NSImage? {
 
        let newImage = NSImage.init(size: size)
    
        NSGraphicsContext.saveGraphicsState()
        newImage.lockFocus()

        let t = NSAffineTransform()
        t.translateX(by: 0.0, yBy: size.height)
        t.scaleX(by: 1.0, yBy: -1.0)
        t.concat()
        
        let inRect = NSRect.init(x: 0, y: 0, width: size.width, height: size.height)
        let fromRect = NSRect.init(x: 0, y: 0, width: self.size.width, height: self.size.height)
        let operation = NSCompositingOperation.copy
        self.draw(in: inRect, from: fromRect, operation: operation, fraction: 1.0)
        
        newImage.unlockFocus()
        NSGraphicsContext.restoreGraphicsState()
        
        return newImage
    }
    
    var cgImage: CGImage? {
        var rect = CGRect.init(origin: .zero, size: self.size)
        return self.cgImage(forProposedRect: &rect, context: nil, hints: nil)
    }
    
    func cgImageWH() -> (CGImage, Int, Int)? {
        
        if let cgi = cgImage(forProposedRect: nil, context: nil, hints: nil) {
            if cgi.width != 0 && cgi.height != 0 {
                return (cgi, cgi.width, cgi.height)
            }
        }
        return nil
    }
    
    func toData(vflip: Bool = false) -> UnsafeMutableRawPointer? {
        
        guard let (cgimage, width, height) = cgImageWH() else { return nil }
    
        // Allocate memory
        guard let data = malloc(height * width * 4) else { return nil; }
        let rawBitmapInfo =
            CGImageAlphaInfo.noneSkipLast.rawValue |
                CGBitmapInfo.byteOrder32Big.rawValue
        let bitmapContext = CGContext(data: data,
                                      width: width,
                                      height: height,
                                      bitsPerComponent: 8,
                                      bytesPerRow: 4 * width,
                                      space: CGColorSpaceCreateDeviceRGB(),
                                      bitmapInfo: rawBitmapInfo)
        
        // Flip image vertically if requested
        if vflip {
            bitmapContext?.translateBy(x: 0.0, y: CGFloat(height))
            bitmapContext?.scaleBy(x: 1.0, y: -1.0)
        }
        
        // Call 'draw' to fill the data array
        let rect = CGRect.init(x: 0, y: 0, width: width, height: height)
        bitmapContext?.draw(cgimage, in: rect)
        return data
    }
    
    func toTexture(device: MTLDevice) -> MTLTexture? {
 
        guard let (_, width, height) = cgImageWH() else { return nil }
        guard let data = toData(vflip: true) else { return nil }

        // Use a texture descriptor to create a texture
        let textureDescriptor = MTLTextureDescriptor.texture2DDescriptor(
            pixelFormat: MTLPixelFormat.rgba8Unorm,
            width: width,
            height: height,
            mipmapped: false)
        let texture = device.makeTexture(descriptor: textureDescriptor)
        
        // Copy data
        // texture?.replace(w: width, h: height, buffer: data)
        let region = MTLRegionMake2D(0, 0, width, height)
        texture?.replace(region: region, mipmapLevel: 0, withBytes: data, bytesPerRow: 4 * width)
        
        free(data)
        return texture
    }
    
    func representation(using: NSBitmapImageRep.FileType) -> Data? {
        
        let bitmap = tiffRepresentation?.bitmap
        return bitmap?.representation(using: using, properties: [:])
    }
        
    func resizeImage(width: CGFloat, height: CGFloat,
                     cutout: NSRect,
                     interpolation: NSImageInterpolation = .high) -> NSImage {
        
        let img = NSImage(size: CGSize(width: width, height: height))
        
        img.lockFocus()
        let ctx = NSGraphicsContext.current
        ctx?.imageInterpolation = interpolation
        self.draw(in: cutout,
                  from: NSRect.init(x: 0, y: 0, width: size.width, height: size.height),
                  operation: .sourceOver,
                  fraction: 1)
        img.unlockFocus()
        
        return img
    }
    
    func resize(width: CGFloat, height: CGFloat) -> NSImage {
        
        let cutout = NSRect.init(x: 0, y: 0, width: width, height: height)
        return resizeImage(width: width, height: height,
                           cutout: cutout)
    }

    func resizeSharp(width: CGFloat, height: CGFloat) -> NSImage {
        
        let cutout = NSRect.init(x: 0, y: 0, width: width, height: height)
        return resizeImage(width: width, height: height,
                           cutout: cutout,
                           interpolation: .none)
    }
        
    func roundCorners(withRadius radius: CGFloat) -> NSImage {
        
        let rect = NSRect.init(origin: NSPoint.zero, size: size)
        if
            let cgImage = self.cgImage,
            let context = CGContext(data: nil,
                                    width: Int(size.width),
                                    height: Int(size.height),
                                    bitsPerComponent: 8,
                                    bytesPerRow: 4 * Int(size.width),
                                    space: CGColorSpaceCreateDeviceRGB(),
                                    bitmapInfo: CGImageAlphaInfo.premultipliedFirst.rawValue) {
            context.beginPath()
            context.addPath(CGPath(roundedRect: rect, cornerWidth: radius, cornerHeight: radius, transform: nil))
            context.closePath()
            context.clip()
            context.draw(cgImage, in: rect)
            
            if let composedImage = context.makeImage() {
                return NSImage(cgImage: composedImage, size: size)
            }
        }
        
        return self
    }
    
    func roundCorners() -> NSImage {
        
        return self.roundCorners(withRadius: size.height / 30)
    }
        
    func makeGlossy() {
        
        let width  = size.width
        let height = size.height
        let glossy = NSImage(named: "glossy")
        let rect   = NSRect(x: 0, y: 0, width: width, height: height)
        
        lockFocus()
        let sourceOver = NSCompositingOperation.sourceOver
        draw(in: rect, from: NSRect.zero, operation: sourceOver, fraction: 1.0)
        glossy!.draw(in: rect, from: NSRect.zero, operation: sourceOver, fraction: 1.0)
        unlockFocus()
    }
    
    func tint(_ color: NSColor) {
        
        let imageRect = NSRect.init(origin: .zero, size: size)
        
        lockFocus()
        color.set()
        imageRect.fill(using: .sourceAtop)
        unlockFocus()
    }
    
    func darken() {
        
        tint(NSColor.init(red: 0.0, green: 0.0, blue: 0.0, alpha: 0.33))
    }

    func pressed() {
        
        tint(NSColor.init(red: 1.0, green: 0.0, blue: 0.0, alpha: 0.5))
        // tint(NSColor.init(red: 0.0, green: 0.0, blue: 0.0, alpha: 0.15))
    }

    func red() {
        
        tint(NSColor.init(red: 1.0, green: 0.0, blue: 0.0, alpha: 0.5))
    }
}
//
// Extensions to Renderer
//

extension Renderer {

    //
    // Image handling
    //

    func screenshot(texture: MTLTexture) -> NSImage? {

        // Use the blitter to copy the texture data back from the GPU
        let queue = texture.device.makeCommandQueue()!
        let commandBuffer = queue.makeCommandBuffer()!
        let blitEncoder = commandBuffer.makeBlitCommandEncoder()!
        blitEncoder.synchronize(texture: texture, slice: 0, level: 0)
        blitEncoder.endEncoding()
        commandBuffer.commit()
        commandBuffer.waitUntilCompleted()
        
        return NSImage.make(texture: texture, rect: textureRect)
    }
    
    func screenshot(afterUpscaling: Bool = true) -> NSImage? {
        
        if afterUpscaling {
            return screenshot(texture: upscaledTexture)
        } else {
            return screenshot(texture: mergeTexture)
        }
    }
    
    func createBackgroundTexture() -> MTLTexture? {

        let size = NSSize(width: 256, height: 256)
        let wallpaper = NSImage(color: .lightGray, size: size)
        return wallpaper.toTexture(device: device)
    }
}
