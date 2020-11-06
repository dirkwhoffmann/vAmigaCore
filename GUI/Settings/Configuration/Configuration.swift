// -----------------------------------------------------------------------------
// This file is part of vAmiga
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

/* Configuration
 *
 * This class stores all items that are specific to an individual emulator
 * instance. Each instance keeps its own object of this class inside the
 * document controller.
 *
 * See class "Preferences" for shared settings.
 */

class Configuration {

    var parent: MyController!
    var amiga: AmigaProxy { return parent.amiga }
    var renderer: Renderer { return parent.renderer }
    var gamePadManager: GamePadManager { return parent.gamePadManager }

    //
    // Roms
    //
        
    var extStart: Int {
        get { return amiga.getConfig(.OPT_EXT_START) }
        set { amiga.configure(.OPT_EXT_START, value: newValue) }
    }

    //
    // Hardware
    //
    
    var agnusRev: Int {
        get { return amiga.getConfig(.OPT_AGNUS_REVISION) }
        set { amiga.configure(.OPT_AGNUS_REVISION, value: newValue) }
    }
    var slowRamMirror: Bool {
        get { return amiga.getConfig(.OPT_SLOW_RAM_MIRROR) != 0 }
        set { amiga.configure(.OPT_SLOW_RAM_MIRROR, enable: newValue) }
    }
    var deniseRev: Int {
        get { return amiga.getConfig(.OPT_DENISE_REVISION) }
        set { amiga.configure(.OPT_DENISE_REVISION, value: newValue) }
    }
    var borderBlank: Bool {
        get { return amiga.getConfig(.OPT_BRDRBLNK) != 0 }
        set { amiga.configure(.OPT_BRDRBLNK, enable: newValue) }
    }
    var ciaRev: Int {
        get { return amiga.getConfig(.OPT_CIA_REVISION) }
        set { amiga.configure(.OPT_CIA_REVISION, value: newValue) }
    }
    var todBug: Bool {
        get { return amiga.getConfig(.OPT_TODBUG) != 0 }
        set { amiga.configure(.OPT_TODBUG, enable: newValue) }
    }
    var rtClock: Int {
        get { return amiga.getConfig(.OPT_RTC_MODEL) }
        set { amiga.configure(.OPT_RTC_MODEL, value: newValue) }
    }
    var chipRam: Int {
        get { return amiga.getConfig(.OPT_CHIP_RAM) }
        set { amiga.configure(.OPT_CHIP_RAM, value: newValue) }
    }
    var slowRam: Int {
        get { return amiga.getConfig(.OPT_SLOW_RAM) }
        set { amiga.configure(.OPT_SLOW_RAM, value: newValue) }
    }
    var fastRam: Int {
        get { return amiga.getConfig(.OPT_FAST_RAM) }
        set { amiga.configure(.OPT_FAST_RAM, value: newValue) }
    }
    var bankMap: Int {
        get { return amiga.getConfig(.OPT_BANKMAP) }
        set { amiga.configure(.OPT_BANKMAP, value: newValue) }
    }
    var ramInitPattern: Int {
        get { return amiga.getConfig(.OPT_RAM_INIT_PATTERN) }
        set { amiga.configure(.OPT_RAM_INIT_PATTERN, value: newValue) }
    }
    var unmappingType: Int {
        get { return amiga.getConfig(.OPT_UNMAPPING_TYPE) }
        set { amiga.configure(.OPT_UNMAPPING_TYPE, value: newValue) }
    }

    //
    // Peripherals
    //

