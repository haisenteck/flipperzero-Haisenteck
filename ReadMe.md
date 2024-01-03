</a>
<h3>Haisenteck - Flipper zero firmware Mod</h3><br>
Link all'installazione dell'ultima release tramite Web Updater:<a href='https://lab.flipper.net/?url=https://rawcdn.githack.com/haisenteck/flipperzero-Haisenteck/a74a03e7ea54fe2f4b9c593438610eaf2c18be53/dist/release/flipper-z-f7-update-Haisenteck_V1.3.1.tgz&channel=Haisenteck_V1.3.2&version=V1.3.2' target='_blank'>WEB UPDATER - RELEASE</a><br>
link al file di installazione stabile: <a href='https://github.com/haisenteck/flipperzero-Haisenteck/releases/tag/V1.3.2' target='_blank'>Versione STABILE</a><br>
Link al file di installazione DEV build: <a href='https://github.com/haisenteck/flipperzero-Haisenteck/blob/dev/dist/f7-C/flipper-z-f7-update-Haisenteck_V1.3.2.tgz' target='_blank'>Versione DEV Build</a><br>
Link al repository dei contenuti extra: <a href='https://github.com/haisenteck/Flipper_MicroSD' target='_blank'>Contenuti Extra per Micro SD</a><br>
<br>
# Novit�<br>
- **FreeRTOS**<br>
<ul>
	<li>Upgrade da versione v10.6.0 a v10.6.1</li>
	<li>Upgrade da versione V10.5.1 a V10.6.0</li>
</ul>- **BLUETOOTH**<br>
<ul>
	<li> Ble Spam FAP attiva</li>
	<li> Ble Spam adattato da Xtreme Firmware</li>
	<del><li>modifica a libreria di base per inclusione di diverse modalità del bluetooth notification per presentare il flipper come dispositivo Apple.</li></del>
	<del><li>creato app per la gestione, si chiama FRUTTIVENDOLO.<br></del> App rimossa in quanto non più necessaria.</li>
	<li>09/12/2023 - provato e testato ble stack full, attualmente ancora in test, mentre su versione pubblicata in uso la light.</li>
	<li>27/12/2023 - integrato Ble Full Stack comprensivo di installazione pulita senza errori.</li>
</ul>- **NFC**<br>
<ul>	
	<li> NFC REFACTOR
	<del><li>aggiunto riconoscimento corretto per le carte<ul></del>
 		<del><li>MIFARE Classic 4k - emulated (6131 NFC)</li></del>
		<del><li>MIFARE Classic 4k - emulated (6131 NFC)</li></ul></li></del>
	<li>ripristinato i dizionari mifare e non creati con Stepzor</li>
	<li>aggiornato dizionario NFC da Stepzor con ultima data: 2023/11/15 </li>
 	<li>aggiunto codici riconoscimento carte EMV</li>
	<li>aggiunta lettura credito chiavette Coges grazie al contributo di un utente Telegram.</li>
	<li>aggiunto parser di carte russe dei trasporti e altre modifiche scritte da unleashed.</li>
	<li>aggiunti parser Mizip (by @zProAle), Microel (by @zProAle), kazan (by @noproto), saflok (by @noproto)</li>
	<li>aggiunti parser Hi (by @zProAle), PoC Skylander (by @bettse),  Hid (by @bettse), SoniCare (by @Sil333033)</li>
	<li>aggiunto parser credit (by @Esonhugh)</li>
</ul>- **Sub-GHz**<br>
<ul>
	<li>Aggiunto AM_Q ai preset di utilizzo</li>
	<li>Aggiunto TPMS ai preset di utilizzo</li>
	<li>Aggiunto HONDA1 ai preset di utilizzo</li>
	<li>Aggiunto HONDA2 ai preset di utilizzo</li>
	<li>predisposizione per ulteriori wheather station e comandi dedicati. </li>
	<li>SubGhz Rolling Flaws - app aggiunta grazie a https://github.com/jamisonderek/flipper-zero-tutorials </li>
	<li>Subghz Shapshup - app aggiunta e corretto alcune parti di codice</li>
	<li>aggiunto ulteriori protocolli ed effettuato un altro porting di protocollo weather station (2023/11/27)</li>
	<li>correzione di protocolli subghz durante test approfonditi</li>
	<li>update aggiornamento protocollo oregon2 da fap weather station 2023/12/09</li>
	<li>Modifica driver CC1101 da Driver esterni a parser della fap principale Subghz velocizzando il tutto e riducendo alcune ridondanze di caricamento.</li>
	<li>sistemazione protocollo Oregon2 con errori e crash del flipper in fase di ricezione</li>
	<li>03/01/2024 - aggiunto gestione gps nel subghz (idea riadattata da progetto xtreme), abilitato salvataggio in file separato delle coordinate gps</li><table>
