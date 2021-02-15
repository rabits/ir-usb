# IR-USB

Simple application to communicate to IR Tiqiaa Tview 10c4:8468

Based on the reverse-engineering article from XEN: https://habr.com/ru/post/494800/

## Build

You will need g++ and libusb-1.0 dev packages to compile the application.
```
$ ./build.sh
```

## Usage

The application is terminal based, so you need to use arguments in order to make it work.

This command will capture the signal via IR receiver, write it to signal.bin file, after that read
this file and send it via IR transmitter.
```
$ ./ir-usb -r signal.bin -s signal.bin
```

All the commands will be executed sequentially, so you can have quite long list of `-r` and `-s`
with the corresponding files.

## Examples

Examples directory contains a couple of captured signals from Optoma UHD50X projector, which don't
have HDMI-CEC for automation, the reason why I actually started this port.

## With KODI

To automate projector or any IR-available hardware with Kodi and control the media-system by Kore
remote you can use [Callbacks addon](https://kodi.wiki/view/Add-on:Kodi_Callbacks). For example, I
setup it like that:

### 1. Install Kodi on Linux

It's simple, so it's up to you. My setup:
* Outputs are persistant by manually providing captured edid's to cmdline, so they're always
active:
   ```
   drm.edid_firmware=DP-3:myedid/optoma-hdmi20.edid,DP-1:myedid/monitor-new.edid,HDMI-A-1:myedid/onkyo-optoma-hdmi14.edid
   video=DP-3:3840x2160e
   video=DP-1:2560x1440e
   video=HDMI-A-1:1920x1080e
   ```
* Projector 4K@60 output via DP-3 for video to Optoma HDMI2.0 input
* Audio via HDMI-A-1 (1.4) through pulse-eight CEC adapter to Onkyo receiver with 5.1 by pulseaudio

### 2. Disable UPower events

Kodi can actually shutdown or suspend your system, but since I use it on workstation I don't want
that behavior. I usually use terminal to execute reboot or poweroff, so don't need those dbus
methods. So I found that it's possible to modify the access configs of dbus in the login file:
`/usr/share/dbus-1/system.d/org.freedesktop.login1.conf` - just comment the "Suspend" and the other
`allow` rules you don't need and leave "CanSuspend" rules - we will use it later.

When you commented the rules - reload system dbus by `sudo systemctl reload dbus`. Probably there
is some better ways to filter the kodi application from 

### 3. Prepare the scripts

Kodi Callbacks addon will execute the scripts in their folder, so we will need 2 simple scripts:
* `kodi_callback_startup.sh` - it uses `ir-usb` binary to run poweron and switch to HDMI2 input:
   ```
   #!/bin/sh

   ./ir-usb -s ir_signals/optoma-uhd50x-power_on.bin
   sleep 20
   ./ir-usb -s ir_signals/optoma-uhd50x-hdmi20.bin
   ```
* `kodi_callback_shutdown.sh` - just powers off the projector by sending poweroff signal twice:
   ```
   #!/bin/sh

   ./ir-usb -s ir_signals/optoma-uhd50x-power_off.bin -s ir_signals/optoma-uhd50x-power_off.bin
   ```

You can capture your signals and use them in the scripts - just place them in the same folder and
change the paths.

Also make sure you copied the [udev rules file](doc/99-usb-ir-tview.rules) to `/etc/udev/rules.d` to
allow any user to use Tview IR usb device with no special permissions, otherwise `ir-usb` will
require root previleges, which Kodi don't have.

### 4. Setup Kodi Callbacks addon

The addon install process is simple, but it's interface is not great. So I just edited the config
file `~/.kodi/userdata/addon_data/script.service.kodi.callbacks/settings.xml` - attached it to the
[docs](docs/settings.xml). The most interesting moments of the config:
* The first event uses the regular `onScreensaverDeactivated` which is triggered when you interact
with the Kodi UI somehow, so it will execute the first task startup script and power on the
projector.
   ```
   <setting id="E1.task" default="true">Task 1</setting>
   <setting id="E1.task">Task 1</setting>
   ...
   <setting id="T1.scriptfile">/home/user/local/kodi_control/kodi_callback_startup.sh</setting>
   ```
* The second event checks log messages by `onLogSimple` and if it finds error with access to UPower
dbus "Suspend" method - it triggers the second task and executes shutdown script. So you can just
use Kore power commands to actually poweroff your devices.
   ```
   <setting id="E2.matchIf" default="true">interface="org.freedesktop.login1.Manager" member="Suspend"</setting>
   <setting id="E2.task">Task 2</setting>
   ...
   <setting id="T2.maxrunning">1</setting>
   <setting id="T2.scriptfile">/home/user/local/kodi_control/kodi_callback_shutdown.sh</setting>
   ```

### 5. Restart Kodi

Now you can restart Kodi (if you replaced settings.xml manually) and check how it's working.