    func dfnConnected(_ n: Int) -> Bool {
        precondition(0 <= n && n <= 3)
        return amiga.getConfig(.OPT_DRIVE_CONNECT, drive: n) != 0
    }
    func setDfnConnected(_ n: Int, connect: Bool) {
        precondition(0 <= n && n <= 3)
        amiga.configure(.OPT_DRIVE_CONNECT, drive: n, enable: connect)
    }
    func dfnType(_ n: Int) -> Int {
        precondition(0 <= n && n <= 3)
        return amiga.getConfig(.OPT_DRIVE_TYPE, drive: n)
    }
    func setDfnType(_ n: Int, type: Int) {
        precondition(0 <= n && n <= 3)
        amiga.configure(.OPT_DRIVE_TYPE, drive: n, value: type)
    }
    var df0Connected: Bool {
        get { return dfnConnected(0) }
        set { setDfnConnected(0, connect: newValue) }
    }
    var df0Type: Int {
        get { return dfnType(0) }
        set { setDfnType(0, type: newValue) }
    }
    var df1Connected: Bool {
        get { return dfnConnected(1) }
        set { setDfnConnected(1, connect: newValue) }
    }
    var df1Type: Int {
        get { return dfnType(1) }
        set { setDfnType(1, type: newValue) }
    }
    var df2Connected: Bool {
        get { return dfnConnected(2) }
        set { setDfnConnected(2, connect: newValue) }
    }
    var df2Type: Int {
        get { return dfnType(2) }
        set { setDfnType(2, type: newValue) }
    }
    var df3Connected: Bool {
        get { return dfnConnected(3) }
        set { setDfnConnected(3, connect: newValue) }
    }
    var df3Type: Int {
        get { return dfnType(3) }
        set { setDfnType(3, type: newValue) }
    }
    var gameDevice1 = PeripheralsDefaults.std.gameDevice1 {
        didSet {
                         
            // Try to connect the device (may disconnect the other device)
            gamePadManager.connect(slot: gameDevice1, port: 1)
            gamePadManager.listDevices()

            // Avoid double mappings
            if gameDevice1 != -1 && gameDevice1 == gameDevice2 {
                gameDevice2 = -1
            }
            
            parent.toolbar.validateVisibleItems()
        }
    }
    var gameDevice2 = PeripheralsDefaults.std.gameDevice2 {
        didSet {
 
            // Try to connect the device (may disconnect the other device)
            gamePadManager.connect(slot: gameDevice2, port: 2)
            gamePadManager.listDevices()

            // Avoid double mappings
            if gameDevice2 != -1 && gameDevice2 == gameDevice1 {
                gameDevice1 = -1
            }
            
            parent.toolbar.validateVisibleItems()
        }
    }
    var serialDevice: Int {
        get { return amiga.getConfig(.OPT_SERIAL_DEVICE) }
        set { amiga.configure(.OPT_SERIAL_DEVICE, value: newValue) }
    }

    //
    // Compatibility
    //

    var blitterAccuracy: Int {
        get { return amiga.getConfig(.OPT_BLITTER_ACCURACY) }
        set { amiga.configure(.OPT_BLITTER_ACCURACY, value: newValue) }
    }
    var eClockSyncing: Bool {
        get { return amiga.getConfig(.OPT_ECLOCK_SYNCING) != 0 }
        set { amiga.configure(.OPT_ECLOCK_SYNCING, enable: newValue) }
    }
    var slowRamDelay: Bool {
        get { return amiga.getConfig(.OPT_SLOW_RAM_DELAY) != 0 }
        set { amiga.configure(.OPT_SLOW_RAM_DELAY, enable: newValue) }
    }
    var clxSprSpr: Bool {
        get { return amiga.getConfig(.OPT_CLX_SPR_SPR) != 0 }
        set { amiga.configure(.OPT_CLX_SPR_SPR, enable: newValue) }
    }
    var clxSprPlf: Bool {
        get { return amiga.getConfig(.OPT_CLX_SPR_PLF) != 0 }
        set { amiga.configure(.OPT_CLX_SPR_PLF, enable: newValue) }
    }
    var clxPlfPlf: Bool {
        get { return amiga.getConfig(.OPT_CLX_PLF_PLF) != 0 }
        set { amiga.configure(.OPT_CLX_PLF_PLF, enable: newValue) }
    }
    var driveSpeed: Int {
        get { return amiga.getConfig(.OPT_DRIVE_SPEED) }
        set { amiga.configure(.OPT_DRIVE_SPEED, value: newValue) }
    }
    var mechanicalDelays: Bool {
        get { return amiga.getConfig(.OPT_EMULATE_MECHANICS, drive: 0) != 0 }
        set {
            amiga.configure(.OPT_EMULATE_MECHANICS, drive: 0, enable: newValue)
            amiga.configure(.OPT_EMULATE_MECHANICS, drive: 1, enable: newValue)
            amiga.configure(.OPT_EMULATE_MECHANICS, drive: 2, enable: newValue)
            amiga.configure(.OPT_EMULATE_MECHANICS, drive: 3, enable: newValue)
        }
    }
    var lockDskSync: Bool {
        get { return amiga.getConfig(.OPT_LOCK_DSKSYNC) != 0 }
        set { amiga.configure(.OPT_LOCK_DSKSYNC, enable: newValue) }
    }
    var autoDskSync: Bool {
        get { return amiga.getConfig(.OPT_AUTO_DSKSYNC) != 0 }
        set { amiga.configure(.OPT_AUTO_DSKSYNC, enable: newValue) }
    }
    var accurateKeyboard: Bool {
        get { return amiga.getConfig(.OPT_ACCURATE_KEYBOARD) != 0 }
        set { amiga.configure(.OPT_ACCURATE_KEYBOARD, enable: newValue) }
    }
    
