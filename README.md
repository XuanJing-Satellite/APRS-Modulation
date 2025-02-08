# APRS调制程序  

本程序为使用C语言编写的一个APRS调制模块，将用于悬镜系列卫星的业余无线电活动。

## 注意

- *目前仅保证支持gcc编译器，如果您需要使用其他编译器，请自行修改源码。*
- 目前路径支持方面存在严重缺陷，尚未实现对多路径的支持。
- 目前没有实现对APRS格式的完整支持。
- 目前ALSA版本存在潜在问题。

## 功能
- 支持AFSK 1k2

## 依赖

- ALSA音频库（APRS_ALSA.c）

## 编译

```
gcc APRS_WAV.c -o aprs_wav -lm
```
```
gcc APRS_ALSA.c -o aprs_alsa -lm -lasound
```

## 运行

使用命令行指定各项参数：  
```
./aprs_wav <'ToCall-SSID'> <'FmCall-SSID'> <'Path1-SSID、Path2-SSID'> <'INFO'> <'Output Filename'>
```  
```
sudo ./aprs_alsa <'ToCall-SSID'> <'FmCall-SSID'> <'Path1-SSID、Path2-SSID'> <'INFO'>
```  

示例：
```
./aprs_wav "CQ" "BG7ZDQ" "RS0ISS" "APRS TEST" output.wav
./aprs_alsa "CQ" "BG7ZDQ" "RS0ISS" "APRS TEST"
```

建议各参数使用`""`包裹。  

## 许可证

本项目采用GNU General Public License v3.0许可证。
