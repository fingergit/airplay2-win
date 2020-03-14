# airplay2-win
Airplay2 for windows.

Migrate [AirplayServer](https://github.com/KqSMea8/AirplayServer) and [dnssd](https://github.com/jevinskie/mDNSResponder) to Windows Platform.

## Build

- Open `airplay2-win.sln` in Visual Studio 2019.
- Make `airplay-dll-demo` as Start Project.
- `Ctrl + B`, Build `airplay-dll-demo`.
- The generated lib and dll files will be placed in `AirPlayServer` folder.

## Reference

- [shairplay](https://github.com/juhovh/shairplay) 
- [dnssd](https://github.com/jevinskie/mDNSResponder)
- [AirplayServer](https://github.com/KqSMea8/AirplayServer)
- [xindawn-windows-airplay-mirroring-sdk](https://github.com/xindawndev/xindawn-windows-airplay-mirroring-sdk)


## 常见问题

- 可以被iOS设备发现，但无法连接。

  确保iOS和Windows处于同一个Wi-Fi，同一个网段下。如果Windows是在虚拟机中运行，确保使用的是桥接网络，而不是共享网络。
  