    //
    // Audio settings
    //

    var vol0: Int {
        get { return amiga.getConfig(.OPT_AUDVOL0) }
        set { amiga.configure(.OPT_AUDVOL0, value: newValue) }
    }
    var vol1: Int {
        get { return amiga.getConfig(.OPT_AUDVOL1) }
        set { amiga.configure(.OPT_AUDVOL1, value: newValue) }
    }
    var vol2: Int {
        get { return amiga.getConfig(.OPT_AUDVOL2) }
        set { amiga.configure(.OPT_AUDVOL2, value: newValue) }
    }
    var vol3: Int {
        get { return amiga.getConfig(.OPT_AUDVOL3) }
        set { amiga.configure(.OPT_AUDVOL3, value: newValue) }
    }
    var pan0: Int {
        get { return amiga.getConfig(.OPT_AUDPAN0) }
        set { amiga.configure(.OPT_AUDPAN0, value: newValue) }
    }
    var pan1: Int {
        get { return amiga.getConfig(.OPT_AUDPAN1) }
        set { amiga.configure(.OPT_AUDPAN1, value: newValue) }
    }
    var pan2: Int {
        get { return amiga.getConfig(.OPT_AUDPAN2) }
        set { amiga.configure(.OPT_AUDPAN2, value: newValue) }
    }
    var pan3: Int {
        get { return amiga.getConfig(.OPT_AUDPAN3) }
        set { amiga.configure(.OPT_AUDPAN3, value: newValue) }
    }
    var volL: Int {
        get { return amiga.getConfig(.OPT_AUDVOLL) }
        set { amiga.configure(.OPT_AUDVOLL, value: newValue) }
    }
    var volR: Int {
        get { return amiga.getConfig(.OPT_AUDVOLR) }
        set { amiga.configure(.OPT_AUDVOLR, value: newValue) }
    }
    var samplingMethod: Int {
        get { return amiga.getConfig(.OPT_SAMPLING_METHOD) }
        set { amiga.configure(.OPT_SAMPLING_METHOD, value: newValue) }
    }
    var filterType: Int {
        get { return amiga.getConfig(.OPT_FILTER_TYPE) }
        set { amiga.configure(.OPT_FILTER_TYPE, value: newValue) }
    }
    var filterAlwaysOn: Bool {
        get { return amiga.getConfig(.OPT_FILTER_ALWAYS_ON) != 0}
        set { amiga.configure(.OPT_FILTER_ALWAYS_ON, enable: newValue) }
    }
    
    //
    // Video settings
    //
    
