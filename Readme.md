About

This EXE allows to connect flight navigation software supporting NMEA data over TCP/IP with Flight Simulator 2020.
This app has been tested & proven to work with XCSoar.

Usage:
- start the exe BEFORE starting a flight in flight Simulator. It connects best when FS2020 is in main menu.
  If FS2020 wasn't started yet when starting the EXE you have 10 minutes to start it so that it can reach the main menu and the EXE can connect.
  (Should be sufficient as long as FS2020 doesn't download updates)
- If starting for first time please allow all connection  within the popup of Windows firewall so that external navigation systems can connect.
- Connect your Navigation software using IP as shown and port 10110. Seetings in XC Soar would be NMEA Source --> TCP Client --> Driver generic or FLarm.
- start your flight in FS2020
- have fun :)


Known issues:
- Only one connection supported currently. If connection to navi software is lost EXE (&flight) must be restarted
- some Anti-Virus like Avast claim this EXE is a threat (IDP.Generic). As you can see on the web these issues are almost always false positive (same here). Please add exception to virus filter.
- no traffic information yet as FS2020 SDK doesn't support to read traffic