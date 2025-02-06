# APRS调制程序  

本程序为使用C语言编写的一个APRS调制模块，将用于悬镜系列卫星的业余无线电活动。

目前仅支持AFSK 1k2。

## 注意
目前路径支持方面存在严重缺陷，尚未实现对多路径的支持。  
目前仅保证支持gcc编译器。

## 编译
```
gcc aprs.c -o aprs -lm
```
## 运行
使用命令行指定各项参数：  
```
用法: ./aprs <'ToCall-SSID'> <'FmCall-SSID'> <'Path1-SSID、Path2-SSID'> <'INFO'> <'Output Filename'>
```  
示例：
```
./aprs "CQ" "BG7ZDQ" "RS0ISS" "APRS TEST" output.wav
```
建议各参数使用`""`包裹。  

## 许可证

本项目采用GNU General Public License v3.0许可证。