    var palette: Palette {
        get { return amiga.denise.palette }
        set { amiga.denise.palette = newValue }
    }
    var brightness: Double {
        get { return amiga.denise.brightness }
        set { amiga.denise.brightness = newValue }
    }
    var contrast: Double {
        get { return amiga.denise.contrast }
        set { amiga.denise.contrast = newValue }
    }
    var saturation: Double {
        get { return amiga.denise.saturation }
        set { amiga.denise.saturation = newValue }
    }
    var hCenter = VideoDefaults.tft.hCenter {
        didSet { renderer.updateTextureRect() }
    }
    var vCenter = VideoDefaults.tft.vCenter {
        didSet { renderer.updateTextureRect() }
    }
    var hZoom = VideoDefaults.tft.hZoom {
        didSet { renderer.updateTextureRect() }
    }
    var vZoom = VideoDefaults.tft.vZoom {
        didSet { renderer.updateTextureRect() }
    }
    var enhancer = VideoDefaults.tft.enhancer {
        didSet { if !renderer.selectEnhancer(enhancer) { enhancer = oldValue } }
    }
    var upscaler = VideoDefaults.tft.upscaler {
        didSet { if !renderer.selectUpscaler(upscaler) { upscaler = oldValue } }
    }
    var blur = VideoDefaults.tft.blur {
        didSet { renderer.shaderOptions.blur = blur }
    }
    var blurRadius = VideoDefaults.tft.blurRadius {
        didSet { renderer.shaderOptions.blurRadius = blurRadius }
    }
    var bloom = VideoDefaults.tft.bloom {
        didSet { renderer.shaderOptions.bloom = bloom }
    }
    var bloomRadius = VideoDefaults.tft.bloomRadius {
        didSet { renderer.shaderOptions.bloomRadius = bloomRadius }
    }
    var bloomBrightness = VideoDefaults.tft.bloomBrightness {
        didSet { renderer.shaderOptions.bloomBrightness = bloomBrightness }
    }
    var bloomWeight = VideoDefaults.tft.bloomWeight {
        didSet { renderer.shaderOptions.bloomWeight = bloomWeight }
    }
    var flicker = VideoDefaults.tft.flicker {
        didSet { renderer.shaderOptions.flicker = flicker }
    }
    var flickerWeight = VideoDefaults.tft.flickerWeight {
        didSet { renderer.shaderOptions.flickerWeight = flickerWeight }
    }
    var dotMask = VideoDefaults.tft.dotMask {
        didSet {
            renderer.shaderOptions.dotMask = dotMask
            renderer.buildDotMasks()
        }
    }
    var dotMaskBrightness = VideoDefaults.tft.dotMaskBrightness {
        didSet {
            renderer.shaderOptions.dotMaskBrightness = dotMaskBrightness
            renderer.buildDotMasks()
        }
    }
    var scanlines = VideoDefaults.tft.scanlines {
        didSet { renderer.shaderOptions.scanlines = scanlines }
    }
    var scanlineBrightness = VideoDefaults.tft.scanlineBrightness {
        didSet { renderer.shaderOptions.scanlineBrightness = scanlineBrightness }
    }
    var scanlineWeight = VideoDefaults.tft.scanlineWeight {
        didSet { renderer.shaderOptions.scanlineWeight = scanlineWeight }
    }
    var disalignment = VideoDefaults.tft.disalignment {
        didSet { renderer.shaderOptions.disalignment = disalignment }
    }
    var disalignmentH = VideoDefaults.tft.disalignmentH {
        didSet { renderer.shaderOptions.disalignmentH = disalignmentH }
    }
    var disalignmentV = VideoDefaults.tft.disalignmentV {
        didSet { renderer.shaderOptions.disalignmentV = disalignmentV }
    }
    
    init(with controller: MyController) { parent = controller }

    //
    // Roms
    //
    
    func loadRomUserDefaults() {

        let defaults = UserDefaults.standard
        
        amiga.suspend()

        extStart = defaults.integer(forKey: Keys.Rom.extStart)
        
        if let url = UserDefaults.womUrl {
            track("Seeking Wom")
            amiga.mem.loadRom(fromFile: url)
        }
        if let url = UserDefaults.romUrl {
            track("Seeking Rom")
            amiga.mem.loadRom(fromFile: url)
        }
        if let url = UserDefaults.extUrl {
            track("Seeking Ext")
            amiga.mem.loadExt(fromFile: url)
        }
        
        amiga.resume()
    }
    
    func saveRomUserDefaults() {
        
        let fm = FileManager.default
        let defaults = UserDefaults.standard

        amiga.suspend()
                
        defaults.set(extStart, forKey: Keys.Rom.extStart)
        
        if let url = UserDefaults.womUrl {
            track("Saving Wom")
            try? fm.removeItem(at: url)
            amiga.mem.saveWom(url)
        }
        if let url = UserDefaults.romUrl {
            track("Saving Rom")
            try? fm.removeItem(at: url)
            amiga.mem.saveRom(url)
        }
        if let url = UserDefaults.extUrl {
            track("Saving Ext")
            try? fm.removeItem(at: url)
            amiga.mem.saveExt(url)
        }
                
        amiga.resume()
    }

    //
    // Hardware
    //
    
    func loadHardwareDefaults(_ defaults: HardwareDefaults) {
        
        amiga.suspend()
        
        agnusRev = defaults.agnusRev.rawValue
        slowRamMirror = defaults.slowRamMirror
        deniseRev = defaults.deniseRev.rawValue
        borderBlank = defaults.borderBlank
        ciaRev = defaults.ciaRev.rawValue
        todBug = defaults.todBug
        rtClock = defaults.realTimeClock.rawValue
        
        chipRam = defaults.chipRam
        slowRam = defaults.slowRam
        fastRam = defaults.fastRam
        ramInitPattern = defaults.ramInitPattern.rawValue

        bankMap = defaults.bankMap.rawValue
        unmappingType = defaults.unmappingType.rawValue
        
        amiga.resume()
    }
    
