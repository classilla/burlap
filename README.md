# BURLAP (Brother Updated Remote Link Access Package)

[Another Old VCR Jam!](https://oldvcr.blogspot.com/2023/06/o-brother-geobook-lets-get-thou-back-on.html)

Copyright (C) 2023 Cameron Kaiser.  
All rights reserved.  
Distributed under the Floodgap Free Software License.

Brother is a registered trademark of Brother International Corporation (USA), Brother Industries, Ltd. (Japan), and its applicable subsidiaries in other countries. This project may involve modification of software distributed by Brother which is the current intellectual property of Microsoft Corporation. Neither Brother, its subsidiaries, nor Microsoft are officially involved with or endorse this project, and your use of it is AT YOUR OWN RISK.

Please read license terms before use.

## What it is, my Brother

BURLAP is a collection of utilities that allow dial-up machines -- like the Brother GeoBook family (PN-9000GR, NB-60 and NB-80C), but also many computers of that era in general -- to establish a network connection over a PPP serial link.

* `mosim.c` is a simple modem emulator written in C89 that replies to Hayes '302/TIES and AT commands with `OK`, and `ATD...` commands with `CONNECT` after which it terminates, leaving the line open for another tool like ...

* `usb2ppp.c` is notionally a tool written in C89 for tunnelling `pppd` over a USB serial dongle, but actually will run almost any program over any serial port.

These tools were written for the Brother GeoBooks, but should work for most systems that can communicate using PPP over a serial/RS-232 link. They only require POSIX and POSIX-style `termios.h`, and should compile with `gcc` or `clang`.

Additionally, BURLAP includes:

* `charno`, a Perl script that acts as an HTTP proxy, filtering the content type `text/html;charset=...` to just `text/html` for older browsers that don't grok what a charset is (like the Brother GeoBooks' GlobeHopper browser). It runs from `inetd` or a similar superserver.

Combined with [Crypto Ancienne](https://github.com/classilla/cryanc)'s `carl` in proxy mode, this can be enough for some configurable browsers to also access sites using TLS 1.3.

## How to set up the PPP link

These instructions should work for all systems, including the Brother GeoBook family.

* You must have root and `pppd` on the machine you will connect your client to (henceforth the PPP server). macOS, Mac OS X, Linux and the *BSDs usually provide it built-in at `/usr/sbin/pppd`.

* Configure and connect your PPP server's serial port, which can be a real on-board serial port or a USB dongle or similar (as long as there's a path to access it), to your PPP client. A null modem cable is typically necessary. Set your PPP client's serial port settings to the fastest available speed (on the Brother systems, the external COM port is limited to 38400bps; ensure you are using the external port under Modem settings in Preferences).

* Select your PPP server options. You will pass them on the command line. For example, mine look like this (replace `__.__.__.__` with the address to assign to the PPP client, the DNS server and the netmask, in order), all of which are defined in the `pppd` man page:

```
:__.__.__.__ local ms-dns __.__.__.__ netmask __.__.__.__ \
passive noauth proxyarp notty debug persist nodetach asyncmap 0 ktune
```

* Compile `usb2ppp` with something like `gcc -O3 -o usb2ppp usb2ppp.c`. You might also put your PPP server options into a shell script for convenience (see below).

* Configure your PPP client's PPP options. On the Brother GeoBook, these are accessed from the Internet pane, Options. IP addresses should match what you selected for the PPP server. The phone number doesn't matter and can be left empty (or `empty`).

