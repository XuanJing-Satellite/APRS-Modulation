# APRS调制程序  

本程序为使用C语言编写的一个APRS调制模块，将用于悬镜系列卫星的业余无线电活动。

目前支持AFSK 1k2

## 编译
```
gcc aprs.c -o aprs
```
## 运行
使用命令行指定各项参数：  
```
用法: ./aprs <'ToCall-SSID'> <'FmCall-SSID'> <'Path1-SSID、Path2-SSID'> <'INFO'> <'Output Filename'>
```
建议各参数使用`""`包裹。  

## 许可证

本项目采用GNU General Public License v3.0许可证。