    func loadHardwareUserDefaults() {
        
        let defaults = UserDefaults.standard
        
        amiga.suspend()
        
        agnusRev = defaults.integer(forKey: Keys.Hrw.agnusRev)
        slowRamMirror = defaults.bool(forKey: Keys.Hrw.slowRamMirror)
        deniseRev = defaults.integer(forKey: Keys.Hrw.deniseRev)
        borderBlank = defaults.bool(forKey: Keys.Hrw.borderBlank)
        ciaRev = defaults.integer(forKey: Keys.Hrw.ciaRev)
        todBug = defaults.bool(forKey: Keys.Hrw.todBug)
        rtClock = defaults.integer(forKey: Keys.Hrw.realTimeClock)

        chipRam = defaults.integer(forKey: Keys.Hrw.chipRam)
        slowRam = defaults.integer(forKey: Keys.Hrw.slowRam)
        fastRam = defaults.integer(forKey: Keys.Hrw.fastRam)
        ramInitPattern = defaults.integer(forKey: Keys.Hrw.ramInitPattern)

        bankMap = defaults.integer(forKey: Keys.Hrw.bankMap)
        unmappingType = defaults.integer(forKey: Keys.Hrw.unmappingType)

        amiga.resume()
    }
    
    func saveHardwareUserDefaults() {
        
        track()
        
        let defaults = UserDefaults.standard

        defaults.set(agnusRev, forKey: Keys.Hrw.agnusRev)
        defaults.set(slowRamMirror, forKey: Keys.Hrw.slowRamMirror)
        defaults.set(deniseRev, forKey: Keys.Hrw.deniseRev)
        defaults.set(borderBlank, forKey: Keys.Hrw.borderBlank)
        defaults.set(ciaRev, forKey: Keys.Hrw.ciaRev)
        defaults.set(todBug, forKey: Keys.Hrw.todBug)
        defaults.set(rtClock, forKey: Keys.Hrw.realTimeClock)

        defaults.set(chipRam, forKey: Keys.Hrw.chipRam)
        defaults.set(slowRam, forKey: Keys.Hrw.slowRam)
        defaults.set(fastRam, forKey: Keys.Hrw.fastRam)
        defaults.set(ramInitPattern, forKey: Keys.Hrw.ramInitPattern)

        defaults.set(bankMap, forKey: Keys.Hrw.bankMap)
        defaults.set(unmappingType, forKey: Keys.Hrw.unmappingType)
    }

    //
    // Peripherals
    //
    
    func loadPeripheralsDefaults(_ defaults: PeripheralsDefaults) {
        
        amiga.suspend()
        
        df0Connected = defaults.driveConnect[0]
        df1Connected = defaults.driveConnect[1]
        df2Connected = defaults.driveConnect[2]
        df3Connected = defaults.driveConnect[3]
        df0Type = defaults.driveType[0].rawValue
        df1Type = defaults.driveType[1].rawValue
        df2Type = defaults.driveType[2].rawValue
        df3Type = defaults.driveType[3].rawValue
        
        gameDevice1 = defaults.gameDevice1
        gameDevice2 = defaults.gameDevice2
        serialDevice = defaults.serialDevice.rawValue
        
        amiga.resume()
    }
    
    func loadPeripheralsUserDefaults() {
        
        let defaults = UserDefaults.standard
        
        amiga.suspend()
        
        df0Connected = defaults.bool(forKey: Keys.Per.df0Connect)
        df1Connected = defaults.bool(forKey: Keys.Per.df1Connect)
        df2Connected = defaults.bool(forKey: Keys.Per.df2Connect)
        df3Connected = defaults.bool(forKey: Keys.Per.df3Connect)
        df0Type = defaults.integer(forKey: Keys.Per.df0Type)
        df1Type = defaults.integer(forKey: Keys.Per.df1Type)
        df2Type = defaults.integer(forKey: Keys.Per.df2Type)
        df3Type = defaults.integer(forKey: Keys.Per.df3Type)

        gameDevice1 = defaults.integer(forKey: Keys.Per.gameDevice1)
        gameDevice2 = defaults.integer(forKey: Keys.Per.gameDevice2)
        serialDevice = defaults.integer(forKey: Keys.Per.serialDevice)
        
        amiga.resume()
    }
    