* Depending on your operating system, the PPP server may need to make dummy modem responses to the PPP client before it will negotiate PPP (for example, the GeoBooks require this, but Apple Remote Access doesn't generally if you don't give it anything to dial; for those systems that *don't*, skip this step). Compile and start the modem simulator on the PPP server with steps like this. Be sure to pass the path to your serial port and the desired port speed, which must match the configuration of your PPP client.

```
% gcc -O3 -o mosim mosim.c
% ./mosim /dev/ttyUSB0 38400
opening /dev/ttyUSB0
setting up for serial access
setting flags on serial port fd=3
```

* On the PPP client, start the PPP connection (if you can't do this explicitly, try accessing a network resource, such as entering a website in GlobeHopper on the GeoBooks). If `mosim` is running (if not, immediately proceed to the next step), the PPP client should run through its `AT` command sequences and then attempt to dial out, after which `mosim` will terminate but leave the connection live.

* Finally, start `usb2ppp` on your PPP server *as `root`, using `sudo` on most systems* with the path to your serial port and the desired port speed, then the path to `pppd` and the PPP options you selected. A command line (which you could put in a shell script) might look like this, with `__.__.__.__` replaced with the appropriate IP addresses explained above:

```
sudo ./usb2ppp /dev/ttyUSB0 38400 /usr/sbin/pppd \
:__.__.__.__ local ms-dns __.__.__.__ netmask __.__.__.__ \
passive noauth proxyarp notty debug persist nodetach asyncmap 0 ktune
```

Your client should now be connected via PPP link to your network. The `ktune` option should temporarily enable IP forwarding while `pppd` is active. If your system can access the PPP server but not your network or the Internet, you may need to set a similar option in your PPP server's operating system.

You can stop `mosim` or `usb2ppp` with CTRL-C, though you may need to start the connection process over from the beginning if your client interprets timing out as a hangup.

## How to set up content-type character set filtering

`charno` is a "stacked" HTTP proxy that filters out `;charset=...` from HTML document content types that older browsers don't understand. It is written in Perl and designed to be run from `inetd` or `xinetd`. It works with most browsers of the era where this filtering would be required (early-mid 1990s), including GlobeHopper on the Brother GeoBook.

"Stacked" means that it does not do HTTP access itself; it relies on another HTTP proxy to make the connection and merely filters that proxy's output. This can be something like Squid, or for HTTPS over HTTP, Crypto Ancienne's `carl` in proxy mode. That proxy must also be accessible via TCP, though it can be on the localhost interface if `charno` is running on the same machine. Point `charno` to that host and port number (either by modifying `charno`'s parameters at the beginning, or passing the host and port number as command line options) and ensure it is executable, then check that it answers on that port number using `nc` or `socat`.

Once set up and functionality is confirmed, start your browser and change the HTTP proxy settings to point to the host and port number on which `charno` is listening.

## How to set up HTTPS/TLS access (Brother GeoBook family only)

To see this in action and get more details, [visit this blog post](https://oldvcr.blogspot.com/2023/06/o-brother-geobook-lets-get-thou-back-on.html). However, here's what you'll need to do, more or less. Note that these steps *have not been tested on the PN-9000GR*, which may have different components (they are known to work fine on the NB-60 and NB-80C, which use the same browser libraries and executable). If you successfully get this working on the PN-9000GR, please open an issue with your steps explained.

* Ensure that HTTP sites work properly with your laptop first, including `charno`. If that's not working, then this won't either.

* Compile and set up [Crypto Ancienne](https://github.com/classilla/cryanc)'s `carl` utility listening in proxy mode via `inetd` or `xinetd`. You'll need to disable timeouts and transparently upgrade HTTP requests to HTTPS as well, so pass `carl` the `-ptu` options in `inetd.conf`, `xinetd.conf` or the analogous configuration file.

* Set `charno`'s host and port parameters to where `carl` is listening, which will serve as `charno`'s HTTP proxy and provide encryption.

* Make sure GlobeHopper isn't running, since the next steps require you to modify its components.

* Copy `F:\GEOWORKS\SYSTEM\WWW\SCHEMES\HTTP.GEO` to `F:\GEOWORKS\SYSTEM\WWW\SCHEMES\HTTPS.GEO`. Note the length of the file. Make the following binary changes to `HTTPS.GEO` with a hex editor and ensure the resulting file length matches the original (you may need to copy it off the system and copy it back on if you don't have a hex editor already installed).

```
-00000000: c745 c153 5757 5720 4854 5450 2073 6368  .E.SWWW HTTP sch
+00000000: c745 c153 5757 5720 4854 4c53 2073 6368  .E.SWWW HTLS sch

-00000030: 0000 0000 0000 0000 4854 5450 0000 4745  ........HTTP..GE
+00000030: 0000 0000 0000 0000 4854 4c53 0000 4745  ........HTLS..GE

-00000130: 2020 2020 6c69 6220 4854 5450 0000 0000      lib HTTP....
+00000130: 7320 2020 6c69 6220 4854 5450 0000 0000  s   lib HTTP....

-000003f0: 7474 7000 0000 0000 b801 0045 558b ec1e  ttp........EU...
+000003f0: 7474 7073 0000 0000 b801 0045 558b ec1e  ttps.......EU...
```

* Create a backup of `F:\GEOWORKS\WORLD\GEOWEB.GEO`. Note the length of the file. Make the following binary change to `GEOWEB.GEO` with a hex editor and ensure the resulting file length matches the original.

```
-000011b0: 7665 0068 7474 7000 6674 7000 4f70 656e  ve.http.ftp.Open
+000011b0: 7665 0068 7474 7073 0074 7000 4f70 656e  ve.https.tp.Open
```

* Start GlobeHopper and initiate your PPP connection, and attempt viewing an HTTPS URL.

With these changes, GlobeHopper will now forward HTTPS requests to `charno`, which will then pass them through Crypto Ancienne and properly clear the character set from the headers it gets back. Note that this breaks using the proxy for *unencrypted* HTTP, which won't work unless you restore the original `GEOWEB.GEO`.

If you don't see the expected binary content in the original file, *stop and don't modify* as you may damage your installation. If you're positive you're looking at the right files, should you open an issue I may ask you for copies to determine what the difference is.

## Don't file bugs or pull requests

Or, if you do, remember that this is a toy project, and changes should be worth my time. To wit:

* Bugs without patches or pull requests may or may not be addressed. Ever.

* Feature requests without patches or pull requests may be closed or deleted.

* Pull requests that refactor or completely rewrite components ("I don't like Perl, this should be Python" "I don't like C, this should be Rust" "I don't like your coding style") will be rejected. For that, fork the project.

On the other hand, a well-written solution to a problem or feature need that doesn't require substantial rewrites or change unrelated sections of code would likely be very welcome.

## License

BURLAP is released under the Floodgap Free Software License. This is a different license than BSD or GPL. You should read it.
