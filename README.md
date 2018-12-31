# RAIDTraylet
## What
A tray-based, Linux RAID monitor I made for easy visual indication my mdadm RAID1 status.

(Made using Qt 5 on QtCreator 4.6.2 and tested on Ubuntu with KDE 5.44/Plasma 5.12 ... but never quite finished or used significantly then or since; I actually ended up switching from RAID to [rsync](https://wiki.archlinux.org/index.php/Rsync#As_a_backup_utility).)
## How
It sits in your tray, showing a

![alt text][green_dim] if no RAID is detected (initial, pre-scan state),

![alt text][red] if a RAID is faulty, or

![alt text][green] if all RAIDs are good (or recovering).

[green]: ./Images/green.png "Status = GOOD"
[red]: ./Images/red_blink.gif "Status = BAD"
[green_dim]: ./Images/green_dim_cbt.png "Status = MISSING"

Instead of having to manually do `cat /proc/mdstat`, it calls and reads it for you! By default, it does this at an interval of 1 minute, set in the QTimer `scanTimer`.

Since this is for mdadm RAIDs, it uses the syntax of the mdstat file to watch for faulty RAIDs via the presence of tokens like "[U_]" or "(F)". When detecting one, its tray icon starts blinking red and it sends a notification every 15 minutes (interval set in `notifyTimer`) listing the faulty devices. These can be disabled in the tray icon's context menu.

## License
[GNU General Public License v3.0](https://www.gnu.org/licenses/gpl-3.0.html)