    func savePeripheralsUserDefaults() {
        
        track()
        
        let defaults = UserDefaults.standard
        
        defaults.set(df0Connected, forKey: Keys.Per.df0Connect)
        defaults.set(df1Connected, forKey: Keys.Per.df1Connect)
        defaults.set(df2Connected, forKey: Keys.Per.df2Connect)
        defaults.set(df3Connected, forKey: Keys.Per.df3Connect)
        defaults.set(df0Type, forKey: Keys.Per.df0Type)
        defaults.set(df1Type, forKey: Keys.Per.df1Type)
        defaults.set(df2Type, forKey: Keys.Per.df2Type)
        defaults.set(df3Type, forKey: Keys.Per.df3Type)

        defaults.set(gameDevice1, forKey: Keys.Per.gameDevice1)
        defaults.set(gameDevice2, forKey: Keys.Per.gameDevice2)
        defaults.set(serialDevice, forKey: Keys.Per.serialDevice)

    }
    
    //
    // Compatibility
    //
    
    func loadCompatibilityDefaults(_ defaults: CompatibilityDefaults) {
         
        amiga.suspend()
        
        blitterAccuracy = defaults.blitterAccuracy

        eClockSyncing = defaults.eClockSyncing
        slowRamDelay = defaults.slowRamDelay

        clxSprSpr = defaults.clxSprSpr
        clxSprPlf = defaults.clxSprPlf
        clxPlfPlf = defaults.clxPlfPlf
                        
        driveSpeed = defaults.driveSpeed
        mechanicalDelays = defaults.mechanicalDelays
        lockDskSync = defaults.lockDskSync
        autoDskSync = defaults.autoDskSync
        
        accurateKeyboard = defaults.accurateKeyboard

        amiga.resume()
     }
    
    func loadCompatibilityUserDefaults() {
        
        let defaults = UserDefaults.standard
        
        amiga.suspend()
        
        blitterAccuracy = defaults.integer(forKey: Keys.Com.blitterAccuracy)

        eClockSyncing = defaults.bool(forKey: Keys.Com.eClockSyncing)
        slowRamDelay = defaults.bool(forKey: Keys.Com.slowRamDelay)

        clxSprSpr = defaults.bool(forKey: Keys.Com.clxSprSpr)
        clxSprPlf = defaults.bool(forKey: Keys.Com.clxSprPlf)
        clxPlfPlf = defaults.bool(forKey: Keys.Com.clxPlfPlf)
                
        driveSpeed = defaults.integer(forKey: Keys.Com.driveSpeed)
        mechanicalDelays = defaults.bool(forKey: Keys.Com.mechanicalDelays)
        lockDskSync = defaults.bool(forKey: Keys.Com.lockDskSync)
        autoDskSync = defaults.bool(forKey: Keys.Com.autoDskSync)

        accurateKeyboard = defaults.bool(forKey: Keys.Com.accurateKeyboard)

        amiga.resume()
     }
    
    func saveCompatibilityUserDefaults() {
        
        track()
        
        let defaults = UserDefaults.standard
        
        defaults.set(blitterAccuracy, forKey: Keys.Com.blitterAccuracy)

        defaults.set(eClockSyncing, forKey: Keys.Com.eClockSyncing)
        defaults.set(slowRamDelay, forKey: Keys.Com.slowRamDelay)

        defaults.set(clxSprSpr, forKey: Keys.Com.clxSprSpr)
        defaults.set(clxSprPlf, forKey: Keys.Com.clxSprPlf)
        defaults.set(clxPlfPlf, forKey: Keys.Com.clxPlfPlf)
                
        defaults.set(driveSpeed, forKey: Keys.Com.driveSpeed)
        defaults.set(mechanicalDelays, forKey: Keys.Com.mechanicalDelays)
        defaults.set(lockDskSync, forKey: Keys.Com.lockDskSync)
        defaults.set(autoDskSync, forKey: Keys.Com.autoDskSync)
        
        defaults.set(accurateKeyboard, forKey: Keys.Com.accurateKeyboard)
    }
    
    //
    // Audio
    //
    