<tr><th>Tag</th><th>Type</th><th>Flag</th></tr>
<tr><td>Acurite_592TXR</td><td>WeatherStation</td><td>433 Enable, 315 Enable, 868 Enable, AM, Save Enable</td></tr>
<tr><td>Acurite_606TX</td><td>WeatherStation</td><td>433 Enable, 315 Enable, 868 Enable, AM, Save Enable, Send Enable</td></tr>
<tr><td>Acurite_609TXC</td><td>WeatherStation</td><td>433 Enable, 315 Enable, 868 Enable, AM, Save Enable, Send Enable</td></tr>
<tr><td>Acurite_986</td><td>WeatherStation</td><td>433 Enable, 315 Enable, 868 Enable, AM, Save Enable</td></tr>
<tr><td>Alutech_at_4n</td><td>Dinamico\Rolling Code</td><td>433 Enable, AM, Save Enable, Send Enable</td></tr>
<tr><td>ambient_weather</td><td>WeatherStation</td><td>433 Enable, 315 Enable, 868 Enable, AM, Save Enable, Send Enable</td></tr>
<tr><td>Ansonic</td><td>Statico</td><td>433 Enable, 315 Enable, FM, Save Enable, Send Enable</td></tr>
<tr><td>auriol_ahfl</td><td>WeatherStation</td><td>433 Enable, 315 Enable, 868 Enable, AM, Save Enable, Send Enable</td></tr>
<tr><td>Auriol_TH</td><td>WeatherStation</td><td>433 Enable, 315 Enable, 868 Enable, AM, Save Enable, Send Enable</td></tr>
<tr><td>Bett</td><td>Statico</td><td>433 Enable, AM, Save Enable, Send Enable</td></tr>
<tr><td>BinRaw</td><td>SubGhzProtocolTypeBinRAW,</td><td>433 Enable, 315 Enable, 868 Enable, AM, FM, Save Enable, Send Enable</td></tr>
<tr><td>Came</td><td>Statico</td><td>433 Enable, 315 Enable, AM, Save Enable, Send Enable</td></tr>
<tr><td>CameAtomo</td><td>Dinamico\Rolling Code</td><td>433 Enable, AM, Save Enable, Send Enable</td></tr>
<tr><td>CameTwee</td><td>Statico</td><td>433 Enable, AM, Save Enable, Send Enable</td></tr>
<tr><td>ChambCode</td><td>Statico</td><td>315 Enable, AM, Save Enable, Send Enable</td></tr>
<tr><td>Clemsa</td><td>Statico</td><td>433 Enable, AM, Save Enable, Send Enable</td></tr>
<tr><td>Doitrand</td><td>Statico</td><td>433 Enable, AM, Save Enable, Send Enable</td></tr>
<tr><td>Dooya</td><td>Statico</td><td>433 Enable, 315 Enable, AM, Save Enable, Send Enable</td></tr>
<tr><td>FaacSLH</td><td>Dinamico\Rolling Code</td><td>433 Enable, 868 Enable, AM, Save Enable, Send Enable</td></tr>
<tr><td>GateTx</td><td>Statico</td><td>433 Enable, AM, Save Enable, Send Enable</td></tr>
<tr><td>Genie</td><td>Dinamico\Rolling Code</td><td>433 Enable, 315 Enable, 868 Enable, AM, Save Enable, Send Enable</td></tr>
<tr><td>GT_WT02</td><td>WeatherStation</td><td>433 Enable, 315 Enable, 868 Enable, AM, Save Enable</td></tr>
<tr><td>GT_WT03</td><td>WeatherStation</td><td>433 Enable, 315 Enable, 868 Enable, AM, Save Enable</td></tr>
<tr><td>Holtek</td><td>Statico</td><td>433 Enable, 315 Enable, 868 Enable, AM, Save Enable, Send Enable</td></tr>
<tr><td>HoltekHt12x</td><td>Statico</td><td>433 Enable, 315 Enable, 868 Enable, AM, FM, Save Enable, Send Enable</td></tr>
<tr><td>Honeywell</td><td>Statico</td><td>433 Enable, 315 Enable, 868 Enable, AM, Save Enable, Send Enable</td></tr>
<tr><td>HoneywellWdb</td><td>Statico</td><td>433 Enable, 315 Enable, AM, Save Enable, Send Enable</td></tr>
<tr><td>HormannHsm</td><td>Statico</td><td>433 Enable, 868 Enable, AM, Save Enable, Send Enable</td></tr>
<tr><td>Ido117/111</td><td>Dinamico\Rolling Code</td><td>433 Enable, AM, Save Enable</td></tr>
<tr><td>Infactory</td><td>WeatherStation</td><td>433 Enable, 315 Enable, 868 Enable, AM, Save Enable</td></tr>
<tr><td>IntertechnoV3</td><td>Statico</td><td>433 Enable, 315 Enable, 868 Enable, AM, Save Enable, Send Enable</td></tr>
<tr><td>Keeloq</td><td>Dinamico\Rolling Code</td><td>433 Enable, 315 Enable, 868 Enable</td></tr>
<tr><td>Kia</td><td>Dinamico\Rolling Code</td><td>433 Enable, FM, Save Enable</td></tr>
<tr><td>KingGatesStylo4k</td><td>Dinamico\Rolling Code</td><td>433 Enable, AM, Save Enable, Send Enable</td></tr>
<tr><td>LaCrosse_TX</td><td>WeatherStation</td><td>433 Enable, 315 Enable, 868 Enable, AM, Save Enable, Send Enable</td></tr>
<tr><td>LaCrosse_TX141THBv2</td><td>WeatherStation</td><td>433 Enable, 315 Enable, 868 Enable, AM, Save Enable, Send Enable</td></tr>
<tr><td>Linear</td><td>Statico</td><td>315 Enable, AM, Save Enable, Send Enable</td></tr>
<tr><td>LinearDelta3</td><td>Statico</td><td>315 Enable, AM, Save Enable, Send Enable</td></tr>
<tr><td>Magellan</td><td>Statico</td><td>433 Enable, AM, Save Enable, Send Enable</td></tr>
<tr><td>Marantec</td><td>Statico</td><td>433 Enable, AM, Save Enable, Send Enable</td></tr>
<tr><td>Mastercode</td><td>Statico</td><td>433 Enable, AM, Save Enable, Send Enable</td></tr>
<tr><td>MegaCode</td><td>Statico</td><td>315 Enable, AM, Save Enable, Send Enable</td></tr>
<tr><td>NeroRadio</td><td>Statico</td><td>433 Enable, AM, Save Enable, Send Enable</td></tr>
<tr><td>NeroSketch</td><td>Statico</td><td>433 Enable, AM, Save Enable, Send Enable</td></tr>
<tr><td>Nexus_TH</td><td>WeatherStation</td><td>433 Enable, 315 Enable, 868 Enable, AM, Save Enable, Send Enable</td></tr>
<tr><td>NiceFlo</td><td>Statico</td><td>433 Enable, 315 Enable, AM, Save Enable, Send Enable</td></tr>
<tr><td>NiceFlorS</td><td>Dinamico\Rolling Code</td><td>433 Enable, 868 Enable, AM, Save Enable, Send Enable</td></tr>
<tr><td>Oregon2</td><td>WeatherStation</td><td>433 Enable, AM, Save Enable</td></tr>
<tr><td>Oregon3</td><td>WeatherStation</td><td>433 Enable, AM, Save Enable, Send Enable</td></tr>
<tr><td>Oregon_V1</td><td>WeatherStation</td><td>433 Enable, 315 Enable, 868 Enable, AM, Save Enable, Send Enable</td></tr>
<tr><td>PhoenixV2</td><td>Statico</td><td>433 Enable, AM, Save Enable, Send Enable</td></tr>
<tr><td>POCSAG</td><td>Statico</td><td>FM, Save Enable, Send Enable</td></tr>
<tr><td>PowerSmart</td><td>Statico</td><td>433 Enable, AM, Save Enable, Send Enable</td></tr>
<tr><td>Princeton</td><td>Statico</td><td>433 Enable, 315 Enable, 868 Enable, AM, Save Enable, Send Enable</td></tr>
<tr><td>Raw</td><td>SubGhzProtocolTypeRAW,</td><td>433 Enable, 315 Enable, 868 Enable, AM, FM, Save Enable, Send Enable</td></tr>
<tr><td>ScherKhan</td><td>Dinamico\Rolling Code</td><td>433 Enable, FM, Save Enable</td></tr>
<tr><td>Schrader</td><td>Statico</td><td>433 Enable, 315 Enable, AM, Save Enable, Send Enable</td></tr>
<tr><td>SecPlusV1</td><td>Dinamico\Rolling Code</td><td>315 Enable, AM, Save Enable, Send Enable</td></tr>
<tr><td>SecPlusV2</td><td>Dinamico\Rolling Code</td><td>315 Enable, AM, Save Enable, Send Enable</td></tr>
<tr><td>Smc5326</td><td>Statico</td><td>433 Enable, 315 Enable, 868 Enable, AM, Save Enable, Send Enable</td></tr>
<tr><td>SomfyKeytis</td><td>Dinamico\Rolling Code</td><td>433 Enable, 868 Enable, AM, Save Enable, Send Enable</td></tr>
<tr><td>SomfyTelis</td><td>Dinamico\Rolling Code</td><td>433 Enable, 868 Enable, AM, Save Enable, Send Enable</td></tr>
<tr><td>StarLine</td><td>Dinamico\Rolling Code</td><td>433 Enable, AM, Save Enable, Send Enable</td></tr>
<tr><td>ThermoPRO_TX4</td><td>WeatherStation</td><td>433 Enable, 315 Enable, 868 Enable, AM, Save Enable, Send Enable</td></tr>
<tr><td>TX_8300</td><td>WeatherStation</td><td>433 Enable, 315 Enable, 868 Enable, AM, Save Enable, Send Enable</td></tr>
<tr><td>WendoxW6726</td><td>WeatherStation</td><td>433 Enable, 315 Enable, 868 Enable, AM, Save Enable, Send Enable</td></tr>
<tr><td>X10</td><td>Dinamico\Rolling Code</td><td>315 Enable, AM, Save Enable, Send Enable</td></tr>
</table>
- **VARIE**<br>
<ul>
	<li>rimosso svariate problematiche relative a submoduli di github bloccati.</li>
	<li>variazione di svariati parametri per ridurre i tempi morti tra i comandi e la frequenza di base del flipper (a discapito di un consumo leggermente superiore di batteria</li>
	<li>diverse sistemazioni nel sistema, pulizia generale</li>
	<li>rimozione di tutte le animazioni e lasciato animazione custom</li>
	<li>Aggiornato STM32CubeWB da 1.17.3 a 1.18.0</li>
	<li>Aggiornato STM32WBXX_HALL da V1.13.0 a V1.14.1</li>
	<li>riportato in compatibilità con store fap da app smartphone il firmware</li>
	<li>27/12/2023 - RIMOSSO IButton in quanto reputato inutile occupazione di spazio per funzioni in Italia mai usate</li>
	<li>27/12/2023 - alleggerimento del firmware</li>
	<li>27/12/2023 - modifica al submodulo cmsis per adattamento al full ble stack.</li>
	<li>27/12/2023 - rimosso fap Ble Spam, inutile fap andata di moda ora inutile e noiosa (come dall'inizio).</li>
	<li>27/12/2023 - aggiornato diverse fap e aggiunto fap "connect wire" e fap provenienti da Malwekem e altre.</li>
	<li>03/01/2024 - risolto bug relativo a percentuale batteria mostrato su desktop</li>
</ul>- **FAP**<br><ul>
Missing data in folder: D:\xxx_flipper_code\firmware\flipperzero-Haisenteck\applications_user\fz_dope
<br>GPIO:<br>
    <li> Name: [BMI160] Air Mouse</li>
    <li> FAP Version: 0.8</li>
--------------------------------------------------
    <li> Name: driver</li>
    <li> FAP Version: 1.0</li>
--------------------------------------------------
    <li> Name: BarCode ScannerE</li>
    <li> FAP Version: (0, 5)</li>
--------------------------------------------------
    <li> Name: [ESP32] Camera Suite</li>
    <li> FAP Version: 1.3</li>
--------------------------------------------------
    <li> Name: free-dap</li>
    <li> FAP Version: 1.1</li>
--------------------------------------------------
    <li> Name: [MAYHEM] Camera v1.0.0</li>
--------------------------------------------------
    <li> Name: [MAYHEM] Morse Flasher v0.1</li>
--------------------------------------------------
    <li> Name: [MAYHEM] Motion detection v0.1</li>
--------------------------------------------------
    <li> Name: [MAYHEM] Nanny Cam v0.1</li>
--------------------------------------------------
    <li> Name: [MAYHEM] QR Code v0.1</li>
--------------------------------------------------
    <li> Name: [ESP32] Gravity</li>
    <li> FAP Version: 0.2.1</li>
--------------------------------------------------
    <li> Name: [LED] Flashlight</li>
    <li> FAP Version: 1.0</li>
--------------------------------------------------
    <li> Name: Flipagotchi</li>
--------------------------------------------------
    <li> Name: Flip Pirate</li>
--------------------------------------------------
    <li> Name: [RCWL0516] Radar Scan</li>
    <li> FAP Version: 2.0</li>
--------------------------------------------------
    <li> Name: Scope</li>
    <li> FAP Version: 0.2</li>
--------------------------------------------------
    <li> Name: [ESP32] Camera</li>
--------------------------------------------------
    <li> Name: esp-serial-flasher</li>
    <li> FAP Version: (1,1)</li>
--------------------------------------------------
    <li> Name: [GPIO] Input Reader 2</li>
--------------------------------------------------
    <li> Name: Atomic Dice Roller</li>
--------------------------------------------------
    <li> Name: [ESP32] Evil Portal</li>
--------------------------------------------------
    <li> Name: [GPIO] i2c Tools</li>
    <li> FAP Version: 1.1</li>
--------------------------------------------------
    <li> Name: Radiation Sensor</li>
    <li> FAP Version: 1.0</li>
--------------------------------------------------
    <li> Name: RGB LED</li>
    <li> FAP Version: 1.0</li>
--------------------------------------------------
    <li> Name: Pokemon Trading</li>
    <li> FAP Version: [1, 2]</li>
--------------------------------------------------
    <li> Name: [J305] Geiger Counter</li>
    <li> FAP Version: 1.0</li>
--------------------------------------------------
    <li> Name: [GPIO] Reader (aureli1c)</li>
    <li> FAP Version: 1.0</li>
--------------------------------------------------
    <li> Name: 7-Segment Output</li>
    <li> FAP Version: 1.1</li>
--------------------------------------------------
    <li> Name: GPIO Controller</li>
--------------------------------------------------
    <li> Name: [NMEA] GPS</li>
    <li> FAP Version: 1.2</li>
--------------------------------------------------
    <li> Name: [HC-SR] Dist. Sensor</li>
    <li> FAP Version: 1.0</li>
--------------------------------------------------
    <li> Name: [ESP] IFTTT Button</li>
--------------------------------------------------
    <li> Name: BH1750</li>
    <li> FAP Version: 1.0</li>
--------------------------------------------------
    <li> Name: Logic Analyzer</li>
    <li> FAP Version: 1.0</li>
--------------------------------------------------
    <li> Name: LoRA_Terminal</li>
--------------------------------------------------
    <li> Name: [MALVEKE] GAME BOY ADVANCE Cartridge</li>
    <li> FAP Version: [1,0]</li>
--------------------------------------------------
    <li> Name: [MALVEKE] GAME BOY Cartridge (GB/GBC)</li>
    <li> FAP Version: [1,0]</li>
--------------------------------------------------
    <li> Name: [MALVEKE] GAME BOY Link-Camera</li>
    <li> FAP Version: [1,0]</li>
--------------------------------------------------
    <li> Name: [MALVEKE] GB Live Camera</li>
    <li> FAP Version: [1,0]</li>
--------------------------------------------------
    <li> Name: [MALVEKE] PIN Test</li>
--------------------------------------------------
    <li> Name: [MALVEKE] Pokemon Trading</li>
    <li> FAP Version: [1,4]</li>
--------------------------------------------------
    <li> Name: [Mx2125] Accelerometer</li>
--------------------------------------------------
    <li> Name: [MH-Z19] CO2 sensor</li>
--------------------------------------------------
    <li> Name: nrf24</li>
    <li> FAP Version: 1.0</li>
--------------------------------------------------
    <li> Name: nrf24</li>
--------------------------------------------------
    <li> Name: nrf24</li>
--------------------------------------------------
    <li> Name: nrf24</li>
    <li> FAP Version: 1.0</li>
--------------------------------------------------
    <li> Name: RC2014 ColecoVision</li>
--------------------------------------------------
    <li> Name: [RCWL-0516] Motion Sensor</li>
--------------------------------------------------
    <li> Name: [GPIO] Sentry Safe</li>
    <li> FAP Version: 1.0</li>
--------------------------------------------------
    <li> Name: Servo Tester 2</li>
    <li> FAP Version: 1.0</li>
--------------------------------------------------
    <li> Name: Signal Generator</li>
    <li> FAP Version: 1.0</li>
--------------------------------------------------
    <li> Name: spi</li>
    <li> FAP Version: 1.2</li>
--------------------------------------------------
    <li> Name: SWD Probe</li>
    <li> FAP Version: 1.0</li>
--------------------------------------------------
    <li> Name: TAS playback",  # Displayed in menus</li>
    <li> FAP Version: (0, 1),  # (major, minor)</li>
--------------------------------------------------
    <li> Name: [GPIO] Timelapse</li>
    <li> FAP Version: 1.1</li>
--------------------------------------------------
    <li> Name: UART Terminal</li>
    <li> FAP Version: 1.1</li>
--------------------------------------------------
    <li> Name: u-blox GPS</li>
    <li> FAP Version: (0, 2), # major, minor</li>
--------------------------------------------------
    <li> Name: Temp sensors reader</li>
    <li> FAP Version: 1.4</li>
--------------------------------------------------
    <li> Name: [USPING] Dist. Sensor</li>
    <li> FAP Version: 1.0</li>
--------------------------------------------------
    <li> Name: Wiegand Reader</li>
    <li> FAP Version: 1.0</li>
--------------------------------------------------
    <li> Name: WIFI Mapping</li>
--------------------------------------------------
    <li> Name: [ESP32] WiFi Marauder</li>
    <li> FAP Version: (6,4)</li>
--------------------------------------------------
    <li> Name: [WiFi] Scanner</li>
    <li> FAP Version: 1.1</li>
--------------------------------------------------
    <li> Name: [WII] EC Analyser</li>
    <li> FAP Version: 1.0</li>
--------------------------------------------------
    <li> Name: [GPIO] Wire Tester</li>
    <li> FAP Version: 1.0</li>
--------------------------------------------------
<br>Games:<br>
    <li> Name: Arkanoid</li>
    <li> FAP Version: 1.0</li>
--------------------------------------------------
    <li> Name: Asteroids</li>
    <li> FAP Version: 1.0</li>
--------------------------------------------------
    <li> Name: Blackjack</li>
    <li> FAP Version: 1.0</li>
--------------------------------------------------
    <li> Name: Bomberduck</li>
    <li> FAP Version: 1.0</li>
--------------------------------------------------
    <li> Name: Chess</li>
    <li> FAP Version: (1, 9)</li>
--------------------------------------------------
    <li> Name: Chess Clock</li>
    <li> FAP Version: 1.0</li>
--------------------------------------------------
    <li> Name: Connect Wires</li>
    <li> FAP Version: 1.0</li>
--------------------------------------------------
    <li> Name: DOOM</li>
    <li> FAP Version: 1.0</li>
--------------------------------------------------
    <li> Name: DVD Bouncer</li>
--------------------------------------------------
    <li> Name: Etch A Sketch</li>
    <li> FAP Version: 1.0</li>
--------------------------------------------------
    <li> Name: Flappy Bird</li>
    <li> FAP Version: 1.0</li>
--------------------------------------------------
    <li> Name: Hangman</li>
    <li> FAP Version: (1, 4)</li>
--------------------------------------------------
    <li> Name: Game 15</li>
    <li> FAP Version: 1.0</li>
--------------------------------------------------
    <li> Name: 2048</li>
    <li> FAP Version: 1.1</li>
--------------------------------------------------
    <li> Name: Game of Life</li>
    <li> FAP Version: 1.0</li>
--------------------------------------------------
    <li> Name: Heap Defence</li>
    <li> FAP Version: 1.0</li>
--------------------------------------------------
    <li> Name: Jetpack Joyride</li>
--------------------------------------------------
    <li> Name: Minesweeper</li>
    <li> FAP Version: 1.0</li>
--------------------------------------------------
    <li> Name: Monty Hall</li>
    <li> FAP Version: 1.1</li>
--------------------------------------------------
    <li> Name: Multi-Dice</li>
--------------------------------------------------
    <li> Name: Nu pogodi</li>
    <li> FAP Version: 1.1</li>
--------------------------------------------------
    <li> Name: Pong</li>
    <li> FAP Version: 1.1</li>
--------------------------------------------------
    <li> Name: Race</li>
    <li> FAP Version: 1.1</li>
--------------------------------------------------
    <li> Name: Reversi</li>
    <li> FAP Version: 1.1</li>
--------------------------------------------------
    <li> Name: Roots of Life</li>
    <li> FAP Version: 1.0</li>
--------------------------------------------------
    <li> Name: Rubik's Cube Scrambler</li>
    <li> FAP Version: 1.1</li>
--------------------------------------------------
    <li> Name: SUPER-CHIP</li>
    <li> FAP Version: 1.0</li>
--------------------------------------------------
    <li> Name: Scorched Tanks</li>
    <li> FAP Version: 1.1</li>
--------------------------------------------------
    <li> Name: Secret Toggle",  # Displayed in menus</li>
--------------------------------------------------
    <li> Name: Simon Says",  # Displayed in UI</li>
--------------------------------------------------
    <li> Name: Slot Machine</li>
    <li> FAP Version: 1.1</li>
--------------------------------------------------
    <li> Name: Snake 2.0</li>
    <li> FAP Version: 2.0</li>
--------------------------------------------------
    <li> Name: Snake Game</li>
--------------------------------------------------
    <li> Name: Solitaire</li>
    <li> FAP Version: 1.0</li>
--------------------------------------------------
    <li> Name: Sudoku</li>
    <li> FAP Version: 0.1</li>
--------------------------------------------------
    <li> Name: TAMA P1</li>
--------------------------------------------------
    <li> Name: Tanks</li>
--------------------------------------------------
    <li> Name: Tetris</li>
    <li> FAP Version: 1.0</li>
--------------------------------------------------
    <li> Name: Tic Tac Toe</li>
    <li> FAP Version: 1.0</li>
--------------------------------------------------
    <li> Name: T-Rex runner</li>
    <li> FAP Version: 1.2</li>
--------------------------------------------------
    <li> Name: Video Poker</li>
    <li> FAP Version: 1.0</li>
--------------------------------------------------
    <li> Name: Yatzee</li>
    <li> FAP Version: 1.0</li>
--------------------------------------------------
    <li> Name: ZERO!</li>
    <li> FAP Version: 1.3</li>
--------------------------------------------------
    <li> Name: Zombiez</li>
    <li> FAP Version: 1.0</li>
--------------------------------------------------
<br>Bluetooth:<br>
    <li> Name: Bad BT</li>
--------------------------------------------------
    <li> Name: BT Serial App</li>
--------------------------------------------------
    <li> Name: TikTok Remote</li>
    <li> FAP Version: 1.1</li>
--------------------------------------------------
    <li> Name: Bluetooth Remote</li>
--------------------------------------------------
<br>Tools:<br>
    <li> Name: Barcode App</li>
    <li> FAP Version: 1.1</li>
--------------------------------------------------
    <li> Name: Brainfuck</li>
    <li> FAP Version: 1.0</li>
--------------------------------------------------
    <li> Name: Caesar Cipher</li>
    <li> FAP Version: 1.0</li>
--------------------------------------------------
    <li> Name: Calculator</li>
    <li> FAP Version: 1.0</li>
--------------------------------------------------
    <li> Name: Calendar</li>
--------------------------------------------------
    <li> Name: CLI-GUI Bridge</li>
--------------------------------------------------
    <li> Name: Count Down Timer</li>
    <li> FAP Version: 1.1</li>
--------------------------------------------------
    <li> Name: Counter</li>
    <li> FAP Version: 1.1</li>
--------------------------------------------------
    <li> Name: DTMF Dolphin</li>
    <li> FAP Version: 1.1</li>
--------------------------------------------------
    <li> Name: crypto</li>
    <li> FAP Version: (1, 13)</li>
--------------------------------------------------
    <li> Name: Nightstand Clock</li>
    <li> FAP Version: 1.0</li>
--------------------------------------------------
    <li> Name: QRCode</li>
    <li> FAP Version: (2,0)</li>
--------------------------------------------------
    <li> Name: Pomodoro Timer</li>
--------------------------------------------------
    <li> Name: Chronometer</li>
--------------------------------------------------
    <li> Name: HEX Viewer</li>
    <li> FAP Version: 1.1</li>
--------------------------------------------------
    <li> Name: MagSpoof WIP</li>
    <li> FAP Version: (0, 5),  # major, minor</li>
--------------------------------------------------
    <li> Name: Multi Converter</li>
    <li> FAP Version: 1.0</li>
--------------------------------------------------
    <li> Name: Multi Counter</li>
    <li> FAP Version: 1.0</li>
--------------------------------------------------
    <li> Name: RFID detector</li>
    <li> FAP Version: 1.2</li>
--------------------------------------------------
    <li> Name: Orgasmotron</li>
--------------------------------------------------
    <li> Name: Password Generator</li>
    <li> FAP Version: 1.1</li>
--------------------------------------------------
    <li> Name: Text Viewer</li>
    <li> FAP Version: 1.0</li>
--------------------------------------------------
    <li> Name: wolfssl</li>
    <li> FAP Version: 5.50</li>
--------------------------------------------------
    <li> Name: WHC SWIO Flasher</li>
    <li> FAP Version: 1.0</li>
--------------------------------------------------
<br>Media:<br>
    <li> Name: BPM Tapper</li>
    <li> FAP Version: 1.0</li>
--------------------------------------------------
    <li> Name: Flizzer Tracker</li>
    <li> FAP Version: (0, 2)</li>
--------------------------------------------------
    <li> Name: Mandelbrot Set</li>
    <li> FAP Version: 1.0</li>
--------------------------------------------------
    <li> Name: Metronome</li>
    <li> FAP Version: 1.0</li>
--------------------------------------------------
    <li> Name: Morse Code</li>
    <li> FAP Version: 1.0</li>
--------------------------------------------------
    <li> Name: Music Beeper</li>
--------------------------------------------------
    <li> Name: Music Player</li>
--------------------------------------------------
    <li> Name: Zero Tracker</li>
    <li> FAP Version: 1.1</li>
--------------------------------------------------
    <li> Name: Ocarina</li>
    <li> FAP Version: 1.0</li>
--------------------------------------------------
    <li> Name: Paint</li>
    <li> FAP Version: 1.0</li>
--------------------------------------------------
    <li> Name: SAM WTF</li>
--------------------------------------------------
    <li> Name: Text to SAM</li>
    <li> FAP Version: 1.1</li>
--------------------------------------------------
    <li> Name: Tuning Fork</li>
    <li> FAP Version: 1.0</li>
--------------------------------------------------
    <li> Name: USB Midi</li>
    <li> FAP Version: 1.0</li>
--------------------------------------------------
    <li> Name: Video Player</li>
    <li> FAP Version: (0, 1)</li>
--------------------------------------------------
    <li> Name: WAV Player</li>
    <li> FAP Version: 1.0</li>
--------------------------------------------------
<br>Coffee:<br>
    <li> Name: Coffee EEPROM</li>
--------------------------------------------------
    <li> Name: Coges</li>
    <li> FAP Version: 1.0</li>
--------------------------------------------------
<br>Misc:<br>
    <li> Name: Creatine</li>
--------------------------------------------------
    <li> Name: Airmon</li>
--------------------------------------------------
    <li> Name: Bad Apple</li>
--------------------------------------------------
    <li> Name: jClock settings</li>
--------------------------------------------------
    <li> Name: LED Pulsator</li>
--------------------------------------------------
<br>CAN:<br>
    <li> Name: CANUtils",  # Displayed in menus</li>
    <li> FAP Version: 0.1</li>
--------------------------------------------------
<br>Infrared:<br>
    <li> Name: Xbox Controller</li>
    <li> FAP Version: 1.0</li>
--------------------------------------------------
    <li> Name: XRemote</li>
    <li> FAP Version: 1.0</li>
--------------------------------------------------
    <li> Name: R.O.B. Control",  # Displayed in menus</li>
--------------------------------------------------
    <li> Name: IR Remote</li>
    <li> FAP Version: 1.0</li>
--------------------------------------------------
    <li> Name: IR Scope</li>
    <li> FAP Version: 1.2</li>
--------------------------------------------------
<br>Sub-GHz:<br>
    <li> Name: ShapShup</li>
--------------------------------------------------
    <li> Name: Sub-GHz Bruteforcer</li>
--------------------------------------------------
    <li> Name: Sub-GHz Playlist</li>
    <li> FAP Version: 1.0</li>
--------------------------------------------------
    <li> Name: POCSAG Pager</li>
    <li> FAP Version: 1.0</li>
--------------------------------------------------
    <li> Name: ProtoView</li>
    <li> FAP Version: 1.0</li>
--------------------------------------------------
    <li> Name: Subghz Rolling Flaws</li>
    <li> FAP Version: (1, 4)</li>
--------------------------------------------------
    <li> Name: Spectrum Analyzer</li>
    <li> FAP Version: 1.1</li>
--------------------------------------------------
    <li> Name: TPMS Reader</li>
--------------------------------------------------
    <li> Name: Weather Station</li>
    <li> FAP Version: 1.2</li>
--------------------------------------------------
<br>RFID:<br>
    <li> Name: RFID Beacon</li>
--------------------------------------------------
    <li> Name: [YRM100] UHF RFID</li>
--------------------------------------------------
<br>Power:<br>
    <li> Name: Charge Controller",  # Displayed in menus</li>
--------------------------------------------------
<br>USB:<br>
    <li> Name: Mass Storage</li>
    <li> FAP Version: 1.3</li>
--------------------------------------------------
    <li> Name: USB HID Autofire</li>
    <li> FAP Version: 1.0</li>
--------------------------------------------------
<br>NFC:<br>
    <li> Name: Mfkey32</li>
    <li> FAP Version: 1.1</li>
--------------------------------------------------
    <li> Name: magic</li>
    <li> FAP Version: 1.3</li>
--------------------------------------------------
    <li> Name: loclass</li>
    <li> FAP Version: 1.9</li>
--------------------------------------------------
    <li> Name: asn1</li>
    <li> FAP Version: 2.0</li>
--------------------------------------------------
<br>Electronics:<br>
    <li> Name: Resistance calculator",  # Displayed in menus</li>
    <li> FAP Version: (1, 1)</li>
--------------------------------------------------
</ul>
<br>
Ultimo Merge da firmware ufficiale (anche fatto a mano): 2023/11/15
<br><br>
<br>
#### **This firmware is a fork from** [FLIPPER ZERO OFFICIAL FIRMWARE](https://github.com/flipperdevices/flipperzero-firmware) <br>
and part take from: [DarkFlippers/unleashed-firmware](https://github.com/DarkFlippers/unleashed-firmware)<br>
app from: https://github.com/xMasterX/all-the-plugins <br>
and many other repository or source. <br>
<br>
<br>
Non sono responsabile per qualsiasi uso illegale anche tentato di questo strumento.<br>
<br>