    func loadAudioDefaults(_ defaults: AudioDefaults) {
        
        amiga.suspend()
        
        vol0 = defaults.vol0
        vol1 = defaults.vol1
        vol2 = defaults.vol2
        vol3 = defaults.vol3
        pan0 = defaults.pan0
        pan1 = defaults.pan1
        pan2 = defaults.pan2
        pan3 = defaults.pan3
        
        volL = defaults.volL
        volR = defaults.volR
        samplingMethod = defaults.samplingMethod.rawValue
        filterType = defaults.filterType.rawValue
        filterAlwaysOn = defaults.filterAlwaysOn
        
        amiga.resume()
    }
    
    func loadAudioUserDefaults() {
        
        let defaults = UserDefaults.standard
        
        amiga.suspend()
        
        vol0 = defaults.integer(forKey: Keys.Aud.vol0)
        vol1 = defaults.integer(forKey: Keys.Aud.vol1)
        vol2 = defaults.integer(forKey: Keys.Aud.vol2)
        vol3 = defaults.integer(forKey: Keys.Aud.vol3)
        pan0 = defaults.integer(forKey: Keys.Aud.pan0)
        pan1 = defaults.integer(forKey: Keys.Aud.pan1)
        pan2 = defaults.integer(forKey: Keys.Aud.pan2)
        pan3 = defaults.integer(forKey: Keys.Aud.pan3)
        
        volL = defaults.integer(forKey: Keys.Aud.volL)
        volR = defaults.integer(forKey: Keys.Aud.volR)
        samplingMethod = defaults.integer(forKey: Keys.Aud.samplingMethod)
        filterType = defaults.integer(forKey: Keys.Aud.filterType)
        filterAlwaysOn = defaults.bool(forKey: Keys.Aud.filterAlwaysOn)
        
        amiga.resume()
    }
    
    func saveAudioUserDefaults() {
        
        track()
        
        let defaults = UserDefaults.standard
        
        defaults.set(vol0, forKey: Keys.Aud.vol0)
        defaults.set(vol1, forKey: Keys.Aud.vol1)
        defaults.set(vol2, forKey: Keys.Aud.vol2)
        defaults.set(vol3, forKey: Keys.Aud.vol3)
        defaults.set(pan0, forKey: Keys.Aud.pan0)
        defaults.set(pan1, forKey: Keys.Aud.pan1)
        defaults.set(pan2, forKey: Keys.Aud.pan2)
        defaults.set(pan3, forKey: Keys.Aud.pan3)
        
        defaults.set(volL, forKey: Keys.Aud.volL)
        defaults.set(volR, forKey: Keys.Aud.volR)
        defaults.set(samplingMethod, forKey: Keys.Aud.samplingMethod)
        defaults.set(filterType, forKey: Keys.Aud.filterType)
        defaults.set(filterAlwaysOn, forKey: Keys.Aud.filterAlwaysOn)
    }
    
    //
    // Video
    //

    func loadColorDefaults(_ defaults: VideoDefaults) {
        
        amiga.suspend()
        
        palette = defaults.palette
        brightness = defaults.brightness
        contrast = defaults.contrast
        saturation = defaults.saturation
        
        amiga.resume()
    }
    
    func loadGeometryDefaults(_ defaults: VideoDefaults) {
        
        hCenter = defaults.hCenter
        vCenter = defaults.vCenter
        hZoom = defaults.hZoom
        vZoom = defaults.vZoom
        
        renderer.updateTextureRect()
    }
    
    func loadShaderDefaults(_ defaults: VideoDefaults) {
        
        enhancer = defaults.enhancer
        upscaler = defaults.upscaler
        
        blur = defaults.blur
        blurRadius = defaults.blurRadius
        
        bloom = defaults.bloom
        bloomRadius = defaults.bloomRadius
        bloomBrightness = defaults.bloomBrightness
        bloomWeight = defaults.bloomWeight
        flicker = defaults.flicker
        flickerWeight = defaults.flickerWeight
        dotMask = defaults.dotMask
        dotMaskBrightness = defaults.dotMaskBrightness
        scanlines = defaults.scanlines
        scanlineBrightness = defaults.scanlineBrightness
        scanlineWeight = defaults.scanlineWeight
        disalignment = defaults.disalignment
        disalignmentH = defaults.disalignmentH
        disalignment = defaults.disalignment
        
        renderer.buildDotMasks()
    }
    
    func loadVideoDefaults(_ defaults: VideoDefaults) {
        
        loadColorDefaults(defaults)
        loadGeometryDefaults(defaults)
        loadColorDefaults(defaults)
    }

    func loadVideoUserDefaults() {
        
        let defaults = UserDefaults.standard
        
        amiga.suspend()
        
        let paletteKey = defaults.integer(forKey: Keys.Vid.palette)
        palette = Palette(rawValue: paletteKey) ?? .PALETTE_COLOR
        brightness = defaults.double(forKey: Keys.Vid.brightness)
        contrast = defaults.double(forKey: Keys.Vid.contrast)
        saturation = defaults.double(forKey: Keys.Vid.saturation)

        hCenter = defaults.float(forKey: Keys.Vid.hCenter)
        vCenter = defaults.float(forKey: Keys.Vid.vCenter)
        hZoom = defaults.float(forKey: Keys.Vid.hZoom)
        vZoom = defaults.float(forKey: Keys.Vid.vZoom)

        enhancer = defaults.integer(forKey: Keys.Vid.enhancer)
        upscaler = defaults.integer(forKey: Keys.Vid.upscaler)
        
        bloom = Int32(defaults.integer(forKey: Keys.Vid.bloom))
        bloomRadius = defaults.float(forKey: Keys.Vid.bloomRadius)
        bloomBrightness = defaults.float(forKey: Keys.Vid.bloomBrightness)
        bloomWeight = defaults.float(forKey: Keys.Vid.bloomWeight)
        flicker = Int32(defaults.integer(forKey: Keys.Vid.flicker))
        flickerWeight = defaults.float(forKey: Keys.Vid.flickerWeight)
        dotMask = Int32(defaults.integer(forKey: Keys.Vid.dotMask))
        dotMaskBrightness = defaults.float(forKey: Keys.Vid.dotMaskBrightness)
        scanlines = Int32(defaults.integer(forKey: Keys.Vid.scanlines))
        scanlineBrightness = defaults.float(forKey: Keys.Vid.scanlineBrightness)
        scanlineWeight = defaults.float(forKey: Keys.Vid.scanlineWeight)
        disalignment = Int32(defaults.integer(forKey: Keys.Vid.disalignment))
        disalignmentH = defaults.float(forKey: Keys.Vid.disalignmentH)
        disalignmentV = defaults.float(forKey: Keys.Vid.disalignmentV)
        
        renderer.updateTextureRect()
        renderer.buildDotMasks()
        
        amiga.resume()
    }
    
    func saveVideoUserDefaults() {
        
        track()
        
        let defaults = UserDefaults.standard
        
        defaults.set(palette.rawValue, forKey: Keys.Vid.palette)
        defaults.set(brightness, forKey: Keys.Vid.brightness)
        defaults.set(contrast, forKey: Keys.Vid.contrast)
        defaults.set(saturation, forKey: Keys.Vid.saturation)

        defaults.set(hCenter, forKey: Keys.Vid.hCenter)
        defaults.set(vCenter, forKey: Keys.Vid.vCenter)
        defaults.set(hZoom, forKey: Keys.Vid.hZoom)
        defaults.set(vZoom, forKey: Keys.Vid.vZoom)

        defaults.set(enhancer, forKey: Keys.Vid.enhancer)
        defaults.set(upscaler, forKey: Keys.Vid.upscaler)
        
        defaults.set(bloom, forKey: Keys.Vid.bloom)
        defaults.set(bloomRadius, forKey: Keys.Vid.bloomRadius)
        defaults.set(bloomBrightness, forKey: Keys.Vid.bloomBrightness)
        defaults.set(bloomWeight, forKey: Keys.Vid.bloomWeight)
        defaults.set(flicker, forKey: Keys.Vid.flicker)
        defaults.set(flickerWeight, forKey: Keys.Vid.flickerWeight)
        defaults.set(dotMask, forKey: Keys.Vid.dotMask)
        defaults.set(dotMaskBrightness, forKey: Keys.Vid.dotMaskBrightness)
        defaults.set(scanlines, forKey: Keys.Vid.scanlines)
        defaults.set(scanlineBrightness, forKey: Keys.Vid.scanlineBrightness)
        defaults.set(scanlineWeight, forKey: Keys.Vid.scanlineWeight)
        defaults.set(disalignment, forKey: Keys.Vid.disalignment)
        defaults.set(disalignmentH, forKey: Keys.Vid.disalignmentH)
        defaults.set(disalignmentV, forKey: Keys.Vid.disalignmentV)
    }
}
